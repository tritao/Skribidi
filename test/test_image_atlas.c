// SPDX-FileCopyrightText: 2025 Mikko Mononen
// SPDX-License-Identifier: MIT

#include "test_macros.h"
#include "skribidi/skb_image_atlas.h"
#include "skribidi/skb_layout.h"
#include "skribidi/skb_font_collection.h"
#include "skribidi/skb_rasterizer.h"

static int test_init(void)
{
	skb_image_atlas_t* atlas = skb_image_atlas_create(NULL);
	ENSURE(atlas != NULL);

	skb_image_atlas_destroy(atlas);

	return 0;
}

static int test_layout_prepare_dirty_epoch(void)
{
	skb_temp_alloc_t* temp_alloc = skb_temp_alloc_create(64 * 1024);
	skb_font_collection_t* fonts = skb_font_collection_create();
	skb_image_atlas_t* atlas = skb_image_atlas_create(NULL);
	skb_rasterizer_t* rasterizer = skb_rasterizer_create(NULL);
	ENSURE(temp_alloc != NULL);
	ENSURE(fonts != NULL);
	ENSURE(atlas != NULL);
	ENSURE(rasterizer != NULL);
	ENSURE(skb_font_collection_add_font(fonts, "data/IBMPlexSans-Regular.ttf", SKB_FONT_FAMILY_DEFAULT, NULL) != 0);

	skb_layout_params_t params = {
		.font_collection = fonts,
		.layout_width = 200.f,
	};
	skb_attribute_t attributes[] = {
		skb_attribute_make_font_size(24.f),
	};
	skb_layout_t* layout = skb_layout_create_utf8(
		temp_alloc, &params, "Skribidi", -1, SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes));
	ENSURE(layout != NULL);
	ENSURE(skb_layout_prepare_glyphs(layout, atlas, temp_alloc, rasterizer, 1.f, SKB_RASTERIZE_ALPHA_MASK));
	const skb_image_atlas_stats_t mask_stats = skb_image_atlas_get_stats(atlas);
	ENSURE(mask_stats.glyph_cache_misses > 0);
	ENSURE(mask_stats.glyphs_rasterized > 0);

	bool saw_mask = false;
	bool tested_newer_epoch = false;
	for (int32_t index = 0; index < skb_image_atlas_get_texture_count(atlas); index++) {
		const skb_image_atlas_dirty_snapshot_t snapshot = skb_image_atlas_peek_texture_dirty(atlas, index);
		if (!snapshot.epoch)
			continue;
		saw_mask = saw_mask || snapshot.format == SKB_IMAGE_ATLAS_FORMAT_R8_MASK;
		ENSURE(snapshot.format == SKB_IMAGE_ATLAS_FORMAT_R8_MASK);
		ENSURE(snapshot.width > 0 && snapshot.height > 0 && snapshot.stride_bytes > 0);
		ENSURE(snapshot.pixels != NULL);
		ENSURE(!skb_image_atlas_ack_texture_dirty(atlas, index, snapshot.epoch + 1));
		ENSURE(skb_image_atlas_peek_texture_dirty(atlas, index).epoch == snapshot.epoch);
		if (!tested_newer_epoch) {
			skb_layout_t* newer_layout = skb_layout_create_utf8(
				temp_alloc, &params, "Skribidi!", -1, SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes));
			ENSURE(newer_layout != NULL);
			ENSURE(skb_layout_prepare_glyphs(
				newer_layout, atlas, temp_alloc, rasterizer, 1.f, SKB_RASTERIZE_ALPHA_MASK));
			const skb_image_atlas_dirty_snapshot_t newer_snapshot =
				skb_image_atlas_peek_texture_dirty(atlas, index);
			ENSURE(newer_snapshot.epoch != snapshot.epoch);
			ENSURE(!skb_image_atlas_ack_texture_dirty(atlas, index, snapshot.epoch));
			ENSURE(skb_image_atlas_peek_texture_dirty(atlas, index).epoch == newer_snapshot.epoch);
			skb_layout_destroy(newer_layout);
			tested_newer_epoch = true;
		}
		const skb_image_atlas_dirty_snapshot_t current = skb_image_atlas_peek_texture_dirty(atlas, index);
		ENSURE(skb_image_atlas_ack_texture_dirty(atlas, index, current.epoch));
		ENSURE(skb_image_atlas_peek_texture_dirty(atlas, index).epoch == 0);
	}
	ENSURE(saw_mask);
	ENSURE(tested_newer_epoch);

	ENSURE(skb_layout_prepare_glyphs(layout, atlas, temp_alloc, rasterizer, 1.f, SKB_RASTERIZE_ALPHA_SDF));
	bool saw_sdf = false;
	for (int32_t index = 0; index < skb_image_atlas_get_texture_count(atlas); index++) {
		const skb_image_atlas_dirty_snapshot_t snapshot = skb_image_atlas_peek_texture_dirty(atlas, index);
		if (!snapshot.epoch)
			continue;
		saw_sdf = saw_sdf || snapshot.format == SKB_IMAGE_ATLAS_FORMAT_R8_SDF;
		ENSURE(snapshot.format == SKB_IMAGE_ATLAS_FORMAT_R8_SDF);
		ENSURE(skb_image_atlas_ack_texture_dirty(atlas, index, snapshot.epoch));
	}
	ENSURE(saw_sdf);
	const skb_image_atlas_stats_t final_stats = skb_image_atlas_get_stats(atlas);
	ENSURE(final_stats.glyph_cache_misses > mask_stats.glyph_cache_misses);
	ENSURE(final_stats.glyphs_rasterized > mask_stats.glyphs_rasterized);

	skb_layout_destroy(layout);
	skb_rasterizer_destroy(rasterizer);
	skb_image_atlas_destroy(atlas);
	skb_font_collection_destroy(fonts);
	skb_temp_alloc_destroy(temp_alloc);
	return 0;
}

