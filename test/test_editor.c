// SPDX-FileCopyrightText: 2025 Mikko Mononen
// SPDX-License-Identifier: MIT

#include <string.h>
#include "test_macros.h"
#include "skribidi/skb_editor.h"
#include "skribidi/skb_font_collection.h"
#include "skribidi/skb_text.h"

typedef struct edit_callback_state_t {
	int text_change_count;
} edit_callback_state_t;

static void on_edit_text_change(skb_editor_t* editor, skb_editor_text_change_reason_t reason, void* context)
{
	SKB_UNUSED(editor);
	SKB_UNUSED(reason);
	edit_callback_state_t* state = context;
	state->text_change_count++;
}

static int test_init(void)
{
	skb_attribute_t attributes[] = {
		skb_attribute_make_font_size(15.f),
	};

	skb_editor_params_t params = {
	 	.font_collection = NULL,
		.caret_mode = SKB_CARET_MODE_SKRIBIDI,
		.paragraph_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
	};

	skb_editor_t* editor = skb_editor_create(&params);
	ENSURE(editor != NULL);

	skb_editor_destroy(editor);

	return 0;
}

static int test_command_line_navigation_macos(void)
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

	skb_editor_params_t params = {
	 	.font_collection = font_collection,
		.caret_mode = SKB_CARET_MODE_SKRIBIDI,
		.editor_behavior = SKB_BEHAVIOR_MACOS,
		.paragraph_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
	};

	skb_editor_t* editor = skb_editor_create(&params);
	ENSURE(editor != NULL);

	// Initialize editor with test text
	const char* test_text = "Hello world\nThis is a test\nabout line jumping";
	skb_editor_set_text_utf8(editor, temp_alloc, test_text, (int32_t)strlen(test_text));

	// Get initial selection - should be at document start
	skb_text_range_t initial_selection = skb_editor_get_current_selection(editor);
	ENSURE(initial_selection.start.offset == 0);
	ENSURE(initial_selection.end.offset == 0);

	// Test Command+Right (should jump to end of line on macOS)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_RIGHT, SKB_MOD_COMMAND);

	skb_text_range_t after_command_right = skb_editor_get_current_selection(editor);
	// Should be at end of first line (position 11, after "Hello world")
	ENSURE(after_command_right.start.offset == 11);
	ENSURE(after_command_right.end.offset == 11);

	// Test Command+Left (should jump to beginning of line on macOS)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_LEFT, SKB_MOD_COMMAND);

	skb_text_range_t after_command_left = skb_editor_get_current_selection(editor);
	// Should be back at beginning of line (position 0)
	ENSURE(after_command_left.start.offset == 0);
	ENSURE(after_command_left.end.offset == 0);

	skb_editor_destroy(editor);
	skb_font_collection_destroy(font_collection);
	skb_temp_alloc_destroy(temp_alloc);

	return 0;
}

