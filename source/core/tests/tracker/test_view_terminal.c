/**
 * test_view_terminal.c - Tests for tracker view terminal backend
 *
 * Tests the terminal rendering and input handling for the tracker view.
 * Uses the built-in testing support (render_to_string, inject_key) to
 * test without needing an actual terminal.
 */

#include "test_framework.h"
#include "tracker_view.h"
#include "tracker_view_terminal.h"
#include "tracker_model.h"
#include <stdlib.h>
#include <string.h>

/* Global test stats required by test framework */
test_stats_t test_stats;

/*============================================================================
 * Test Helpers
 *============================================================================*/

static TrackerSong* create_test_song(int num_rows, int num_tracks) {
    TrackerSong* song = tracker_song_new("Test Song");
    if (!song) return NULL;

    TrackerPattern* pattern = tracker_pattern_new(num_rows, num_tracks, "Pattern 1");
    if (!pattern) {
        tracker_song_free(song);
        return NULL;
    }

    tracker_song_add_pattern(song, pattern);
    return song;
}

static void set_test_expression(TrackerPattern* pattern, int row, int track, const char* expr) {
    TrackerCell* cell = tracker_pattern_get_cell(pattern, row, track);
    if (cell) {
        cell->type = TRACKER_CELL_EXPRESSION;
        free(cell->expression);
        cell->expression = strdup(expr);
    }
}

/*============================================================================
 * Config Initialization Tests
 *============================================================================*/

TEST(config_init_sets_defaults) {
    TrackerTerminalConfig config;
    memset(&config, 0xFF, sizeof(config));  /* Fill with garbage */

    tracker_terminal_config_init(&config);

    ASSERT_EQ(config.min_track_width, 10);
    ASSERT_EQ(config.max_track_width, 20);
    ASSERT_EQ(config.row_number_width, 4);
    ASSERT_TRUE(config.use_unicode_borders);
    ASSERT_TRUE(config.use_colors);
    ASSERT_TRUE(config.use_256_colors);
    ASSERT_TRUE(config.use_true_color);
    ASSERT_TRUE(config.alternate_screen);
    ASSERT_FALSE(config.mouse_support);
    ASSERT_EQ(config.frame_rate, 30);
}

TEST(config_init_multiple_calls_consistent) {
    TrackerTerminalConfig config1, config2;

    tracker_terminal_config_init(&config1);
    tracker_terminal_config_init(&config2);

    ASSERT_EQ(config1.min_track_width, config2.min_track_width);
    ASSERT_EQ(config1.max_track_width, config2.max_track_width);
    ASSERT_EQ(config1.frame_rate, config2.frame_rate);
}

/*============================================================================
 * View Creation Tests
 *============================================================================*/

TEST(terminal_new_creates_view) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    /* Verify callbacks are set */
    ASSERT_NOT_NULL(view->callbacks.render);
    ASSERT_NOT_NULL(view->callbacks.poll_input);
    ASSERT_NOT_NULL(view->callbacks.get_dimensions);
    ASSERT_NOT_NULL(view->callbacks.backend_data);

    tracker_view_free(view);
}

TEST(terminal_new_with_config_uses_config) {
    TrackerTerminalConfig config;
    tracker_terminal_config_init(&config);
    config.use_colors = false;
    config.use_unicode_borders = false;

    TrackerView* view = tracker_view_terminal_new_with_config(&config);
    ASSERT_NOT_NULL(view);

    /* Verify config was applied */
    ASSERT_FALSE(tracker_view_terminal_has_colors(view));

    tracker_view_free(view);
}

TEST(terminal_new_with_fds_creates_view) {
    /* Use invalid FDs - view creation should still succeed */
    TrackerView* view = tracker_view_terminal_new_with_fds(-1, -1);
    ASSERT_NOT_NULL(view);

    tracker_view_free(view);
}

TEST(terminal_view_free_null_safe) {
    /* This implicitly tests via tracker_view_free which handles NULL */
    tracker_view_free(NULL);
}

/*============================================================================
 * Color Support Tests
 *============================================================================*/

