// SPDX-FileCopyrightText: 2025 Mikko Mononen
// SPDX-License-Identifier: MIT

#ifndef SKB_LAYOUT_INTERNAL_H
#define SKB_LAYOUT_INTERNAL_H

#include <assert.h>
#include <stdint.h>

// Internal representation of a content run.
typedef struct skb__content_run_t {
	float content_width;				// Width of object or icon specified by the run
	float content_height;				// Height of object or icon specified by the run
	intptr_t content_data;				// Data of object or icon specified by the run
	intptr_t content_id;				// Custom identifier for a content run.
	skb_range_t text_range;				// Range of text the attributes apply to.
	skb_range_t attributes_range;		// The content attributes
	uint8_t type;						// Type of the content run which described the attributes. See skb_content_run_type_t.
	bool has_text_background;
} skb__content_run_t;

// Represents run of text in same script, font and style, for shaping.
typedef struct skb__shaping_run_t {
	skb_range_t text_range;
	skb_range_t glyph_range;			// Glyphs are in visual oder.
	skb_range_t cluster_range;			// Clusters are in logical order.
	int32_t content_run_idx;
	uint8_t script;
	uint8_t direction;
	uint8_t bidi_level;
	bool is_emoji;
	bool has_baseline_shift;
	float font_size;					// Cached font size for the run.
	skb_font_handle_t font_handle;
	float padding_start;
	float padding_end;
} skb__shaping_run_t;

// Owned shape buffers. References belong to flat layouts or indexed pieces;
// row geometry and font collections are not owned by these blocks.
typedef struct skb__shape_block_t {
	int32_t references;
	uint32_t* text;
	skb_text_property_t* properties;
	skb_glyph_t* glyphs;
	skb_cluster_t* clusters;
} skb__shape_block_t;

typedef struct skb__shape_piece_t {
	skb__shape_block_t* block;
	int32_t source_start;
	int32_t destination_start;
	int32_t count;
} skb__shape_piece_t;

// Mutable compatibility cache, logically separate from immutable shape data.
typedef struct skb__shape_cache_t {
	uint32_t* text;
	skb_text_property_t* properties;
	skb_glyph_t* glyphs;
	skb_cluster_t* clusters;
} skb__shape_cache_t;

typedef struct skb_layout_t {
	skb_layout_params_t params;	// Note: params has 'base_attributes' slice which points to attributes in the 'attributes' array.
	uint64_t generation;		// Changes whenever the layout content or parameters are rebuilt.

	skb_rect2_t bounds;
	skb_padding2_t padding;
	float advance_y;
	uint8_t resolved_direction;
	uint32_t flags; // See skb_layout_flags_t

	// Text, text props, content_runs, and attributes are create based on the input text.
	uint32_t* text;
	skb_text_property_t* text_props;
	int32_t text_count;
	int32_t text_cap;

	skb__content_run_t* content_runs;
	int32_t content_runs_count;
	int32_t content_runs_cap;

	skb_attribute_t* attributes;
	int32_t attributes_count;
	int32_t attributes_cap;

	// Shaping runs is the output if itemization. The shaping runs are in logical order.
	skb__shaping_run_t* shaping_runs;
	int32_t shaping_runs_count;
	int32_t shaping_runs_cap;

	// Glyphs and clusters are output of shaping.
	skb_glyph_t* glyphs;
	int32_t glyphs_count;
	int32_t glyphs_cap;

	skb_cluster_t* clusters;
	int32_t clusters_count;
	int32_t clusters_cap;

	// Lines, layout runs, and decorations are output of line layout.
	skb_layout_line_t* lines;
	int32_t lines_count;
	int32_t lines_cap;

	// The layout runs are in visual order.
	skb_layout_run_t* layout_runs;
	int32_t layout_runs_count;
	int32_t layout_runs_cap;

	skb_decoration_t* decorations;
	int32_t decorations_count;
	int32_t decorations_cap;

	skb__shape_block_t* shape_block;
	skb__shape_piece_t* shape_pieces;
	int32_t shape_pieces_count;
	int32_t shape_pieces_cap;
	skb__shape_cache_t* shape_cache;

	uint8_t should_free_instance;
} skb_layout_t;

