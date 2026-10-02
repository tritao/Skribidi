// SPDX-FileCopyrightText: 2025 Mikko Mononen
// SPDX-License-Identifier: MIT

#include "test_macros.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "skribidi/skb_layout.h"
#include "skribidi/skb_font_collection.h"
#include "skb_layout_internal.h"

static int test_init(void)
{
	skb_layout_params_t layout_params = {
		.font_collection = NULL,
	};

	skb_layout_t* layout = skb_layout_create(&layout_params);
	ENSURE(layout != NULL);

	skb_layout_destroy(layout);

	return 0;
}

static int test_missing_script(void)
{
	skb_temp_alloc_t* temp_alloc = skb_temp_alloc_create(1024);
	ENSURE(temp_alloc != NULL);

	skb_font_collection_t* font_collection = skb_font_collection_create();
	skb_font_handle_t font_handle = skb_font_collection_add_font(font_collection, "data/IBMPlexSans-Regular.ttf", SKB_FONT_FAMILY_DEFAULT, NULL);
	ENSURE(font_handle);

	skb_layout_params_t layout_params = {
		.font_collection = font_collection,
	};
	skb_attribute_t attributes[] = {
		skb_attribute_make_font_size(15.f),
	};

	// The loaded font should not support the script of the text. We should still get a valid layout, but with invalid glyphs.
	skb_layout_t* layout = skb_layout_create_utf8(temp_alloc, &layout_params, "今天天气晴朗", -1, SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes));
	ENSURE(layout != NULL);
	ENSURE(skb_layout_get_glyphs_count(layout) > 0);
	const skb_glyph_t* glyphs = skb_layout_get_glyphs(layout);
	ENSURE(glyphs[0].gid == 0);

	skb_layout_destroy(layout);
	skb_font_collection_destroy(font_collection);
	skb_temp_alloc_destroy(temp_alloc);

	return 0;
}

typedef struct render_glyph_test_context_t {
	int32_t count;
	bool valid;
	skb_font_handle_t expected_font;
} render_glyph_test_context_t;

static bool inspect_render_glyph(const skb_layout_render_glyph_t* glyph, void* context)
{
	render_glyph_test_context_t* test = (render_glyph_test_context_t*)context;
	if (glyph->font_handle != test->expected_font || glyph->glyph_id == 0 ||
		glyph->font_size <= 0.f || glyph->text_range.start < 0 ||
		glyph->text_range.start >= glyph->text_range.end) {
		test->valid = false;
		return false;
	}
	test->count++;
	return true;
}

static bool stop_after_one_render_glyph(const skb_layout_render_glyph_t* glyph, void* context)
{
	(void)glyph;
	int32_t* count = (int32_t*)context;
	(*count)++;
	return false;
}

static bool count_render_glyphs(const skb_layout_render_glyph_t* glyph, void* context)
{
	(void)glyph;
	int32_t* count = (int32_t*)context;
	(*count)++;
	return true;
}

static int test_render_glyph_iterator(void)
{
	skb_temp_alloc_t* temp_alloc = skb_temp_alloc_create(1024);
	skb_font_collection_t* font_collection = skb_font_collection_create();
	ENSURE(temp_alloc != NULL);
	ENSURE(font_collection != NULL);
	skb_font_handle_t font_handle = skb_font_collection_add_font(
		font_collection, "data/IBMPlexSans-Regular.ttf", SKB_FONT_FAMILY_DEFAULT, NULL);
	ENSURE(font_handle != 0);

	skb_attribute_t attributes[] = {
		skb_attribute_make_font_size(15.f),
	};
	skb_layout_params_t layout_params = {
		.font_collection = font_collection,
		.layout_width = 200.f,
	};
	skb_layout_t* layout = skb_layout_create_utf8(
		temp_alloc, &layout_params, "Skribidi", -1, SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes));
	ENSURE(layout != NULL);

	render_glyph_test_context_t context = {
		.count = 0,
		.valid = true,
		.expected_font = font_handle,
	};
	ENSURE(skb_layout_iterate_render_glyphs(layout, inspect_render_glyph, &context));
	ENSURE(context.valid);
	ENSURE(context.count > 0);

	int32_t stopped_count = 0;
	ENSURE(!skb_layout_iterate_render_glyphs(layout, stop_after_one_render_glyph, &stopped_count));
	ENSURE(stopped_count == 1);

	skb_layout_destroy(layout);
	skb_font_collection_destroy(font_collection);
	skb_temp_alloc_destroy(temp_alloc);
	return 0;
}