TEST(has_colors_returns_config_value) {
    TrackerTerminalConfig config;
    tracker_terminal_config_init(&config);

    /* Test with colors enabled */
    config.use_colors = true;
    TrackerView* view = tracker_view_terminal_new_with_config(&config);
    ASSERT_NOT_NULL(view);
    ASSERT_TRUE(tracker_view_terminal_has_colors(view));
    tracker_view_free(view);

    /* Test with colors disabled */
    config.use_colors = false;
    view = tracker_view_terminal_new_with_config(&config);
    ASSERT_NOT_NULL(view);
    ASSERT_FALSE(tracker_view_terminal_has_colors(view));
    tracker_view_free(view);
}

TEST(has_256_colors_returns_config_value) {
    TrackerTerminalConfig config;
    tracker_terminal_config_init(&config);

    config.use_256_colors = true;
    TrackerView* view = tracker_view_terminal_new_with_config(&config);
    ASSERT_NOT_NULL(view);
    ASSERT_TRUE(tracker_view_terminal_has_256_colors(view));
    tracker_view_free(view);

    config.use_256_colors = false;
    view = tracker_view_terminal_new_with_config(&config);
    ASSERT_NOT_NULL(view);
    ASSERT_FALSE(tracker_view_terminal_has_256_colors(view));
    tracker_view_free(view);
}

TEST(has_true_color_returns_config_value) {
    TrackerTerminalConfig config;
    tracker_terminal_config_init(&config);

    config.use_true_color = true;
    TrackerView* view = tracker_view_terminal_new_with_config(&config);
    ASSERT_NOT_NULL(view);
    ASSERT_TRUE(tracker_view_terminal_has_true_color(view));
    tracker_view_free(view);

    config.use_true_color = false;
    view = tracker_view_terminal_new_with_config(&config);
    ASSERT_NOT_NULL(view);
    ASSERT_FALSE(tracker_view_terminal_has_true_color(view));
    tracker_view_free(view);
}

/*============================================================================
 * Layout Tests
 *============================================================================*/

TEST(get_layout_returns_valid_layout) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    const TrackerTerminalLayout* layout = tracker_view_terminal_get_layout(view);
    ASSERT_NOT_NULL(layout);

    /* Basic sanity checks */
    ASSERT_TRUE(layout->header_rows >= 0);
    ASSERT_TRUE(layout->footer_rows >= 0);
    ASSERT_TRUE(layout->row_num_width > 0);

    tracker_view_free(view);
}

TEST(get_layout_with_song_calculates_tracks) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerSong* song = create_test_song(16, 4);
    ASSERT_NOT_NULL(song);

    tracker_view_attach(view, song, NULL);

    const TrackerTerminalLayout* layout = tracker_view_terminal_get_layout(view);
    ASSERT_NOT_NULL(layout);

    /* Should have calculated track widths */
    ASSERT_TRUE(layout->track_count > 0);
    ASSERT_NOT_NULL(layout->track_widths);

    tracker_view_free(view);
    tracker_song_free(song);
}

TEST(layout_respects_screen_dimensions) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    /* Render to specific dimensions to set layout */
    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);
    free(output);

    const TrackerTerminalLayout* layout = tracker_view_terminal_get_layout(view);
    ASSERT_NOT_NULL(layout);

    ASSERT_EQ(layout->screen_cols, 80);
    ASSERT_EQ(layout->screen_rows, 24);

    tracker_view_free(view);
}

/*============================================================================
 * String Rendering Tests
 *============================================================================*/

TEST(render_to_string_returns_output) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);
    ASSERT_TRUE(strlen(output) > 0);

    free(output);
    tracker_view_free(view);
}

TEST(render_to_string_contains_escape_sequences) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should contain VT100 escape sequences */
    ASSERT_NOT_NULL(strstr(output, "\x1b["));

    free(output);
    tracker_view_free(view);
}

TEST(render_to_string_with_song) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerSong* song = create_test_song(16, 4);
    ASSERT_NOT_NULL(song);

    tracker_view_attach(view, song, NULL);

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should contain pattern info */
    ASSERT_NOT_NULL(strstr(output, "Pattern"));

    free(output);
    tracker_view_free(view);
    tracker_song_free(song);
}