// Indexed reads are the shared boundary for rendering and geometry. Values
// never borrow an element address, allowing storage to be immutable pieces.
static inline const skb__shape_piece_t* skb__layout_piece_at(const skb_layout_t* layout, int32_t index)
{
	int32_t lo = 0, hi = layout->shape_pieces_count;
	while (lo < hi) {
		const int32_t mid = lo + (hi - lo) / 2;
		const skb__shape_piece_t* piece = &layout->shape_pieces[mid];
		if (index >= piece->destination_start + piece->count)
			lo = mid + 1;
		else
			hi = mid;
	}
	assert(lo < layout->shape_pieces_count);
	return &layout->shape_pieces[lo];
}

static inline uint32_t skb__layout_text_at(const skb_layout_t* layout, int32_t index)
{
	assert(layout && index >= 0 && index < layout->text_count);
	if (!layout->shape_pieces_count) return layout->text[index];
	const skb__shape_piece_t* piece = skb__layout_piece_at(layout, index);
	return piece->block->text[piece->source_start + index - piece->destination_start];
}

static inline skb_text_property_t skb__layout_text_property_at(const skb_layout_t* layout, int32_t index)
{
	assert(layout && index >= 0 && index < layout->text_count);
	if (!layout->shape_pieces_count) return layout->text_props[index];
	const skb__shape_piece_t* piece = skb__layout_piece_at(layout, index);
	return piece->block->properties[piece->source_start + index - piece->destination_start];
}

// Shape reads do not calculate position; advance-only geometry walks use this.
static inline skb_glyph_t skb__layout_shape_glyph_at(const skb_layout_t* layout, int32_t index)
{
	assert(layout && index >= 0 && index < layout->glyphs_count);
	if (!layout->shape_pieces_count) return layout->glyphs[index];
	const skb__shape_piece_t* piece = skb__layout_piece_at(layout, index);
	skb_glyph_t glyph = piece->block->glyphs[piece->source_start + index - piece->destination_start];
	glyph.cluster_idx += piece->destination_start - piece->source_start;
	return glyph;
}

static inline skb_glyph_t skb__layout_glyph_at(const skb_layout_t* layout, int32_t index)
{
	skb_glyph_t glyph = skb__layout_shape_glyph_at(layout, index);
	if (!layout->shape_pieces_count) return glyph;
	if (layout->shape_cache->glyphs) return layout->shape_cache->glyphs[index];
	int32_t lo = 0, hi = layout->lines_count;
	while (lo < hi) {
		const int32_t mid = lo + (hi - lo) / 2;
		if (layout->lines[mid].text_range.end <= index) lo = mid + 1;
		else hi = mid;
	}
	assert(lo < layout->lines_count);
	const skb_layout_line_t* line = &layout->lines[lo];
	float x = line->bounds.x;
	for (int32_t i = line->text_range.start; i < index; ++i)
		x += skb__layout_shape_glyph_at(layout, i).advance_x;
	glyph.offset_x = x;
	glyph.offset_y = line->baseline;
	return glyph;
}

static inline skb_cluster_t skb__layout_cluster_at(const skb_layout_t* layout, int32_t index)
{
	assert(layout && index >= 0 && index < layout->clusters_count);
	if (!layout->shape_pieces_count) return layout->clusters[index];
	const skb__shape_piece_t* piece = skb__layout_piece_at(layout, index);
	skb_cluster_t cluster = piece->block->clusters[piece->source_start + index - piece->destination_start];
	cluster.text_offset += piece->destination_start - piece->source_start;
	cluster.glyphs_offset += piece->destination_start - piece->source_start;
	return cluster;
}

skb_layout_t skb_layout_make_empty(void);
bool skb_layout_add_ellipsis_to_last_line(skb_layout_t* layout);

#endif // SKB_LAYOUT_INTERNAL_H