static int test_mixed_rtl_glyph_range(void)
{
	skb_temp_alloc_t* temp_alloc = skb_temp_alloc_create(1024);
	skb_font_collection_t* font_collection = skb_font_collection_create();
	ENSURE(temp_alloc != NULL);
	ENSURE(font_collection != NULL);
	skb_font_handle_t font_handle = skb_font_collection_add_font(
		font_collection, "data/IBMPlexSansArabic-Regular.ttf", SKB_FONT_FAMILY_DEFAULT, NULL);
	ENSURE(font_handle != 0);

	skb_attribute_t attributes[] = {
		skb_attribute_make_font_size(15.f),
	};
	skb_layout_params_t layout_params = {
		.font_collection = font_collection,
		.layout_width = 300.f,
	};
	skb_layout_t* layout = skb_layout_create_utf8(
		temp_alloc, &layout_params, "مرحبا שלום", -1,
		SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes));
	ENSURE(layout != NULL);

	// The glyph array is visual order while clusters are logical order. A
	// mixed RTL line must still expose every shaped glyph to render iteration,
	// including missing-glyph placeholders from the deliberately Arabic-only
	// test font.
	int32_t rendered_glyph_count = 0;
	ENSURE(skb_layout_iterate_render_glyphs(layout, count_render_glyphs, &rendered_glyph_count));
	ENSURE(rendered_glyph_count == skb_layout_get_glyphs_count(layout));

	skb_layout_destroy(layout);
	skb_font_collection_destroy(font_collection);
	skb_temp_alloc_destroy(temp_alloc);
	return 0;
}

static int test_caret_pos(void)
{
	skb_temp_alloc_t* temp_alloc = skb_temp_alloc_create(1024);
	ENSURE(temp_alloc != NULL);

	skb_font_collection_t* font_collection = skb_font_collection_create();
	skb_font_handle_t font_handle = skb_font_collection_add_font(font_collection, "data/IBMPlexSans-Regular.ttf", SKB_FONT_FAMILY_DEFAULT, NULL);
	ENSURE(font_handle);

	{
		skb_attribute_t attributes[] = {
			skb_attribute_make_font_size(15.f),
			skb_attribute_make_text_overflow(SKB_OVERFLOW_ELLIPSIS),
		};

		skb_layout_params_t layout_params = {
			.font_collection = font_collection,
			.layout_width = 0.f,
			.layout_height = 0.f,
			.layout_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
		};

		// No lines will be created, because the text is clipped to 0 x 0 rect.
		skb_layout_t* layout = skb_layout_create_utf8(temp_alloc, &layout_params, "moikka\nmoi", -1, (skb_attribute_set_t){0});
		ENSURE(layout != NULL);

		const int32_t line_idx = skb_layout_get_line_index(layout, (skb_text_position_t){ .offset = 6 });
		ENSURE(line_idx == 0);

		skb_layout_destroy(layout);
	}

	{
		skb_attribute_t attributes[] = {
			skb_attribute_make_font_size(15.f),
			skb_attribute_make_text_overflow(SKB_OVERFLOW_ELLIPSIS),
		};

		skb_layout_params_t layout_params = {
			.font_collection = font_collection,
			.layout_width = 45.f,
			.layout_height = 100.f,
			.layout_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
		};

		// No lines will be created, because the text is clipped to 0 x 0 rect.
		skb_layout_t* layout = skb_layout_create_utf8(temp_alloc, &layout_params, "moikka\nmoikka\nmoi", -1, (skb_attribute_set_t){0});
		ENSURE(layout != NULL);

		// Should get line 1 even if it is truncated at end.
		const int32_t line_idx = skb_layout_get_line_index(layout, (skb_text_position_t){ .offset = 13 });
		ENSURE(line_idx == 1);

		// Should get cared position on text that is truncated.
		skb_caret_info_t caret_info = skb_layout_get_caret_info_at(layout, (skb_text_position_t){ .offset = 13 });
		ENSURE(caret_info.x > 0.f);

		// Should get cared position on text that is truncated.
		skb_caret_info_t caret_info2 = skb_layout_get_caret_info_at(layout, (skb_text_position_t){ .offset = 100 });
		ENSURE(caret_info2.x > 0.f);

		// Should get cared position on text that is truncated.
		skb_caret_info_t caret_info3 = skb_layout_get_caret_info_at(layout, (skb_text_position_t){ .offset = -100 });
		ENSURE(caret_info3.x == 0.f);

		skb_layout_destroy(layout);
	}


	skb_font_collection_destroy(font_collection);
	skb_temp_alloc_destroy(temp_alloc);

	return 0;
}

