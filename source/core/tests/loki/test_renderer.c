/**
 * @file test_renderer.c
 * @brief Unit tests for the renderer module.
 *
 * Tests both terminal renderer (VT100 escape sequences) and null renderer.
 */

#include "test_framework.h"
#include "loki/renderer.h"
#include "loki/hl_types.h"
#include "loki/internal.h"
#include "loki/terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * Test Helpers
 *============================================================================*/

/* Access terminal renderer's internal buffer for verification */
typedef struct {
    struct abuf ab;
    int cols;
    int rows;
} TerminalRendererData;

/* Get the output buffer from terminal renderer */
static const char *get_renderer_output(Renderer *r, int *len) {
    if (!r || !r->data) return NULL;
    TerminalRendererData *data = (TerminalRendererData *)r->data;
    if (len) *len = data->ab.len;
    return data->ab.b;
}

/* Check if buffer contains a substring */
static int buffer_contains(const char *buf, int buflen, const char *substr) {
    if (!buf || !substr) return 0;
    int sublen = strlen(substr);
    if (sublen > buflen) return 0;

    for (int i = 0; i <= buflen - sublen; i++) {
        if (memcmp(buf + i, substr, sublen) == 0) return 1;
    }
    return 0;
}

/* Create minimal StatusInfo */
static StatusInfo create_status_info(void) {
    StatusInfo info = {0};
    info.mode = "NORMAL";
    info.filename = "test.txt";
    info.lang = "Text";
    info.numrows = 100;
    info.current_row = 42;
    info.dirty = 0;
    info.playing = 0;
    info.link_active = 0;
    info.metronome_active = 0;
    info.transport_armed = 0;
    info.tempo = 120.0;
    info.beat = 0.0;
    info.bar = 1;
    info.beat_in_bar = 1;
    info.link_peers = 0;
    return info;
}

/* Create minimal ReplInfo */
static ReplInfo create_repl_info(void) {
    ReplInfo info = {0};
    info.prompt = "> ";
    info.input = "test input";
    info.input_len = 10;
    info.log_lines = NULL;
    info.log_count = 0;
    info.max_display_lines = 5;
    return info;
}

/* Create minimal PickerInfo */
static PickerInfo create_picker_info(const char **items, int count) {
    PickerInfo info = {0};
    info.title = "Test Picker";
    info.items = items;
    info.item_count = count;
    info.selected_index = 0;
    info.scroll_offset = 0;
    info.visible_rows = count < 10 ? count : 10;
    return info;
}

/*============================================================================
 * hl_const_to_type Tests
 *============================================================================*/

TEST(hl_const_to_type_normal) {
    ASSERT_EQ(hl_const_to_type(HL_NORMAL), HL_TYPE_NORMAL);
}

TEST(hl_const_to_type_comment) {
    ASSERT_EQ(hl_const_to_type(HL_COMMENT), HL_TYPE_COMMENT);
}

TEST(hl_const_to_type_keyword1) {
    ASSERT_EQ(hl_const_to_type(HL_KEYWORD1), HL_TYPE_KEYWORD1);
}

TEST(hl_const_to_type_keyword2) {
    ASSERT_EQ(hl_const_to_type(HL_KEYWORD2), HL_TYPE_KEYWORD2);
}

TEST(hl_const_to_type_string) {
    ASSERT_EQ(hl_const_to_type(HL_STRING), HL_TYPE_STRING);
}

TEST(hl_const_to_type_number) {
    ASSERT_EQ(hl_const_to_type(HL_NUMBER), HL_TYPE_NUMBER);
}

TEST(hl_const_to_type_match) {
    ASSERT_EQ(hl_const_to_type(HL_MATCH), HL_TYPE_MATCH);
}

TEST(hl_const_to_type_nonprint) {
    ASSERT_EQ(hl_const_to_type(HL_NONPRINT), HL_TYPE_NONPRINT);
}