static int test_command_document_navigation_macos(void)
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

	skb_editor_params_t params = {
		.font_collection = font_collection,
		.caret_mode = SKB_CARET_MODE_SKRIBIDI,
		.editor_behavior = SKB_BEHAVIOR_MACOS,
		.paragraph_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
	};

	skb_editor_t* editor = skb_editor_create(&params);
	ENSURE(editor != NULL);

	// Initialize editor with test text
	const char* test_text = "Hello world\nThis is a test\nabout line jumping";
	skb_editor_set_text_utf8(editor, temp_alloc, test_text, (int32_t)strlen(test_text));

	// Get initial selection - should be at document start
	skb_text_range_t initial_selection = skb_editor_get_current_selection(editor);
	ENSURE(initial_selection.start.offset == 0);
	ENSURE(initial_selection.end.offset == 0);

	// Test Command+Down (should navigate to document end on macOS)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_DOWN, SKB_MOD_COMMAND);

	skb_text_range_t after_command_down = skb_editor_get_current_selection(editor);
	int32_t expected_end = (int32_t)strlen(test_text);
	ENSURE(after_command_down.start.offset == expected_end);
	ENSURE(after_command_down.end.offset == expected_end);

	// Test Command+Up (should navigate to document start on macOS)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_UP, SKB_MOD_COMMAND);

	skb_text_range_t after_command_up = skb_editor_get_current_selection(editor);
	ENSURE(after_command_up.start.offset == 0);
	ENSURE(after_command_up.end.offset == 0);

	// Position caret in the middle of the document for more comprehensive testing
	skb_text_position_t middle_pos = {.offset = 20, .affinity = SKB_AFFINITY_TRAILING}; // Around "This is a test"
	skb_text_range_t middle_selection = {.start = middle_pos, .end = middle_pos};
	skb_editor_select(editor, middle_selection);

	skb_text_range_t middle_check = skb_editor_get_current_selection(editor);
	ENSURE(middle_check.start.offset == 20);
	ENSURE(middle_check.end.offset == 20);

	// Test Command+Up from middle (should go to document start)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_UP, SKB_MOD_COMMAND);

	skb_text_range_t from_middle_up = skb_editor_get_current_selection(editor);
	ENSURE(from_middle_up.start.offset == 0);
	ENSURE(from_middle_up.end.offset == 0);

	// Reset to middle position
	skb_editor_select(editor, middle_selection);

	// Test Command+Down from middle (should go to document end)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_DOWN, SKB_MOD_COMMAND);

	skb_text_range_t from_middle_down = skb_editor_get_current_selection(editor);
	ENSURE(from_middle_down.start.offset == expected_end);
	ENSURE(from_middle_down.end.offset == expected_end);

	skb_editor_destroy(editor);
	skb_font_collection_destroy(font_collection);
	skb_temp_alloc_destroy(temp_alloc);

	return 0;
}

static int test_shift_command_text_selection_macos(void)
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

	skb_editor_params_t params = {
		.font_collection = font_collection,
		.caret_mode = SKB_CARET_MODE_SKRIBIDI,
		.editor_behavior = SKB_BEHAVIOR_MACOS,
		.paragraph_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
	};

	skb_editor_t* editor = skb_editor_create(&params);
	ENSURE(editor != NULL);

	// Initialize editor with test text
	const char* test_text = "Hello world\nThis is a test\nabout line jumping";
	skb_editor_set_text_utf8(editor, temp_alloc, test_text, (int32_t)strlen(test_text));

	// Get initial selection - should be at document start
	skb_text_range_t initial_selection = skb_editor_get_current_selection(editor);
	ENSURE(initial_selection.start.offset == 0);
	ENSURE(initial_selection.end.offset == 0);

	// Test Shift+Command+Down (should select from start to document end)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_DOWN, SKB_MOD_SHIFT | SKB_MOD_COMMAND);

	skb_text_range_t full_selection = skb_editor_get_current_selection(editor);
	int32_t expected_end = (int32_t)strlen(test_text);
	ENSURE(full_selection.start.offset == 0);
	ENSURE(full_selection.end.offset == expected_end);

	// Reset to end position and test Shift+Command+Up (should select from end to document start)
	skb_text_position_t end_pos = {.offset = expected_end, .affinity = SKB_AFFINITY_TRAILING};
	skb_text_range_t end_selection = {.start = end_pos, .end = end_pos};
	skb_editor_select(editor, end_selection);

	skb_text_range_t end_check = skb_editor_get_current_selection(editor);
	ENSURE(end_check.start.offset == expected_end);
	ENSURE(end_check.end.offset == expected_end);

	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_UP, SKB_MOD_SHIFT | SKB_MOD_COMMAND);

	skb_text_range_t reverse_selection = skb_editor_get_current_selection(editor);
	// Should select from end to start
	ENSURE((reverse_selection.start.offset == 0 && reverse_selection.end.offset == expected_end) ||
	       (reverse_selection.start.offset == expected_end && reverse_selection.end.offset == 0));

	// Reset to start position for line-level testing
	skb_text_position_t start_pos = {.offset = 0, .affinity = SKB_AFFINITY_TRAILING};
	skb_text_range_t start_selection = {.start = start_pos, .end = start_pos};
	skb_editor_select(editor, start_selection);

	skb_text_range_t reset_check = skb_editor_get_current_selection(editor);
	ENSURE(reset_check.start.offset == 0);
	ENSURE(reset_check.end.offset == 0);

	// Test Shift+Command+Right (should select from start to end of line)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_RIGHT, SKB_MOD_SHIFT | SKB_MOD_COMMAND);

	skb_text_range_t line_selection = skb_editor_get_current_selection(editor);
	// Should select from position 0 to end of first line (position 11, after "Hello world")
	ENSURE(line_selection.start.offset == 0);
	ENSURE(line_selection.end.offset == 11);

	// Test Shift+Command+Left (should select from current position to start of line)
	// First move to end of first line
	skb_text_position_t end_line_pos = {.offset = 11, .affinity = SKB_AFFINITY_TRAILING};
	skb_text_range_t end_line_selection = {.start = end_line_pos, .end = end_line_pos};
	skb_editor_select(editor, end_line_selection);

	skb_text_range_t end_line_check = skb_editor_get_current_selection(editor);
	ENSURE(end_line_check.start.offset == 11);
	ENSURE(end_line_check.end.offset == 11);

	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_LEFT, SKB_MOD_SHIFT | SKB_MOD_COMMAND);

	skb_text_range_t reverse_line_selection = skb_editor_get_current_selection(editor);
	// Should select from position 11 back to position 0
	ENSURE((reverse_line_selection.start.offset == 0 && reverse_line_selection.end.offset == 11) ||
	       (reverse_line_selection.start.offset == 11 && reverse_line_selection.end.offset == 0));

	skb_editor_destroy(editor);
	skb_font_collection_destroy(font_collection);
	skb_temp_alloc_destroy(temp_alloc);

	return 0;
}