static int test_missing_glyph_caret_position(void)
{
	skb_temp_alloc_t* temp_alloc = skb_temp_alloc_create(1024);
	ENSURE(temp_alloc != NULL);
	skb_font_collection_t* fonts = skb_font_collection_create();
	ENSURE(fonts != NULL);
	ENSURE(skb_font_collection_add_font(fonts, "data/IBMPlexSans-Regular.ttf", SKB_FONT_FAMILY_DEFAULT, NULL));
	skb_attribute_t attributes[] = { skb_attribute_make_font_size(15.f) };
	skb_layout_params_t params = {
		.font_collection = fonts,
		.layout_width = 200.f,
		.layout_height = 100.f,
		.layout_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
	};
	// The font has no emoji glyph. Its insertion position must still stay
	// between the surrounding characters instead of returning the line origin.
	skb_layout_t* layout = skb_layout_create_utf8(temp_alloc, &params, "abc🙂def", -1, (skb_attribute_set_t){0});
	ENSURE(layout != NULL);
	const skb_caret_info_t before = skb_layout_get_caret_info_at(layout, (skb_text_position_t){ .offset = 2 });
	const skb_caret_info_t missing = skb_layout_get_caret_info_at(layout, (skb_text_position_t){ .offset = 3 });
	const skb_caret_info_t after = skb_layout_get_caret_info_at(layout, (skb_text_position_t){ .offset = 4 });
	ENSURE(before.x > 0.f);
	ENSURE(missing.x >= before.x);
	ENSURE(missing.x <= after.x);
	skb_layout_destroy(layout);
	skb_font_collection_destroy(fonts);
	skb_temp_alloc_destroy(temp_alloc);
	return 0;
}

static int test_word_start_at_document_start(void)
{
	skb_temp_alloc_t* temp_alloc = skb_temp_alloc_create(1024);
	ENSURE(temp_alloc != NULL);

	skb_font_collection_t* font_collection = skb_font_collection_create();
	ENSURE(font_collection != NULL);
	skb_font_handle_t font_handle = skb_font_collection_add_font(font_collection, "data/IBMPlexSans-Regular.ttf", SKB_FONT_FAMILY_DEFAULT, NULL);
	ENSURE(font_handle);

	skb_attribute_t attributes[] = {
		skb_attribute_make_font_size(15.f),
	};
	skb_layout_params_t layout_params = {
		.font_collection = font_collection,
		.layout_width = 200.f,
		.layout_height = 100.f,
		.layout_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
	};

	skb_layout_t* layout = skb_layout_create_utf8(temp_alloc, &layout_params, "Hello world", -1, (skb_attribute_set_t){0});
	ENSURE(layout != NULL);

	skb_text_position_t word_start = skb_layout_get_word_start_at(layout, (skb_text_position_t){
		.offset = 0,
		.affinity = SKB_AFFINITY_SOL,
	});
	ENSURE(word_start.offset == 0);

	skb_layout_destroy(layout);
	skb_font_collection_destroy(font_collection);
	skb_temp_alloc_destroy(temp_alloc);

	return 0;
}