TEST(hl_const_to_type_unknown_returns_normal) {
    ASSERT_EQ(hl_const_to_type(999), HL_TYPE_NORMAL);
    ASSERT_EQ(hl_const_to_type(-1), HL_TYPE_NORMAL);
}

/*============================================================================
 * Terminal Renderer Creation Tests
 *============================================================================*/

TEST(terminal_renderer_create_not_null) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);
    r->destroy(r);
}

TEST(terminal_renderer_has_all_callbacks) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    ASSERT_NOT_NULL(r->begin_frame);
    ASSERT_NOT_NULL(r->end_frame);
    ASSERT_NOT_NULL(r->render_tabs);
    ASSERT_NOT_NULL(r->render_row);
    ASSERT_NOT_NULL(r->render_status);
    ASSERT_NOT_NULL(r->render_message);
    ASSERT_NOT_NULL(r->render_repl);
    ASSERT_NOT_NULL(r->render_picker);
    ASSERT_NOT_NULL(r->set_cursor);
    ASSERT_NOT_NULL(r->show_cursor);
    ASSERT_NOT_NULL(r->hide_cursor);
    ASSERT_NOT_NULL(r->clipboard_copy);
    ASSERT_NOT_NULL(r->destroy);

    r->destroy(r);
}

TEST(terminal_renderer_has_data) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);
    ASSERT_NOT_NULL(r->data);
    r->destroy(r);
}

/*============================================================================
 * Null Renderer Creation Tests
 *============================================================================*/

TEST(null_renderer_create_not_null) {
    Renderer *r = null_renderer_create();
    ASSERT_NOT_NULL(r);
    r->destroy(r);
}

TEST(null_renderer_has_all_callbacks) {
    Renderer *r = null_renderer_create();
    ASSERT_NOT_NULL(r);

    ASSERT_NOT_NULL(r->begin_frame);
    ASSERT_NOT_NULL(r->end_frame);
    ASSERT_NOT_NULL(r->render_tabs);
    ASSERT_NOT_NULL(r->render_row);
    ASSERT_NOT_NULL(r->render_status);
    ASSERT_NOT_NULL(r->render_message);
    ASSERT_NOT_NULL(r->render_repl);
    ASSERT_NOT_NULL(r->render_picker);
    ASSERT_NOT_NULL(r->set_cursor);
    ASSERT_NOT_NULL(r->show_cursor);
    ASSERT_NOT_NULL(r->hide_cursor);
    ASSERT_NOT_NULL(r->clipboard_copy);
    ASSERT_NOT_NULL(r->destroy);

    r->destroy(r);
}

TEST(null_renderer_data_is_null) {
    Renderer *r = null_renderer_create();
    ASSERT_NOT_NULL(r);
    ASSERT_NULL(r->data);
    r->destroy(r);
}

/*============================================================================
 * Terminal Renderer Begin/End Frame Tests
 *============================================================================*/

TEST(terminal_begin_frame_hides_cursor) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    int len;
    const char *buf = get_renderer_output(r, &len);
    /* Check for hide cursor sequence: ESC[?25l */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[?25l"));

    r->destroy(r);
}

TEST(terminal_begin_frame_goes_home) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    int len;
    const char *buf = get_renderer_output(r, &len);
    /* Check for go home sequence: ESC[H */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[H"));

    r->destroy(r);
}

/*============================================================================
 * Terminal Renderer Cursor Tests
 *============================================================================*/

TEST(terminal_set_cursor_generates_sequence) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);
    r->set_cursor(r, 10, 20);

    int len;
    const char *buf = get_renderer_output(r, &len);
    /* Check for cursor position sequence: ESC[10;20H */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[10;20H"));

    r->destroy(r);
}

TEST(terminal_show_cursor_generates_sequence) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);
    r->show_cursor(r);

    int len;
    const char *buf = get_renderer_output(r, &len);
    /* Check for show cursor sequence: ESC[?25h */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[?25h"));

    r->destroy(r);
}