TEST(render_to_string_with_cell_content) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerSong* song = create_test_song(16, 4);
    ASSERT_NOT_NULL(song);

    TrackerPattern* pattern = tracker_song_get_pattern(song, 0);
    set_test_expression(pattern, 0, 0, "C4");

    tracker_view_attach(view, song, NULL);

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should contain the note */
    ASSERT_NOT_NULL(strstr(output, "C4"));

    free(output);
    tracker_view_free(view);
    tracker_song_free(song);
}

TEST(render_to_string_help_mode) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    view->state.view_mode = TRACKER_VIEW_MODE_HELP;

    char* output = tracker_view_terminal_render_to_string(view, 80, 40);
    ASSERT_NOT_NULL(output);

    /* Should contain help text */
    ASSERT_NOT_NULL(strstr(output, "HELP"));
    ASSERT_NOT_NULL(strstr(output, "NAVIGATION"));

    free(output);
    tracker_view_free(view);
}

TEST(render_to_string_arrange_mode) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerSong* song = create_test_song(16, 4);
    ASSERT_NOT_NULL(song);

    tracker_view_attach(view, song, NULL);
    view->state.view_mode = TRACKER_VIEW_MODE_ARRANGE;

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should contain arrange mode header */
    ASSERT_NOT_NULL(strstr(output, "ARRANGE"));

    free(output);
    tracker_view_free(view);
    tracker_song_free(song);
}

TEST(render_to_string_mixer_mode) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerSong* song = create_test_song(16, 4);
    ASSERT_NOT_NULL(song);

    tracker_view_attach(view, song, NULL);
    view->state.view_mode = TRACKER_VIEW_MODE_MIXER;

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should contain mixer mode header */
    ASSERT_NOT_NULL(strstr(output, "MIXER"));

    free(output);
    tracker_view_free(view);
    tracker_song_free(song);
}

TEST(render_to_string_fx_mode) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerSong* song = create_test_song(16, 4);
    ASSERT_NOT_NULL(song);

    tracker_view_attach(view, song, NULL);
    view->state.view_mode = TRACKER_VIEW_MODE_FX;

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should contain FX mode header */
    ASSERT_NOT_NULL(strstr(output, "FX"));

    free(output);
    tracker_view_free(view);
    tracker_song_free(song);
}

TEST(render_to_string_different_dimensions) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    /* Test various dimensions */
    int test_sizes[][2] = {
        {40, 10},
        {80, 24},
        {120, 40},
        {200, 50}
    };

    for (int i = 0; i < 4; i++) {
        char* output = tracker_view_terminal_render_to_string(view,
            test_sizes[i][0], test_sizes[i][1]);
        ASSERT_NOT_NULL(output);
        ASSERT_TRUE(strlen(output) > 0);
        free(output);
    }

    tracker_view_free(view);
}

/*============================================================================
 * Key Injection Tests
 *============================================================================*/

TEST(inject_key_stores_input) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    tracker_view_terminal_inject_key(view, "j");

    /* Input should be available via poll_input */
    TrackerInputEvent event;
    bool result = view->callbacks.poll_input(view, 0, &event);
    ASSERT_TRUE(result);
    ASSERT_EQ(event.type, TRACKER_INPUT_CURSOR_DOWN);  /* 'j' maps to cursor down */

    tracker_view_free(view);
}

TEST(inject_key_multiple_chars) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    tracker_view_terminal_inject_key(view, "jjj");

    /* Should be able to read 3 events */
    TrackerInputEvent event;
    for (int i = 0; i < 3; i++) {
        bool result = view->callbacks.poll_input(view, 0, &event);
        ASSERT_TRUE(result);
        ASSERT_EQ(event.type, TRACKER_INPUT_CURSOR_DOWN);
    }

    /* Fourth read should fail (no more input) */
    bool result = view->callbacks.poll_input(view, 0, &event);
    ASSERT_FALSE(result);

    tracker_view_free(view);
}

TEST(inject_key_vim_navigation) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerInputEvent event;

    /* Test h key */
    tracker_view_terminal_inject_key(view, "h");
    ASSERT_TRUE(view->callbacks.poll_input(view, 0, &event));
    ASSERT_EQ(event.type, TRACKER_INPUT_CURSOR_LEFT);

    /* Test k key */
    tracker_view_terminal_inject_key(view, "k");
    ASSERT_TRUE(view->callbacks.poll_input(view, 0, &event));
    ASSERT_EQ(event.type, TRACKER_INPUT_CURSOR_UP);

    /* Test l key */
    tracker_view_terminal_inject_key(view, "l");
    ASSERT_TRUE(view->callbacks.poll_input(view, 0, &event));
    ASSERT_EQ(event.type, TRACKER_INPUT_CURSOR_RIGHT);

    tracker_view_free(view);
}