// A wrapped word must not rescan its entire suffix for every visual line.
static int test_long_wrapped_word(void)
{
	const int32_t count = 1024 * 1024;
	char* text = malloc((size_t)count + 5);
	ENSURE(text != NULL);
	memset(text, 'a', (size_t)count);
	memcpy(text + count, " \nb", 4);
	skb_temp_alloc_t* temp = skb_temp_alloc_create(1024);
	skb_font_collection_t* fonts = skb_font_collection_create();
	ENSURE(skb_font_collection_add_font(fonts, "data/IBMPlexSans-Regular.ttf", SKB_FONT_FAMILY_DEFAULT, NULL));
	const skb_attribute_t attributes[] = {
		skb_attribute_make_font_size(15.f),
		skb_attribute_make_text_wrap(SKB_WRAP_WORD_CHAR),
	};
	const skb_layout_params_t params = {
		.font_collection = fonts,
		.layout_width = 200.f,
		.layout_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
	};
	const clock_t started = clock();
	skb_layout_t* layout = skb_layout_create_utf8(temp, &params, text, -1, (skb_attribute_set_t){0});
	ENSURE(layout != NULL);
	printf("long wrapped word: %.3f CPU seconds\n", (double)(clock() - started) / CLOCKS_PER_SEC);
	const int32_t lines_count = skb_layout_get_lines_count(layout);
	const skb_layout_line_t* lines = skb_layout_get_lines(layout);
	ENSURE(lines_count > 1000);
	int32_t end = 0;
	for (int32_t i = 0; i < lines_count; i++) {
		ENSURE(lines[i].text_range.start == end);
		ENSURE(lines[i].text_range.end >= end);
		end = lines[i].text_range.end;
		ENSURE(lines[i].bounds.width <= 200.01f);
	}
	ENSURE(end == count + 3);
	// A request for a middle visual line must not visit the million-glyph tail.
	const int32_t middle = lines_count / 2;
	const skb_layout_run_t* runs = skb_layout_get_layout_runs(layout);
	int32_t expected = 0;
	for (int32_t i = lines[middle].layout_run_range.start; i < lines[middle].layout_run_range.end; i++)
		expected += runs[i].glyph_range.end - runs[i].glyph_range.start;
	int32_t visited = 0;
	ENSURE(skb_layout_iterate_render_glyphs_range(layout, (skb_range_t){middle, middle + 1}, count_render_glyphs, &visited));
	ENSURE(visited == expected && visited > 0 && visited < 100);
	visited = 0;
	ENSURE(skb_layout_iterate_render_glyphs_range(layout, (skb_range_t){middle, middle}, count_render_glyphs, &visited));
	ENSURE(visited == 0);
	ENSURE(!skb_layout_iterate_render_glyphs_range(layout, (skb_range_t){-1, 1}, count_render_glyphs, &visited));
	ENSURE(!skb_layout_iterate_render_glyphs_range(layout, (skb_range_t){0, lines_count + 1}, count_render_glyphs, &visited));
	ENSURE(!skb_layout_iterate_render_glyphs_range(layout, (skb_range_t){middle + 1, middle}, count_render_glyphs, &visited));
	ENSURE(!skb_layout_iterate_render_glyphs_range(layout, (skb_range_t){middle, middle + 1}, stop_after_one_render_glyph, &visited));
	ENSURE(visited == 1);

	skb_layout_destroy(layout);
	skb_font_collection_destroy(fonts);
	skb_temp_alloc_destroy(temp);
	free(text);
	return 0;
}