TEST(terminal_hide_cursor_generates_sequence) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);
    r->hide_cursor(r);

    int len;
    const char *buf = get_renderer_output(r, &len);
    /* Count occurrences of hide cursor (begin_frame + hide_cursor) */
    int count = 0;
    for (int i = 0; i < len - 5; i++) {
        if (memcmp(buf + i, "\x1b[?25l", 6) == 0) count++;
    }
    ASSERT_GTE(count, 2);  /* At least 2: one from begin_frame, one from hide_cursor */

    r->destroy(r);
}

/*============================================================================
 * Terminal Renderer Tab Tests
 *============================================================================*/

TEST(terminal_render_tabs_single_tab_does_nothing) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);
    int len_before;
    get_renderer_output(r, &len_before);

    const char *tabs[] = {"file.txt"};
    r->render_tabs(r, tabs, 1, 0, 80);

    int len_after;
    get_renderer_output(r, &len_after);

    /* Single tab should not render tab bar */
    ASSERT_EQ(len_before, len_after);

    r->destroy(r);
}

TEST(terminal_render_tabs_multiple_tabs) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    const char *tabs[] = {"file1.txt", "file2.txt", "file3.txt"};
    r->render_tabs(r, tabs, 3, 1, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    /* Should contain reverse video for tab bar */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[7m"));
    /* Should contain tab names */
    ASSERT_TRUE(buffer_contains(buf, len, "file1"));
    ASSERT_TRUE(buffer_contains(buf, len, "file2"));
    ASSERT_TRUE(buffer_contains(buf, len, "file3"));
    /* Should have separators */
    ASSERT_TRUE(buffer_contains(buf, len, "|"));

    r->destroy(r);
}

TEST(terminal_render_tabs_active_tab_bold) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    const char *tabs[] = {"file1.txt", "file2.txt"};
    r->render_tabs(r, tabs, 2, 0, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    /* Active tab should use bold: ESC[1m */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[1m"));

    r->destroy(r);
}

/*============================================================================
 * Terminal Renderer Row Tests
 *============================================================================*/

TEST(terminal_render_row_empty) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);
    r->render_row(r, 1, NULL, 0, 4, 1);  /* is_empty = 1 */

    int len;
    const char *buf = get_renderer_output(r, &len);

    /* Empty row should show tilde */
    ASSERT_TRUE(buffer_contains(buf, len, "~"));

    r->destroy(r);
}

TEST(terminal_render_row_with_line_number) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    RenderSegment seg = {.text = "hello", .len = 5, .hl_type = HL_TYPE_NORMAL, .selected = 0, .is_playing = 0};
    r->render_row(r, 42, &seg, 1, 4, 0);

    int len;
    const char *buf = get_renderer_output(r, &len);

    /* Should contain line number */
    ASSERT_TRUE(buffer_contains(buf, len, "42"));
    /* Should contain text */
    ASSERT_TRUE(buffer_contains(buf, len, "hello"));

    r->destroy(r);
}

TEST(terminal_render_row_with_selection) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    RenderSegment segs[2] = {
        {.text = "normal", .len = 6, .hl_type = HL_TYPE_NORMAL, .selected = 0, .is_playing = 0},
        {.text = "selected", .len = 8, .hl_type = HL_TYPE_NORMAL, .selected = 1, .is_playing = 0}
    };
    r->render_row(r, 1, segs, 2, 0, 0);

    int len;
    const char *buf = get_renderer_output(r, &len);

    /* Should contain reverse video for selection: ESC[7m */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[7m"));

    r->destroy(r);
}