static int test_option_word_navigation_macos(void)
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

	skb_editor_params_t params = {
		.font_collection = font_collection,
		.caret_mode = SKB_CARET_MODE_SKRIBIDI,
		.editor_behavior = SKB_BEHAVIOR_MACOS,
		.paragraph_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
	};

	skb_editor_t* editor = skb_editor_create(&params);
	ENSURE(editor != NULL);

	// Initialize editor with test text
	const char* test_text = "Hello world\nThis is a test\nabout line jumping";
	skb_editor_set_text_utf8(editor, temp_alloc, test_text, (int32_t)strlen(test_text));

	// Get initial selection - should be at document start
	skb_text_range_t initial_selection = skb_editor_get_current_selection(editor);
	ENSURE(initial_selection.start.offset == 0);
	ENSURE(initial_selection.end.offset == 0);

	// Test Option+Right (should jump to next word boundary)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_RIGHT, SKB_MOD_OPTION);

	skb_text_range_t after_first_jump = skb_editor_get_current_selection(editor);
	// Should move to next word boundary (position depends on word boundary algorithm)
	ENSURE(after_first_jump.start.offset > 0);
	ENSURE(after_first_jump.start.offset == after_first_jump.end.offset);
	int32_t first_word_end = after_first_jump.start.offset;

	// Test Option+Right again (should jump to next word)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_RIGHT, SKB_MOD_OPTION);

	skb_text_range_t after_second_jump = skb_editor_get_current_selection(editor);
	ENSURE(after_second_jump.start.offset > first_word_end);
	ENSURE(after_second_jump.start.offset == after_second_jump.end.offset);
	int32_t second_word_end = after_second_jump.start.offset;

	// Test Option+Left (should jump back to previous word boundary)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_LEFT, SKB_MOD_OPTION);

	skb_text_range_t after_left_jump = skb_editor_get_current_selection(editor);
	ENSURE(after_left_jump.start.offset < second_word_end);
	ENSURE(after_left_jump.start.offset == after_left_jump.end.offset);

	// Test Option+Left again (should jump back further)
	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_LEFT, SKB_MOD_OPTION);

	skb_text_range_t after_second_left = skb_editor_get_current_selection(editor);
	ENSURE(after_second_left.start.offset < after_left_jump.start.offset);
	ENSURE(after_second_left.start.offset == after_second_left.end.offset);

	// Reset to start and test Shift+Option+Right (should select word)
	skb_text_position_t start_pos = {.offset = 0, .affinity = SKB_AFFINITY_TRAILING};
	skb_text_range_t start_selection = {.start = start_pos, .end = start_pos};
	skb_editor_select(editor, start_selection);

	skb_text_range_t reset_check = skb_editor_get_current_selection(editor);
	ENSURE(reset_check.start.offset == 0);
	ENSURE(reset_check.end.offset == 0);

	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_RIGHT, SKB_MOD_SHIFT | SKB_MOD_OPTION);

	skb_text_range_t word_selection = skb_editor_get_current_selection(editor);
	// Should have a selection that spans some text
	ENSURE(word_selection.start.offset == 0);
	ENSURE(word_selection.end.offset > 0);

	// Test Shift+Option+Left from a position in the middle
	// Move to middle of text first
	skb_text_position_t middle_pos = {.offset = 20, .affinity = SKB_AFFINITY_TRAILING};
	skb_text_range_t middle_selection = {.start = middle_pos, .end = middle_pos};
	skb_editor_select(editor, middle_selection);

	skb_text_range_t middle_check = skb_editor_get_current_selection(editor);
	ENSURE(middle_check.start.offset == 20);
	ENSURE(middle_check.end.offset == 20);

	skb_temp_alloc_reset(temp_alloc);
	skb_editor_process_key_pressed(editor, temp_alloc, SKB_KEY_LEFT, SKB_MOD_SHIFT | SKB_MOD_OPTION);

	skb_text_range_t reverse_word_selection = skb_editor_get_current_selection(editor);
	// Should have a selection going backwards
	ENSURE(reverse_word_selection.start.offset != reverse_word_selection.end.offset);
	ENSURE((reverse_word_selection.start.offset == 20 && reverse_word_selection.end.offset < 20) ||
	       (reverse_word_selection.start.offset < 20 && reverse_word_selection.end.offset == 20));

	skb_editor_destroy(editor);
	skb_font_collection_destroy(font_collection);
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
	skb_editor_params_t params = {
		.font_collection = font_collection,
		.caret_mode = SKB_CARET_MODE_SKRIBIDI,
		.paragraph_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
	};

	skb_editor_t* editor = skb_editor_create(&params);
	ENSURE(editor != NULL);

	const char* text = "Hello world";
	skb_editor_set_text_utf8(editor, temp_alloc, text, (int32_t)strlen(text));

	skb_caret_info_t caret_info = skb_editor_get_caret_info_at(editor, SKB_CURRENT_SELECTION_END);
	skb_text_position_t hit_position = skb_editor_hit_test(editor, SKB_MOVEMENT_CARET, caret_info.x, caret_info.y);
	ENSURE(hit_position.offset == 0);

	// A double click at the first caret invokes the editor word-start path.
	skb_editor_process_mouse_click(editor, caret_info.x, caret_info.y, 0, 1.0);
	skb_editor_process_mouse_click(editor, caret_info.x, caret_info.y, 0, 1.1);
	skb_text_range_t selection = skb_editor_get_current_selection(editor);
	ENSURE(selection.start.offset == 0);

	skb_editor_destroy(editor);
	skb_font_collection_destroy(font_collection);
	skb_temp_alloc_destroy(temp_alloc);

	return 0;
}