static int test_indexed_changed_wrap(void)
{
	skb_temp_alloc_t* temp = skb_temp_alloc_create(1024);
	skb_font_collection_t* fonts = skb_font_collection_create();
	ENSURE(skb_font_collection_add_font(fonts, "data/IBMPlexSans-Regular.ttf", SKB_FONT_FAMILY_DEFAULT, NULL));
	for (int truncation = 0; truncation < 2; ++truncation) {
		const skb_attribute_t attrs[] = {
			skb_attribute_make_font_size(15.f),
			skb_attribute_make_text_wrap(SKB_WRAP_WORD_CHAR),
			skb_attribute_make_text_overflow(truncation ? SKB_OVERFLOW_ELLIPSIS : SKB_OVERFLOW_NONE),
		};
		skb_layout_params_t params = {
			.font_collection = fonts, .layout_width = 143.f, .layout_height = 100000.f,
			.layout_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attrs),
		};
		char text[160];
		memset(text, 'b', 128); text[128] = 0;
		skb_layout_t* original = skb_layout_create_utf8(temp, &params, text, -1, (skb_attribute_set_t){0});
		ENSURE(original);
		ENSURE(!skb_layout_create_ascii_edit(original, temp, 32, 33, "W", 1));
		const skb__shape_block_t* prefix = original->shape_block;
		skb_layout_t* edited = skb_layout_create_ascii_edit(original, temp, 32, 33, "wwwwwwww", 8);
		ENSURE(edited);
		ENSURE((edited->shape_pieces_count > 0) == !truncation);
		if (!truncation) {
			ENSURE(edited->shape_pieces[0].block == prefix);
			ENSURE(!edited->text && !edited->text_props && !edited->glyphs && !edited->clusters);
		}
		bool moved = edited->lines_count != original->lines_count;
		for (int row = 0; row + 1 < original->lines_count && row + 1 < edited->lines_count; ++row)
			moved |= edited->lines[row].text_range.end != original->lines[row].text_range.end;
		ENSURE(moved);
		skb_layout_destroy(original);
		memmove(text + 40, text + 33, 96);
		memcpy(text + 32, "wwwwwwww", 8);
		skb_layout_t* fresh = skb_layout_create_utf8(temp, &params, text, -1, (skb_attribute_set_t){0});
		ENSURE(fresh && edited->text_count == 135 && edited->lines_count == fresh->lines_count);
		for (int i = 0; i < 135; ++i) {
			const skb_glyph_t a = skb_layout_get_glyph_at(edited, i);
			const skb_glyph_t b = skb_layout_get_glyph_at(fresh, i);
			ENSURE(a.gid == b.gid && a.cluster_idx == b.cluster_idx && a.advance_x == b.advance_x);
			ENSURE(fabsf(a.offset_x - b.offset_x) < .001f && fabsf(a.offset_y - b.offset_y) < .001f);
		}
		if (!truncation)
			ENSURE(!edited->shape_cache->text && !edited->shape_cache->properties &&
				!edited->shape_cache->glyphs && !edited->shape_cache->clusters);
		skb_layout_destroy(edited);
		skb_layout_destroy(fresh);
	}
	skb_font_collection_destroy(fonts);
	skb_temp_alloc_destroy(temp);
	return 0;
}