TEST(terminal_render_row_playing_line) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    RenderSegment seg = {.text = "playing", .len = 7, .hl_type = HL_TYPE_NORMAL, .selected = 0, .is_playing = 1};
    r->render_row(r, 5, &seg, 1, 4, 0);

    int len;
    const char *buf = get_renderer_output(r, &len);

    /* Should contain bright green for playing line: ESC[92m */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[92m"));
    /* Should contain play indicator */
    ASSERT_TRUE(buffer_contains(buf, len, ">"));
    /* Should contain green background: ESC[48;5;22m */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[48;5;22m"));

    r->destroy(r);
}

TEST(terminal_render_row_syntax_colors) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    RenderSegment segs[] = {
        {.text = "comment", .len = 7, .hl_type = HL_TYPE_COMMENT, .selected = 0, .is_playing = 0},
        {.text = "keyword", .len = 7, .hl_type = HL_TYPE_KEYWORD1, .selected = 0, .is_playing = 0},
        {.text = "string", .len = 6, .hl_type = HL_TYPE_STRING, .selected = 0, .is_playing = 0},
    };
    r->render_row(r, 1, segs, 3, 0, 0);

    int len;
    const char *buf = get_renderer_output(r, &len);

    /* Comment: gray ESC[90m */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[90m"));
    /* Keyword1: yellow ESC[33m */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[33m"));
    /* String: cyan ESC[36m */
    ASSERT_TRUE(buffer_contains(buf, len, "\x1b[36m"));

    r->destroy(r);
}

/*============================================================================
 * Terminal Renderer Status Tests
 *============================================================================*/

TEST(terminal_render_status_contains_mode) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    StatusInfo info = create_status_info();
    r->render_status(r, &info, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "NORMAL"));

    r->destroy(r);
}

TEST(terminal_render_status_contains_filename) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    StatusInfo info = create_status_info();
    r->render_status(r, &info, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "test.txt"));

    r->destroy(r);
}

TEST(terminal_render_status_shows_modified) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    StatusInfo info = create_status_info();
    info.dirty = 1;
    r->render_status(r, &info, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "modified"));

    r->destroy(r);
}

TEST(terminal_render_status_shows_playing) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    StatusInfo info = create_status_info();
    info.playing = 1;
    r->render_status(r, &info, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "[PLAYING]"));

    r->destroy(r);
}

TEST(terminal_render_status_shows_metronome) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    StatusInfo info = create_status_info();
    info.metronome_active = 1;
    r->render_status(r, &info, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "[METRONOME]"));

    r->destroy(r);
}

TEST(terminal_render_status_shows_armed) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    StatusInfo info = create_status_info();
    info.transport_armed = 1;
    r->render_status(r, &info, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "[ARMED]"));

    r->destroy(r);
}

TEST(terminal_render_status_shows_link_peers) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    StatusInfo info = create_status_info();
    info.link_active = 1;
    info.link_peers = 3;
    r->render_status(r, &info, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "[3P]"));

    r->destroy(r);
}

TEST(terminal_render_status_shows_tempo) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    StatusInfo info = create_status_info();
    info.playing = 1;
    info.tempo = 140.0;
    r->render_status(r, &info, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "140BPM"));

    r->destroy(r);
}

/*============================================================================
 * Terminal Renderer Message Tests
 *============================================================================*/

TEST(terminal_render_message_shows_text) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);
    r->render_message(r, "Hello, World!", 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "Hello, World!"));

    r->destroy(r);
}

TEST(terminal_render_message_null_safe) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);
    r->render_message(r, NULL, 80);

    /* Should not crash */
    ASSERT_TRUE(1);

    r->destroy(r);
}

TEST(terminal_render_message_truncates) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 10, 24);  /* Narrow width */
    r->render_message(r, "This is a very long message", 10);

    int len;
    const char *buf = get_renderer_output(r, &len);

    /* Should contain truncated text */
    ASSERT_TRUE(buffer_contains(buf, len, "This is a "));
    /* Should not contain full text */
    ASSERT_FALSE(buffer_contains(buf, len, "very long message"));

    r->destroy(r);
}