TEST(inject_key_command_keys) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerInputEvent event;

    /* Test colon for command mode */
    tracker_view_terminal_inject_key(view, ":");
    ASSERT_TRUE(view->callbacks.poll_input(view, 0, &event));
    ASSERT_EQ(event.type, TRACKER_INPUT_COMMAND_MODE);

    /* Test space for play toggle */
    tracker_view_terminal_inject_key(view, " ");
    ASSERT_TRUE(view->callbacks.poll_input(view, 0, &event));
    ASSERT_EQ(event.type, TRACKER_INPUT_PLAY_TOGGLE);

    tracker_view_free(view);
}

TEST(inject_key_escape) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    tracker_view_terminal_inject_key(view, "\x1b");

    TrackerInputEvent event;
    bool result = view->callbacks.poll_input(view, 0, &event);
    ASSERT_TRUE(result);
    ASSERT_EQ(event.type, TRACKER_INPUT_CANCEL);

    tracker_view_free(view);
}

TEST(inject_key_printable_char) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    /* Test a character that maps to TRACKER_INPUT_CHAR */
    tracker_view_terminal_inject_key(view, "1");

    TrackerInputEvent event;
    bool result = view->callbacks.poll_input(view, 0, &event);
    ASSERT_TRUE(result);
    ASSERT_EQ(event.type, TRACKER_INPUT_CHAR);
    ASSERT_EQ(event.character, '1');

    tracker_view_free(view);
}

TEST(inject_key_replaces_previous) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    /* Inject first set */
    tracker_view_terminal_inject_key(view, "abc");

    /* Inject second set - should replace */
    tracker_view_terminal_inject_key(view, "x");

    TrackerInputEvent event;
    bool result = view->callbacks.poll_input(view, 0, &event);
    ASSERT_TRUE(result);
    ASSERT_EQ(event.type, TRACKER_INPUT_CLEAR_CELL);  /* 'x' maps to clear cell */

    /* No more input */
    result = view->callbacks.poll_input(view, 0, &event);
    ASSERT_FALSE(result);

    tracker_view_free(view);
}

/*============================================================================
 * Dimensions Tests
 *============================================================================*/

TEST(get_size_returns_values) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    int cols = 0, rows = 0;
    tracker_view_terminal_get_size(view, &cols, &rows);

    /* Should return some default values */
    ASSERT_TRUE(cols > 0);
    ASSERT_TRUE(rows > 0);

    tracker_view_free(view);
}

TEST(update_size_marks_layout_dirty) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    /* Get layout to clear dirty flag */
    tracker_view_terminal_get_layout(view);

    /* Update size should mark layout as dirty */
    tracker_view_terminal_update_size(view);

    /* Next get_layout should recalculate */
    const TrackerTerminalLayout* layout = tracker_view_terminal_get_layout(view);
    ASSERT_NOT_NULL(layout);

    tracker_view_free(view);
}

/*============================================================================
 * Status/Error Message Tests
 *============================================================================*/

TEST(show_message_displays_in_output) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    view->callbacks.show_message(view, "Test status message");

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Status message should appear in output */
    ASSERT_NOT_NULL(strstr(output, "Test status message"));

    free(output);
    tracker_view_free(view);
}

TEST(show_error_displays_in_output) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    view->callbacks.show_error(view, "Test error message");

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Error message should appear in output */
    ASSERT_NOT_NULL(strstr(output, "Test error message"));

    free(output);
    tracker_view_free(view);
}

/*============================================================================
 * Playback State Rendering Tests
 *============================================================================*/

TEST(render_shows_play_state) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    view->state.is_playing = false;
    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);
    ASSERT_NOT_NULL(strstr(output, "STOP"));
    free(output);

    view->state.is_playing = true;
    output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);
    ASSERT_NOT_NULL(strstr(output, "PLAY"));
    free(output);

    tracker_view_free(view);
}