static int test_incremental_row_work(void)
{
	skb_temp_alloc_t* temp = skb_temp_alloc_create(1024);
	skb_font_collection_t* fonts = skb_font_collection_create();
	ENSURE(skb_font_collection_add_font(fonts, "data/IBMPlexSans-Regular.ttf", SKB_FONT_FAMILY_DEFAULT, NULL));
	const skb_attribute_t attrs[] = {skb_attribute_make_font_size(15.f), skb_attribute_make_text_wrap(SKB_WRAP_WORD_CHAR)};
	skb_layout_params_t params = {.font_collection = fonts, .layout_width = 43.f,
		.layout_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attrs)};
	char text[4113]; memset(text, 'b', 4096); text[4096] = 0; text[260] = 'w';
	skb_layout_t* original = skb_layout_create_utf8(temp, &params, text, -1, (skb_attribute_set_t){0});
	ENSURE(original);
	skb_layout_t* edited = skb_layout_create_ascii_edit(original, temp, 256, 264, "wbbb bbb", 8);
	ENSURE(!edited); // Whitespace remains outside the guarded ASCII contract.
	edited = skb_layout_create_ascii_edit(original, temp, 256, 264, "wbbbbbbb", 8);
	ENSURE(edited);
	// The equal-length exchange recovers the unchanged suffix boundary.
	memcpy(text + 256, "wbbbbbbb", 8);
	skb_layout_t* fresh = skb_layout_create_utf8(temp, &params, text, -1, (skb_attribute_set_t){0});
	ENSURE(fresh && edited->lines_count == fresh->lines_count);
	ENSURE(edited->reused_prefix_rows > 0 && edited->reused_suffix_rows > 0);
	ENSURE(edited->reflowed_rows < 20 && edited->reflow_measured_clusters < 256);
	printf("incremental rows: %d prefix, %d reflowed, %d suffix, %llu measured clusters\n",
		edited->reused_prefix_rows, edited->reflowed_rows, edited->reused_suffix_rows,
		(unsigned long long)edited->reflow_measured_clusters);
	for (int i = 0; i < 4096; ++i) {
		const skb_glyph_t a = skb_layout_get_glyph_at(edited, i), b = skb_layout_get_glyph_at(fresh, i);
		ENSURE(a.gid == b.gid && fabsf(a.offset_x - b.offset_x) < .001f && fabsf(a.offset_y - b.offset_y) < .001f);
	}
	// Lookup keeps the original linear semantics at every offset and boundary.
	for (int offset = -1; offset <= 4097; ++offset) {
		int expected_row = 0;
		for (int row = fresh->lines_count - 1; row >= 0; --row) {
			if (fresh->lines[row].text_range.start <= offset) { expected_row = row; break; }
		}
		ENSURE(skb_layout_get_line_index(edited, (skb_text_position_t){.offset = offset}) == expected_row);
	}
	// A row-count change must not claim convergence at the old vertical positions.
	skb_layout_t* tail = skb_layout_create_ascii_edit(original, temp, 2048, 2048, "wwwwwwww", 8);
	ENSURE(tail && tail->reused_prefix_rows > 0 && tail->reused_suffix_rows == 0);
	memset(text, 'b', 4096); text[4096] = 0; text[260] = 'w';
	memmove(text + 2056, text + 2048, 2049); memcpy(text + 2048, "wwwwwwww", 8);
	skb_layout_t* tail_fresh = skb_layout_create_utf8(temp, &params, text, -1, (skb_attribute_set_t){0});
	ENSURE(tail_fresh && tail->lines_count == tail_fresh->lines_count);
	for (int i = 0; i < 4104; ++i) {
		const skb_glyph_t a = skb_layout_get_glyph_at(tail, i), b = skb_layout_get_glyph_at(tail_fresh, i);
		ENSURE(a.gid == b.gid && fabsf(a.offset_x - b.offset_x) < .001f && fabsf(a.offset_y - b.offset_y) < .001f);
	}
	skb_layout_destroy(tail); skb_layout_destroy(tail_fresh);
	ENSURE(original->ascii_shape_valid && original->ascii_rows_valid);
	skb_layout_set_utf8(original, temp, &params, "UPPERCASE", -1, (skb_attribute_set_t){0});
	ENSURE(!original->ascii_shape_valid && !original->ascii_rows_valid);
	ENSURE(!skb_layout_create_ascii_edit(original, temp, 0, 1, "b", 1));
	skb_layout_destroy(original); skb_layout_destroy(edited); skb_layout_destroy(fresh);
	skb_font_collection_destroy(fonts); skb_temp_alloc_destroy(temp);
	return 0;
}

int layout_tests(void)
{
	RUN_SUBTEST(test_init);
	RUN_SUBTEST(test_indexed_changed_wrap);
	RUN_SUBTEST(test_incremental_row_work);
	RUN_SUBTEST(test_long_wrapped_word);
	RUN_SUBTEST(test_missing_script);
	RUN_SUBTEST(test_render_glyph_iterator);
	RUN_SUBTEST(test_mixed_rtl_glyph_range);
	RUN_SUBTEST(test_caret_pos);
	RUN_SUBTEST(test_missing_glyph_caret_position);
	RUN_SUBTEST(test_word_start_at_document_start);
	return 0;
}