/*============================================================================
 * Terminal Renderer REPL Tests
 *============================================================================*/

TEST(terminal_render_repl_shows_prompt) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    ReplInfo info = create_repl_info();
    r->render_repl(r, &info, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "> "));

    r->destroy(r);
}

TEST(terminal_render_repl_shows_input) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    ReplInfo info = create_repl_info();
    r->render_repl(r, &info, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "test input"));

    r->destroy(r);
}

TEST(terminal_render_repl_shows_log) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    const char *log[] = {"log line 1", "log line 2"};
    ReplInfo info = create_repl_info();
    info.log_lines = log;
    info.log_count = 2;

    r->render_repl(r, &info, 80);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "log line 1"));
    ASSERT_TRUE(buffer_contains(buf, len, "log line 2"));

    r->destroy(r);
}

/*============================================================================
 * Terminal Renderer Picker Tests
 *============================================================================*/

TEST(terminal_render_picker_shows_title) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    const char *items[] = {"item1", "item2"};
    PickerInfo info = create_picker_info(items, 2);
    r->render_picker(r, &info, 80, 24);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "Test Picker"));

    r->destroy(r);
}

TEST(terminal_render_picker_shows_items) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    const char *items[] = {"option_one", "option_two", "option_three"};
    PickerInfo info = create_picker_info(items, 3);
    r->render_picker(r, &info, 80, 24);

    int len;
    const char *buf = get_renderer_output(r, &len);

    ASSERT_TRUE(buffer_contains(buf, len, "option_one"));
    ASSERT_TRUE(buffer_contains(buf, len, "option_two"));
    ASSERT_TRUE(buffer_contains(buf, len, "option_three"));

    r->destroy(r);
}

TEST(terminal_render_picker_shows_selection_indicator) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    const char *items[] = {"item1", "item2"};
    PickerInfo info = create_picker_info(items, 2);
    info.selected_index = 0;
    r->render_picker(r, &info, 80, 24);

    int len;
    const char *buf = get_renderer_output(r, &len);

    /* Selection uses > indicator */
    ASSERT_TRUE(buffer_contains(buf, len, "> "));

    r->destroy(r);
}

TEST(terminal_render_picker_shows_help) {
    Renderer *r = terminal_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);

    const char *items[] = {"item1"};
    PickerInfo info = create_picker_info(items, 1);
    r->render_picker(r, &info, 80, 24);

    int len;
    const char *buf = get_renderer_output(r, &len);

    /* Should show help text */
    ASSERT_TRUE(buffer_contains(buf, len, "j/k:move"));
    ASSERT_TRUE(buffer_contains(buf, len, "ENTER:select"));
    ASSERT_TRUE(buffer_contains(buf, len, "ESC:cancel"));

    r->destroy(r);
}

/*============================================================================
 * Null Renderer Tests
 *============================================================================*/

TEST(null_renderer_begin_frame_does_nothing) {
    Renderer *r = null_renderer_create();
    ASSERT_NOT_NULL(r);

    /* Should not crash */
    r->begin_frame(r, 80, 24);

    r->destroy(r);
}

TEST(null_renderer_end_frame_does_nothing) {
    Renderer *r = null_renderer_create();
    ASSERT_NOT_NULL(r);

    r->begin_frame(r, 80, 24);
    r->end_frame(r);

    /* Should not crash */
    ASSERT_TRUE(1);

    r->destroy(r);
}

TEST(null_renderer_render_row_does_nothing) {
    Renderer *r = null_renderer_create();
    ASSERT_NOT_NULL(r);

    RenderSegment seg = {.text = "test", .len = 4, .hl_type = HL_TYPE_NORMAL, .selected = 0, .is_playing = 0};
    r->render_row(r, 1, &seg, 1, 4, 0);

    /* Should not crash */
    ASSERT_TRUE(1);

    r->destroy(r);
}