TEST(render_shows_record_state) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    view->state.is_recording = true;
    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);
    ASSERT_NOT_NULL(strstr(output, "REC"));
    free(output);

    tracker_view_free(view);
}

/*============================================================================
 * Edit Mode Rendering Tests
 *============================================================================*/

TEST(render_shows_edit_mode) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    view->state.edit_mode = TRACKER_EDIT_MODE_NAVIGATE;
    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);
    ASSERT_NOT_NULL(strstr(output, "NAV"));
    free(output);

    view->state.edit_mode = TRACKER_EDIT_MODE_EDIT;
    output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);
    ASSERT_NOT_NULL(strstr(output, "EDIT"));
    free(output);

    tracker_view_free(view);
}

TEST(render_shows_command_mode) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    view->state.edit_mode = TRACKER_EDIT_MODE_COMMAND;
    view->state.command_buffer = strdup("w");

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should show command prompt */
    ASSERT_NOT_NULL(strstr(output, ":"));

    free(output);
    free(view->state.command_buffer);
    view->state.command_buffer = NULL;
    tracker_view_free(view);
}

/*============================================================================
 * Unicode/ASCII Fallback Tests
 *============================================================================*/

TEST(config_unicode_borders) {
    TrackerTerminalConfig config;
    tracker_terminal_config_init(&config);
    config.use_unicode_borders = true;

    TrackerView* view = tracker_view_terminal_new_with_config(&config);
    ASSERT_NOT_NULL(view);

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Unicode box drawing uses multi-byte sequences */
    /* Check for presence of extended characters (beyond ASCII) */
    bool has_extended = false;
    for (size_t i = 0; output[i]; i++) {
        if ((unsigned char)output[i] > 127) {
            has_extended = true;
            break;
        }
    }
    ASSERT_TRUE(has_extended);

    free(output);
    tracker_view_free(view);
}

TEST(config_ascii_borders) {
    TrackerTerminalConfig config;
    tracker_terminal_config_init(&config);
    config.use_unicode_borders = false;

    TrackerView* view = tracker_view_terminal_new_with_config(&config);
    ASSERT_NOT_NULL(view);

    TrackerSong* song = create_test_song(16, 4);
    ASSERT_NOT_NULL(song);
    tracker_view_attach(view, song, NULL);

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* ASCII mode should use +, -, | for borders */
    ASSERT_NOT_NULL(strstr(output, "|"));
    ASSERT_NOT_NULL(strstr(output, "-"));

    free(output);
    tracker_view_free(view);
    tracker_song_free(song);
}

/*============================================================================
 * Track Operations Rendering Tests
 *============================================================================*/

TEST(render_shows_muted_track) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerSong* song = create_test_song(16, 4);
    ASSERT_NOT_NULL(song);

    TrackerPattern* pattern = tracker_song_get_pattern(song, 0);
    pattern->tracks[0].muted = true;

    tracker_view_attach(view, song, NULL);

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should show mute indicator */
    ASSERT_NOT_NULL(strstr(output, "[M]"));

    free(output);
    tracker_view_free(view);
    tracker_song_free(song);
}

TEST(render_shows_solo_track) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerSong* song = create_test_song(16, 4);
    ASSERT_NOT_NULL(song);

    TrackerPattern* pattern = tracker_song_get_pattern(song, 0);
    pattern->tracks[0].solo = true;

    tracker_view_attach(view, song, NULL);

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should show solo indicator */
    ASSERT_NOT_NULL(strstr(output, "[S]"));

    free(output);
    tracker_view_free(view);
    tracker_song_free(song);
}

/*============================================================================
 * Beep Test
 *============================================================================*/

TEST(beep_does_not_crash) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    /* Just verify it doesn't crash */
    view->callbacks.beep(view);

    tracker_view_free(view);
}

/*============================================================================
 * Selection Rendering Tests
 *============================================================================*/

TEST(render_shows_visual_mode) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerSong* song = create_test_song(16, 4);
    ASSERT_NOT_NULL(song);

    tracker_view_attach(view, song, NULL);
    view->state.selecting = true;

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should show VISUAL mode indicator */
    ASSERT_NOT_NULL(strstr(output, "VISUAL"));

    free(output);
    tracker_view_free(view);
    tracker_song_free(song);
}