typedef struct generation_test_context_t {
	skb_image_atlas_t* atlas;
	skb_font_collection_t* fonts;
	bool valid;
} generation_test_context_t;

static bool verify_quad_generation(const skb_layout_render_glyph_t* glyph, void* context)
{
	generation_test_context_t* test = (generation_test_context_t*)context;
	const skb_quad_t quad = skb_image_atlas_get_glyph_quad(
		test->atlas, 0.f, 0.f, 1.f, test->fonts, glyph->font_handle, glyph->glyph_id,
		glyph->font_size, glyph->color, SKB_RASTERIZE_ALPHA_MASK);
	if (quad.flags & SKB_QUAD_IS_EMPTY)
		return true;
	if (quad.texture_idx >= skb_image_atlas_get_texture_count(test->atlas) ||
		quad.texture_generation !=
			skb_image_atlas_get_texture_generation(test->atlas, quad.texture_idx)) {
		test->valid = false;
		return false;
	}
	return true;
}

static int test_texture_generation_growth(void)
{
	skb_temp_alloc_t* temp_alloc = skb_temp_alloc_create(64 * 1024);
	skb_font_collection_t* fonts = skb_font_collection_create();
	skb_rasterizer_t* rasterizer = skb_rasterizer_create(NULL);
	skb_image_atlas_config_t config = skb_image_atlas_get_default_config();
	config.init_width = 64;
	config.init_height = 64;
	config.expand_size = 64;
	config.max_width = 64;
	config.max_height = 128;
	skb_image_atlas_t* atlas = skb_image_atlas_create(&config);
	ENSURE(temp_alloc != NULL);
	ENSURE(fonts != NULL);
	ENSURE(rasterizer != NULL);
	ENSURE(atlas != NULL);
	skb_font_handle_t font = skb_font_collection_add_font(
		fonts, "data/IBMPlexSans-Regular.ttf", SKB_FONT_FAMILY_DEFAULT, NULL);
	ENSURE(font != 0);
	skb_layout_params_t params = {
		.font_collection = fonts,
		.layout_width = 400.f,
	};
	skb_attribute_t attributes[] = {
		skb_attribute_make_font_size(24.f),
	};
	skb_layout_t* layout = skb_layout_create_utf8(
		temp_alloc, &params, "ABCDEFGHIJKLMNOP", -1,
		SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes));
	ENSURE(layout != NULL);
	ENSURE(skb_layout_prepare_glyphs(layout, atlas, temp_alloc, rasterizer, 1.f,
		SKB_RASTERIZE_ALPHA_MASK));

	bool saw_growth = false;
	for (int32_t index = 0; index < skb_image_atlas_get_texture_count(atlas); index++) {
		const uint32_t generation = skb_image_atlas_get_texture_generation(atlas, index);
		const skb_image_atlas_dirty_snapshot_t snapshot =
			skb_image_atlas_peek_texture_dirty(atlas, index);
		ENSURE(snapshot.texture_generation == generation);
		saw_growth = saw_growth || generation > 1;
	}
	ENSURE(saw_growth);
	generation_test_context_t context = {
		.atlas = atlas,
		.fonts = fonts,
		.valid = true,
	};
	ENSURE(skb_layout_iterate_render_glyphs(layout, verify_quad_generation, &context));
	ENSURE(context.valid);

	skb_layout_destroy(layout);
	skb_image_atlas_destroy(atlas);
	skb_rasterizer_destroy(rasterizer);
	skb_font_collection_destroy(fonts);
	skb_temp_alloc_destroy(temp_alloc);
	return 0;
}

int image_atlas_tests(void)
{
	RUN_SUBTEST(test_init);
	RUN_SUBTEST(test_layout_prepare_dirty_epoch);
	RUN_SUBTEST(test_texture_generation_growth);
	return 0;
}