TEST(null_renderer_clipboard_copy_succeeds) {
    Renderer *r = null_renderer_create();
    ASSERT_NOT_NULL(r);

    int result = r->clipboard_copy(r, "test", 4);
    ASSERT_EQ(result, 0);

    r->destroy(r);
}

/*============================================================================
 * Main Test Runner
 *============================================================================*/

BEGIN_TEST_SUITE("Renderer Tests")
    /* hl_const_to_type */
    RUN_TEST(hl_const_to_type_normal);
    RUN_TEST(hl_const_to_type_comment);
    RUN_TEST(hl_const_to_type_keyword1);
    RUN_TEST(hl_const_to_type_keyword2);
    RUN_TEST(hl_const_to_type_string);
    RUN_TEST(hl_const_to_type_number);
    RUN_TEST(hl_const_to_type_match);
    RUN_TEST(hl_const_to_type_nonprint);
    RUN_TEST(hl_const_to_type_unknown_returns_normal);

    /* Terminal renderer creation */
    RUN_TEST(terminal_renderer_create_not_null);
    RUN_TEST(terminal_renderer_has_all_callbacks);
    RUN_TEST(terminal_renderer_has_data);

    /* Null renderer creation */
    RUN_TEST(null_renderer_create_not_null);
    RUN_TEST(null_renderer_has_all_callbacks);
    RUN_TEST(null_renderer_data_is_null);

    /* Terminal begin/end frame */
    RUN_TEST(terminal_begin_frame_hides_cursor);
    RUN_TEST(terminal_begin_frame_goes_home);

    /* Terminal cursor */
    RUN_TEST(terminal_set_cursor_generates_sequence);
    RUN_TEST(terminal_show_cursor_generates_sequence);
    RUN_TEST(terminal_hide_cursor_generates_sequence);

    /* Terminal tabs */
    RUN_TEST(terminal_render_tabs_single_tab_does_nothing);
    RUN_TEST(terminal_render_tabs_multiple_tabs);
    RUN_TEST(terminal_render_tabs_active_tab_bold);

    /* Terminal row */
    RUN_TEST(terminal_render_row_empty);
    RUN_TEST(terminal_render_row_with_line_number);
    RUN_TEST(terminal_render_row_with_selection);
    RUN_TEST(terminal_render_row_playing_line);
    RUN_TEST(terminal_render_row_syntax_colors);

    /* Terminal status */
    RUN_TEST(terminal_render_status_contains_mode);
    RUN_TEST(terminal_render_status_contains_filename);
    RUN_TEST(terminal_render_status_shows_modified);
    RUN_TEST(terminal_render_status_shows_playing);
    RUN_TEST(terminal_render_status_shows_metronome);
    RUN_TEST(terminal_render_status_shows_armed);
    RUN_TEST(terminal_render_status_shows_link_peers);
    RUN_TEST(terminal_render_status_shows_tempo);

    /* Terminal message */
    RUN_TEST(terminal_render_message_shows_text);
    RUN_TEST(terminal_render_message_null_safe);
    RUN_TEST(terminal_render_message_truncates);

    /* Terminal REPL */
    RUN_TEST(terminal_render_repl_shows_prompt);
    RUN_TEST(terminal_render_repl_shows_input);
    RUN_TEST(terminal_render_repl_shows_log);

    /* Terminal picker */
    RUN_TEST(terminal_render_picker_shows_title);
    RUN_TEST(terminal_render_picker_shows_items);
    RUN_TEST(terminal_render_picker_shows_selection_indicator);
    RUN_TEST(terminal_render_picker_shows_help);

    /* Null renderer */
    RUN_TEST(null_renderer_begin_frame_does_nothing);
    RUN_TEST(null_renderer_end_frame_does_nothing);
    RUN_TEST(null_renderer_render_row_does_nothing);
    RUN_TEST(null_renderer_clipboard_copy_succeeds);
END_TEST_SUITE()