/*============================================================================
 * BPM Display Tests
 *============================================================================*/

TEST(render_shows_bpm) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    TrackerSong* song = create_test_song(16, 4);
    ASSERT_NOT_NULL(song);
    song->bpm = 140;

    tracker_view_attach(view, song, NULL);

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should show BPM */
    ASSERT_NOT_NULL(strstr(output, "140"));
    ASSERT_NOT_NULL(strstr(output, "BPM"));

    free(output);
    tracker_view_free(view);
    tracker_song_free(song);
}

/*============================================================================
 * Step/Octave Display Tests
 *============================================================================*/

TEST(render_shows_step_and_octave) {
    TrackerView* view = tracker_view_terminal_new();
    ASSERT_NOT_NULL(view);

    view->state.step_size = 4;
    view->state.default_octave = 5;

    char* output = tracker_view_terminal_render_to_string(view, 80, 24);
    ASSERT_NOT_NULL(output);

    /* Should show step size and octave */
    ASSERT_NOT_NULL(strstr(output, "Step:4"));
    ASSERT_NOT_NULL(strstr(output, "Oct:5"));

    free(output);
    tracker_view_free(view);
}

/*============================================================================
 * Main Test Runner
 *============================================================================*/

BEGIN_TEST_SUITE("Tracker View Terminal Tests")
    /* Config tests */
    RUN_TEST(config_init_sets_defaults);
    RUN_TEST(config_init_multiple_calls_consistent);

    /* View creation tests */
    RUN_TEST(terminal_new_creates_view);
    RUN_TEST(terminal_new_with_config_uses_config);
    RUN_TEST(terminal_new_with_fds_creates_view);
    RUN_TEST(terminal_view_free_null_safe);

    /* Color support tests */
    RUN_TEST(has_colors_returns_config_value);
    RUN_TEST(has_256_colors_returns_config_value);
    RUN_TEST(has_true_color_returns_config_value);

    /* Layout tests */
    RUN_TEST(get_layout_returns_valid_layout);
    RUN_TEST(get_layout_with_song_calculates_tracks);
    RUN_TEST(layout_respects_screen_dimensions);

    /* String rendering tests */
    RUN_TEST(render_to_string_returns_output);
    RUN_TEST(render_to_string_contains_escape_sequences);
    RUN_TEST(render_to_string_with_song);
    RUN_TEST(render_to_string_with_cell_content);
    RUN_TEST(render_to_string_help_mode);
    RUN_TEST(render_to_string_arrange_mode);
    RUN_TEST(render_to_string_mixer_mode);
    RUN_TEST(render_to_string_fx_mode);
    RUN_TEST(render_to_string_different_dimensions);

    /* Key injection tests */
    RUN_TEST(inject_key_stores_input);
    RUN_TEST(inject_key_multiple_chars);
    RUN_TEST(inject_key_vim_navigation);
    RUN_TEST(inject_key_command_keys);
    RUN_TEST(inject_key_escape);
    RUN_TEST(inject_key_printable_char);
    RUN_TEST(inject_key_replaces_previous);

    /* Dimensions tests */
    RUN_TEST(get_size_returns_values);
    RUN_TEST(update_size_marks_layout_dirty);

    /* Status/error message tests */
    RUN_TEST(show_message_displays_in_output);
    RUN_TEST(show_error_displays_in_output);

    /* Playback state rendering tests */
    RUN_TEST(render_shows_play_state);
    RUN_TEST(render_shows_record_state);

    /* Edit mode rendering tests */
    RUN_TEST(render_shows_edit_mode);
    RUN_TEST(render_shows_command_mode);

    /* Unicode/ASCII tests */
    RUN_TEST(config_unicode_borders);
    RUN_TEST(config_ascii_borders);

    /* Track operations tests */
    RUN_TEST(render_shows_muted_track);
    RUN_TEST(render_shows_solo_track);

    /* Beep test */
    RUN_TEST(beep_does_not_crash);

    /* Selection test */
    RUN_TEST(render_shows_visual_mode);

    /* BPM test */
    RUN_TEST(render_shows_bpm);

    /* Step/octave test */
    RUN_TEST(render_shows_step_and_octave);
END_TEST_SUITE()
