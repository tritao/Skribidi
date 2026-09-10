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

	skb_layout_destroy(layout);
	skb_rasterizer_destroy(rasterizer);
	skb_image_atlas_destroy(atlas);
	skb_font_collection_destroy(fonts);
	skb_temp_alloc_destroy(temp_alloc);
	return 0;
}

int image_atlas_tests(void)
{
	RUN_SUBTEST(test_init);
	RUN_SUBTEST(test_layout_prepare_dirty_epoch);
	return 0;
}