static int test_edit_transaction(void)
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
	skb_editor_params_t params = {
		.font_collection = font_collection,
		.caret_mode = SKB_CARET_MODE_SKRIBIDI,
		.paragraph_attributes = SKB_ATTRIBUTE_SET_FROM_STATIC_ARRAY(attributes),
	};

	skb_editor_t* editor = skb_editor_create(&params);
	ENSURE(editor != NULL);
	skb_editor_set_text_utf8(editor, temp_alloc, "hello", -1);

	edit_callback_state_t callback_state = {0};
	skb_editor_set_on_text_change_callback(editor, on_edit_text_change, &callback_state);

	skb_text_t* replacement_text = skb_text_create();
	ENSURE(replacement_text != NULL);
	skb_text_append_utf8(replacement_text, "i", 1, (skb_attribute_set_t){0});

	skb_edit_transaction_t transaction = {
		.replacement = {
			.start = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
			.end = {.offset = 3, .affinity = SKB_AFFINITY_TRAILING},
		},
		.replacement_text = replacement_text,
		.resulting_selection = {
			.anchor = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
			.focus = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
		},
		.history_kind = SKB_EDIT_HISTORY_TYPING,
	};
	ENSURE(skb_editor_apply_transaction(editor, temp_alloc, &transaction) == SKB_RESULT_SUCCESS);
	ENSURE(callback_state.text_change_count == 1);

	char text[32] = {0};
	skb_editor_get_text_utf8(editor, text, (int32_t)sizeof(text));
	ENSURE(strcmp(text, "hilo") == 0);
	skb_selection_t selection = skb_editor_get_selection(editor);
	ENSURE(selection.anchor.offset == 2 && selection.focus.offset == 2);

	// Direction must survive the new API, including when the legacy range
	// representation is used by the existing editor implementation.
	skb_editor_set_selection(editor, (skb_selection_t){
		.anchor = {.offset = 3, .affinity = SKB_AFFINITY_TRAILING},
		.focus = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
	});
	selection = skb_editor_get_selection(editor);
	ENSURE(selection.anchor.offset == 3 && selection.focus.offset == 1);

	skb_text_reset(replacement_text);
	skb_text_append_utf8(replacement_text, "X", 1, (skb_attribute_set_t){0});
	transaction.replacement = SKB_CURRENT_SELECTION;
	transaction.replacement_text = replacement_text;
	transaction.resulting_selection = (skb_selection_t){
		.anchor = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
		.focus = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
	};
	transaction.history_kind = SKB_EDIT_HISTORY_PASTE;
	ENSURE(skb_editor_apply_transaction(editor, temp_alloc, &transaction) == SKB_RESULT_SUCCESS);
	memset(text, 0, sizeof(text));
	skb_editor_get_text_utf8(editor, text, (int32_t)sizeof(text));
	ENSURE(strcmp(text, "hXo") == 0);
	ENSURE(skb_editor_get_selection(editor).anchor.offset == 2);

	skb_temp_alloc_reset(temp_alloc);
	skb_editor_undo(editor, temp_alloc);
	memset(text, 0, sizeof(text));
	skb_editor_get_text_utf8(editor, text, (int32_t)sizeof(text));
	ENSURE(strcmp(text, "hilo") == 0);
	selection = skb_editor_get_selection(editor);
	ENSURE(selection.anchor.offset == 3 && selection.focus.offset == 1);

	transaction.replacement = (skb_text_range_t){
		.start = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
		.end = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
	};
	transaction.replacement_text = NULL;
	transaction.resulting_selection = (skb_selection_t){
		.anchor = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
		.focus = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
	};
	transaction.history_kind = SKB_EDIT_HISTORY_DELETE_BACKWARD;
	ENSURE(skb_editor_apply_transaction(editor, temp_alloc, &transaction) == SKB_RESULT_SUCCESS);
	memset(text, 0, sizeof(text));
	skb_editor_get_text_utf8(editor, text, (int32_t)sizeof(text));
	ENSURE(strcmp(text, "hlo") == 0);

	skb_temp_alloc_reset(temp_alloc);
	skb_editor_undo(editor, temp_alloc);
	memset(text, 0, sizeof(text));
	skb_editor_get_text_utf8(editor, text, (int32_t)sizeof(text));
	ENSURE(strcmp(text, "hilo") == 0);

	skb_text_reset(replacement_text);
	skb_text_append_utf8(replacement_text, "日", -1, (skb_attribute_set_t){0});
	transaction.replacement = (skb_text_range_t){
		.start = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
		.end = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
	};
	transaction.replacement_text = replacement_text;
	transaction.resulting_selection = (skb_selection_t){
		.anchor = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
		.focus = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
	};
	transaction.has_composition = true;
	transaction.composition_range = (skb_text_range_t){
		.start = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
		.end = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
	};
	transaction.history_kind = SKB_EDIT_HISTORY_COMPOSITION;
	ENSURE(skb_editor_apply_transaction(editor, temp_alloc, &transaction) == SKB_RESULT_SUCCESS);
	memset(text, 0, sizeof(text));
	skb_editor_get_text_utf8(editor, text, (int32_t)sizeof(text));
	ENSURE(strcmp(text, "h日ilo") == 0);
	ENSURE(skb_editor_has_composition(editor));
	skb_text_range_t composition = skb_editor_get_composition(editor);
	ENSURE(composition.start.offset == 1 && composition.end.offset == 2);

	// Composition updates are one transient session. Cancelling restores the
	// text and selection from before the first update and does not leave redo.
	skb_text_reset(replacement_text);
	skb_text_append_utf8(replacement_text, "日本", -1, (skb_attribute_set_t){0});
	transaction.replacement = (skb_text_range_t){
		.start = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
		.end = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
	};
	transaction.replacement_text = replacement_text;
	transaction.resulting_selection = (skb_selection_t){
		.anchor = {.offset = 3, .affinity = SKB_AFFINITY_TRAILING},
		.focus = {.offset = 3, .affinity = SKB_AFFINITY_TRAILING},
	};
	transaction.composition_range = (skb_text_range_t){
		.start = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
		.end = {.offset = 3, .affinity = SKB_AFFINITY_TRAILING},
	};
	ENSURE(skb_editor_apply_transaction(editor, temp_alloc, &transaction) == SKB_RESULT_SUCCESS);
	memset(text, 0, sizeof(text));
	skb_editor_get_text_utf8(editor, text, (int32_t)sizeof(text));
	ENSURE(strcmp(text, "h日本ilo") == 0);
	composition = skb_editor_get_composition(editor);
	ENSURE(composition.start.offset == 1 && composition.end.offset == 3);

	ENSURE(skb_editor_cancel_composition(editor, temp_alloc));
	memset(text, 0, sizeof(text));
	skb_editor_get_text_utf8(editor, text, (int32_t)sizeof(text));
	ENSURE(strcmp(text, "hilo") == 0 && !skb_editor_has_composition(editor));
	selection = skb_editor_get_selection(editor);
	ENSURE(selection.anchor.offset == 3 && selection.focus.offset == 1);
	ENSURE(!skb_editor_can_redo(editor));

	// A committed composition is one undo step, regardless of how many
	// preedit replacements it received.
	skb_editor_set_selection(editor, (skb_selection_t){
		.anchor = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
		.focus = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
	});
	skb_text_reset(replacement_text);
	skb_text_append_utf8(replacement_text, "日", -1, (skb_attribute_set_t){0});
	transaction.replacement = (skb_text_range_t){
		.start = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
		.end = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
	};
	transaction.replacement_text = replacement_text;
	transaction.resulting_selection = (skb_selection_t){
		.anchor = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
		.focus = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
	};
	transaction.composition_range = (skb_text_range_t){
		.start = {.offset = 1, .affinity = SKB_AFFINITY_TRAILING},
		.end = {.offset = 2, .affinity = SKB_AFFINITY_TRAILING},
	};
	ENSURE(skb_editor_apply_transaction(editor, temp_alloc, &transaction) == SKB_RESULT_SUCCESS);
	ENSURE(skb_editor_commit_composition(editor));
	ENSURE(!skb_editor_has_composition(editor));

	skb_temp_alloc_reset(temp_alloc);
	skb_editor_undo(editor, temp_alloc);
	memset(text, 0, sizeof(text));
	skb_editor_get_text_utf8(editor, text, (int32_t)sizeof(text));
	ENSURE(strcmp(text, "hilo") == 0 && !skb_editor_has_composition(editor));

	skb_temp_alloc_reset(temp_alloc);
	skb_editor_redo(editor, temp_alloc);
	memset(text, 0, sizeof(text));
	skb_editor_get_text_utf8(editor, text, (int32_t)sizeof(text));
	ENSURE(strcmp(text, "h日ilo") == 0 && !skb_editor_has_composition(editor));

	transaction.replacement = (skb_text_range_t){
		.start = {.offset = 100, .affinity = SKB_AFFINITY_TRAILING},
		.end = {.offset = 100, .affinity = SKB_AFFINITY_TRAILING},
	};
	ENSURE(skb_editor_apply_transaction(editor, temp_alloc, &transaction) == SKB_RESULT_INVALID_RANGE);
	memset(text, 0, sizeof(text));
	skb_editor_get_text_utf8(editor, text, (int32_t)sizeof(text));
	ENSURE(strcmp(text, "h日ilo") == 0);

	skb_text_destroy(replacement_text);
	skb_editor_destroy(editor);
	skb_font_collection_destroy(font_collection);
	skb_temp_alloc_destroy(temp_alloc);
	return 0;
}

int editor_tests(void)
{
	RUN_SUBTEST(test_init);
	RUN_SUBTEST(test_command_line_navigation_macos);
	RUN_SUBTEST(test_command_document_navigation_macos);
	RUN_SUBTEST(test_shift_command_text_selection_macos);
	RUN_SUBTEST(test_option_word_navigation_macos);
	RUN_SUBTEST(test_word_start_at_document_start);
	RUN_SUBTEST(test_edit_transaction);
	return 0;
}
