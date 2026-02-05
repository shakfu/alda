/**
 * test_view.c - Tests for tracker view core functionality
 */

#include "test_framework.h"
#include "tracker_view.h"
#include "tracker_model.h"
#include <stdlib.h>
#include <string.h>

/* Global test stats required by test framework */
test_stats_t test_stats;

/*============================================================================
 * Test Helpers
 *============================================================================*/

/* Create a minimal view for testing (no callbacks) */
static TrackerView* create_test_view(void) {
    TrackerView* view = calloc(1, sizeof(TrackerView));
    if (view) {
        tracker_view_state_init(&view->state);
        tracker_undo_init(&view->undo, 0);
        /* Set default theme like tracker_view_new() does */
        view->state.theme = (TrackerTheme*)tracker_theme_get("default");
        view->state.owns_theme = false;
    }
    return view;
}

/* Create a view with song attached */
static TrackerView* create_test_view_with_song(int num_rows, int num_tracks) {
    TrackerView* view = create_test_view();
    if (!view) return NULL;

    TrackerSong* song = tracker_song_new("Test Song");
    if (!song) {
        tracker_view_state_cleanup(&view->state);
        tracker_undo_cleanup(&view->undo);
        free(view);
        return NULL;
    }

    TrackerPattern* pattern = tracker_pattern_new(num_rows, num_tracks, "Pattern 1");
    if (!pattern) {
        tracker_song_free(song);
        tracker_view_state_cleanup(&view->state);
        tracker_undo_cleanup(&view->undo);
        free(view);
        return NULL;
    }

    tracker_song_add_pattern(song, pattern);
    view->song = song;
    view->state.cursor_pattern = 0;
    view->state.cursor_track = 0;
    view->state.cursor_row = 0;

    return view;
}

static void free_test_view(TrackerView* view) {
    if (view) {
        if (view->song) {
            tracker_song_free(view->song);
        }
        tracker_view_clipboard_clear(view);
        tracker_view_state_cleanup(&view->state);
        tracker_undo_cleanup(&view->undo);
        free(view->file_path);
        free(view);
    }
}

/*============================================================================
 * State Init/Cleanup Tests
 *============================================================================*/

TEST(state_init_null_safe) {
    tracker_view_state_init(NULL);
}

TEST(state_init_sets_defaults) {
    TrackerViewState state;
    tracker_view_state_init(&state);

    ASSERT_EQ(state.view_mode, TRACKER_VIEW_MODE_PATTERN);
    ASSERT_EQ(state.edit_mode, TRACKER_EDIT_MODE_NAVIGATE);
    ASSERT_EQ(state.selection.type, TRACKER_SEL_NONE);
    ASSERT_TRUE(state.follow_playback);
    ASSERT_TRUE(state.show_row_numbers);
    ASSERT_TRUE(state.show_track_headers);
    ASSERT_TRUE(state.show_transport);
    ASSERT_TRUE(state.show_status_line);
    ASSERT_TRUE(state.highlight_current_row);
    ASSERT_TRUE(state.highlight_beat_rows);
    ASSERT_EQ(state.beat_highlight_interval, 4);
    ASSERT_EQ(state.visible_tracks, 8);
    ASSERT_EQ(state.visible_rows, 32);
    ASSERT_EQ(state.step_size, 1);
    ASSERT_EQ(state.default_octave, 4);

    tracker_view_state_cleanup(&state);
}

TEST(state_cleanup_null_safe) {
    tracker_view_state_cleanup(NULL);
}

TEST(state_cleanup_frees_buffers) {
    TrackerViewState state;
    tracker_view_state_init(&state);

    state.edit_buffer = strdup("test");
    state.command_buffer = strdup("command");
    state.status_message = strdup("status");
    state.error_message = strdup("error");

    tracker_view_state_cleanup(&state);

    /* After cleanup, everything should be zeroed */
    ASSERT_NULL(state.edit_buffer);
    ASSERT_NULL(state.command_buffer);
}

/*============================================================================
 * View New/Free Tests
 *============================================================================*/

TEST(view_new_no_callbacks) {
    TrackerView* view = tracker_view_new(NULL);
    ASSERT_NOT_NULL(view);

    ASSERT_EQ(view->state.view_mode, TRACKER_VIEW_MODE_PATTERN);
    ASSERT_NOT_NULL(view->state.theme);
    ASSERT_FALSE(view->state.owns_theme);

    tracker_view_free(view);
}

TEST(view_free_null_safe) {
    tracker_view_free(NULL);
}

/*============================================================================
 * Attach/Detach Tests
 *============================================================================*/

TEST(attach_null_safe) {
    tracker_view_attach(NULL, NULL, NULL);
}

TEST(attach_sets_song) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    TrackerSong* song = tracker_song_new("Test");
    TrackerPattern* pattern = tracker_pattern_new(16, 4, "Pattern 1");
    tracker_song_add_pattern(song, pattern);

    tracker_view_attach(view, song, NULL);

    ASSERT_PTR_EQ(view->song, song);
    ASSERT_EQ(view->state.cursor_pattern, 0);
    ASSERT_EQ(view->state.cursor_track, 0);
    ASSERT_EQ(view->state.cursor_row, 0);

    tracker_song_free(song);
    view->song = NULL;
    free_test_view(view);
}

TEST(detach_clears_song) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);
    ASSERT_NOT_NULL(view->song);

    TrackerSong* song = view->song;
    tracker_view_detach(view);

    ASSERT_NULL(view->song);
    ASSERT_NULL(view->engine);

    tracker_song_free(song);
    free_test_view(view);
}

TEST(detach_null_safe) {
    tracker_view_detach(NULL);
}

/*============================================================================
 * Theme Tests
 *============================================================================*/

TEST(set_theme_null_safe) {
    tracker_view_set_theme(NULL, NULL, false);
}

TEST(set_theme_by_name_null_safe) {
    ASSERT_FALSE(tracker_view_set_theme_by_name(NULL, "default"));
}

TEST(set_theme_by_name_invalid_returns_false) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    ASSERT_FALSE(tracker_view_set_theme_by_name(view, "nonexistent_theme"));

    free_test_view(view);
}

TEST(set_theme_by_name_valid) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    ASSERT_TRUE(tracker_view_set_theme_by_name(view, "default"));
    ASSERT_NOT_NULL(view->state.theme);

    free_test_view(view);
}

TEST(get_theme_null_safe) {
    ASSERT_NULL(tracker_view_get_theme(NULL));
}

TEST(get_theme_returns_theme) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    const TrackerTheme* theme = tracker_view_get_theme(view);
    ASSERT_NOT_NULL(theme);

    free_test_view(view);
}

/*============================================================================
 * Invalidation Tests
 *============================================================================*/

TEST(invalidate_null_safe) {
    tracker_view_invalidate(NULL);
}

TEST(invalidate_sets_all_flags) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->dirty_flags = TRACKER_DIRTY_NONE;
    tracker_view_invalidate(view);

    ASSERT_EQ(view->dirty_flags, TRACKER_DIRTY_ALL);

    free_test_view(view);
}

TEST(invalidate_cell_null_safe) {
    tracker_view_invalidate_cell(NULL, 0, 0);
}

TEST(invalidate_cell_sets_flag) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->dirty_flags = TRACKER_DIRTY_NONE;
    tracker_view_invalidate_cell(view, 2, 5);

    ASSERT_TRUE(view->dirty_flags & TRACKER_DIRTY_CELL);
    ASSERT_EQ(view->dirty_cell_track, 2);
    ASSERT_EQ(view->dirty_cell_row, 5);

    free_test_view(view);
}

TEST(invalidate_row_null_safe) {
    tracker_view_invalidate_row(NULL, 0);
}

TEST(invalidate_row_sets_flag) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->dirty_flags = TRACKER_DIRTY_NONE;
    tracker_view_invalidate_row(view, 3);

    ASSERT_TRUE(view->dirty_flags & TRACKER_DIRTY_ROW);
    ASSERT_EQ(view->dirty_row, 3);

    free_test_view(view);
}

TEST(invalidate_track_null_safe) {
    tracker_view_invalidate_track(NULL, 0);
}

TEST(invalidate_track_sets_flag) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->dirty_flags = TRACKER_DIRTY_NONE;
    tracker_view_invalidate_track(view, 1);

    ASSERT_TRUE(view->dirty_flags & TRACKER_DIRTY_TRACK);
    ASSERT_EQ(view->dirty_track, 1);

    free_test_view(view);
}

TEST(invalidate_cursor_null_safe) {
    tracker_view_invalidate_cursor(NULL);
}

TEST(invalidate_cursor_sets_flag) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->dirty_flags = TRACKER_DIRTY_NONE;
    tracker_view_invalidate_cursor(view);

    ASSERT_TRUE(view->dirty_flags & TRACKER_DIRTY_CURSOR);

    free_test_view(view);
}

TEST(invalidate_selection_null_safe) {
    tracker_view_invalidate_selection(NULL);
}

TEST(invalidate_status_null_safe) {
    tracker_view_invalidate_status(NULL);
}

/*============================================================================
 * Render Tests
 *============================================================================*/

TEST(render_null_safe) {
    tracker_view_render(NULL);
}

TEST(render_clears_dirty_flags) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->dirty_flags = TRACKER_DIRTY_ALL;
    tracker_view_render(view);  /* No callbacks, should just clear flags */

    ASSERT_EQ(view->dirty_flags, TRACKER_DIRTY_NONE);

    free_test_view(view);
}

TEST(render_skip_when_clean) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->dirty_flags = TRACKER_DIRTY_NONE;
    tracker_view_render(view);

    ASSERT_EQ(view->dirty_flags, TRACKER_DIRTY_NONE);

    free_test_view(view);
}

/*============================================================================
 * Update Playback Tests
 *============================================================================*/

TEST(update_playback_null_safe) {
    tracker_view_update_playback(NULL, 0, 0);
}

TEST(update_playback_updates_state) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->dirty_flags = TRACKER_DIRTY_NONE;
    tracker_view_update_playback(view, 1, 8);

    ASSERT_EQ(view->state.playback_pattern, 1);
    ASSERT_EQ(view->state.playback_row, 8);
    ASSERT_TRUE(view->dirty_flags & TRACKER_DIRTY_PLAYBACK);

    free_test_view(view);
}

TEST(update_playback_no_change_no_dirty) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->state.playback_pattern = 0;
    view->state.playback_row = 5;
    view->dirty_flags = TRACKER_DIRTY_NONE;

    tracker_view_update_playback(view, 0, 5);

    ASSERT_FALSE(view->dirty_flags & TRACKER_DIRTY_PLAYBACK);

    free_test_view(view);
}

/*============================================================================
 * Cursor Movement Tests
 *============================================================================*/

TEST(cursor_up_null_safe) {
    tracker_view_cursor_up(NULL, 1);
}

TEST(cursor_up_moves_cursor) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_row = 5;
    tracker_view_cursor_up(view, 2);

    ASSERT_EQ(view->state.cursor_row, 3);

    free_test_view(view);
}

TEST(cursor_up_clamps_to_zero) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_row = 2;
    tracker_view_cursor_up(view, 10);

    ASSERT_EQ(view->state.cursor_row, 0);

    free_test_view(view);
}

TEST(cursor_down_null_safe) {
    tracker_view_cursor_down(NULL, 1);
}

TEST(cursor_down_moves_cursor) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_row = 5;
    tracker_view_cursor_down(view, 3);

    ASSERT_EQ(view->state.cursor_row, 8);

    free_test_view(view);
}

TEST(cursor_down_clamps_to_max) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_row = 10;
    tracker_view_cursor_down(view, 20);

    ASSERT_EQ(view->state.cursor_row, 15);  /* 16 rows, max is 15 */

    free_test_view(view);
}

TEST(cursor_left_null_safe) {
    tracker_view_cursor_left(NULL, 1);
}

TEST(cursor_left_moves_cursor) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_track = 3;
    tracker_view_cursor_left(view, 2);

    ASSERT_EQ(view->state.cursor_track, 1);

    free_test_view(view);
}

TEST(cursor_left_clamps_to_zero) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_track = 1;
    tracker_view_cursor_left(view, 5);

    ASSERT_EQ(view->state.cursor_track, 0);

    free_test_view(view);
}

TEST(cursor_right_null_safe) {
    tracker_view_cursor_right(NULL, 1);
}

TEST(cursor_right_moves_cursor) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_track = 0;
    tracker_view_cursor_right(view, 2);

    ASSERT_EQ(view->state.cursor_track, 2);

    free_test_view(view);
}

TEST(cursor_right_clamps_to_max) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_track = 2;
    tracker_view_cursor_right(view, 10);

    ASSERT_EQ(view->state.cursor_track, 3);  /* 4 tracks, max is 3 */

    free_test_view(view);
}

TEST(cursor_page_up_null_safe) {
    tracker_view_cursor_page_up(NULL);
}

TEST(cursor_page_up_moves_by_visible_rows) {
    TrackerView* view = create_test_view_with_song(64, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_row = 40;
    view->state.visible_rows = 16;
    tracker_view_cursor_page_up(view);

    ASSERT_EQ(view->state.cursor_row, 24);

    free_test_view(view);
}

TEST(cursor_page_down_null_safe) {
    tracker_view_cursor_page_down(NULL);
}

TEST(cursor_page_down_moves_by_visible_rows) {
    TrackerView* view = create_test_view_with_song(64, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_row = 10;
    view->state.visible_rows = 16;
    tracker_view_cursor_page_down(view);

    ASSERT_EQ(view->state.cursor_row, 26);

    free_test_view(view);
}

TEST(cursor_home_null_safe) {
    tracker_view_cursor_home(NULL);
}

TEST(cursor_home_moves_to_first_track) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_track = 3;
    tracker_view_cursor_home(view);

    ASSERT_EQ(view->state.cursor_track, 0);

    free_test_view(view);
}

TEST(cursor_end_null_safe) {
    tracker_view_cursor_end(NULL);
}

TEST(cursor_end_moves_to_last_track) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_track = 0;
    tracker_view_cursor_end(view);

    ASSERT_EQ(view->state.cursor_track, 3);

    free_test_view(view);
}

TEST(cursor_pattern_start_null_safe) {
    tracker_view_cursor_pattern_start(NULL);
}

TEST(cursor_pattern_start_moves_to_row_zero) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_row = 10;
    tracker_view_cursor_pattern_start(view);

    ASSERT_EQ(view->state.cursor_row, 0);

    free_test_view(view);
}

TEST(cursor_pattern_end_null_safe) {
    tracker_view_cursor_pattern_end(NULL);
}

TEST(cursor_pattern_end_moves_to_last_row) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_row = 5;
    tracker_view_cursor_pattern_end(view);

    ASSERT_EQ(view->state.cursor_row, 15);

    free_test_view(view);
}

TEST(cursor_goto_null_safe) {
    tracker_view_cursor_goto(NULL, 0, 0, 0);
}

TEST(cursor_goto_sets_position) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    tracker_view_cursor_goto(view, 0, 2, 8);

    ASSERT_EQ(view->state.cursor_pattern, 0);
    ASSERT_EQ(view->state.cursor_track, 2);
    ASSERT_EQ(view->state.cursor_row, 8);

    free_test_view(view);
}

/*============================================================================
 * Pattern Navigation Tests
 *============================================================================*/

TEST(next_pattern_null_safe) {
    tracker_view_next_pattern(NULL);
}

TEST(next_pattern_no_song_noop) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    tracker_view_next_pattern(view);

    free_test_view(view);
}

TEST(next_pattern_wraps_around) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    /* Add second pattern */
    TrackerPattern* p2 = tracker_pattern_new(16, 4, "Pattern 2");
    tracker_song_add_pattern(view->song, p2);

    view->state.cursor_pattern = 1;
    tracker_view_next_pattern(view);

    ASSERT_EQ(view->state.cursor_pattern, 0);  /* Wrapped */

    free_test_view(view);
}

TEST(prev_pattern_null_safe) {
    tracker_view_prev_pattern(NULL);
}

TEST(prev_pattern_wraps_around) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    /* Add second pattern */
    TrackerPattern* p2 = tracker_pattern_new(16, 4, "Pattern 2");
    tracker_song_add_pattern(view->song, p2);

    view->state.cursor_pattern = 0;
    tracker_view_prev_pattern(view);

    ASSERT_EQ(view->state.cursor_pattern, 1);  /* Wrapped to last */

    free_test_view(view);
}

TEST(new_pattern_null_safe) {
    tracker_view_new_pattern(NULL);
}

TEST(new_pattern_creates_pattern) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    int initial_count = view->song->num_patterns;
    tracker_view_new_pattern(view);

    ASSERT_EQ(view->song->num_patterns, initial_count + 1);
    ASSERT_EQ(view->state.cursor_pattern, initial_count);
    ASSERT_TRUE(view->modified);

    free_test_view(view);
}

TEST(clone_pattern_null_safe) {
    tracker_view_clone_pattern(NULL);
}

TEST(clone_pattern_copies_pattern) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    /* Set some content in first pattern */
    TrackerPattern* original = tracker_view_get_current_pattern(view);
    TrackerCell* cell = tracker_pattern_get_cell(original, 0, 0);
    cell->type = TRACKER_CELL_EXPRESSION;
    cell->expression = strdup("C4");

    int initial_count = view->song->num_patterns;
    tracker_view_clone_pattern(view);

    ASSERT_EQ(view->song->num_patterns, initial_count + 1);

    /* New pattern should have same content */
    TrackerPattern* cloned = tracker_view_get_current_pattern(view);
    TrackerCell* cloned_cell = tracker_pattern_get_cell(cloned, 0, 0);
    ASSERT_EQ(cloned_cell->type, TRACKER_CELL_EXPRESSION);
    ASSERT_STR_EQ(cloned_cell->expression, "C4");

    free_test_view(view);
}

TEST(delete_pattern_null_safe) {
    tracker_view_delete_pattern(NULL);
}

TEST(delete_pattern_cannot_delete_last) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    ASSERT_EQ(view->song->num_patterns, 1);
    tracker_view_delete_pattern(view);

    /* Should still have 1 pattern */
    ASSERT_EQ(view->song->num_patterns, 1);

    free_test_view(view);
}

TEST(delete_pattern_removes_pattern) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    /* Add second pattern */
    TrackerPattern* p2 = tracker_pattern_new(16, 4, "Pattern 2");
    tracker_song_add_pattern(view->song, p2);

    ASSERT_EQ(view->song->num_patterns, 2);
    view->state.cursor_pattern = 1;
    tracker_view_delete_pattern(view);

    ASSERT_EQ(view->song->num_patterns, 1);
    ASSERT_TRUE(view->modified);

    free_test_view(view);
}

/*============================================================================
 * Track Management Tests
 *============================================================================*/

TEST(add_track_null_safe) {
    tracker_view_add_track(NULL);
}

TEST(add_track_adds_track) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    TrackerPattern* pattern = tracker_view_get_current_pattern(view);
    int initial_tracks = pattern->num_tracks;

    tracker_view_add_track(view);

    ASSERT_EQ(pattern->num_tracks, initial_tracks + 1);
    ASSERT_EQ(view->state.cursor_track, initial_tracks);
    ASSERT_TRUE(view->modified);

    free_test_view(view);
}

TEST(remove_track_null_safe) {
    tracker_view_remove_track(NULL);
}

TEST(remove_track_cannot_remove_last) {
    TrackerView* view = create_test_view_with_song(16, 1);
    ASSERT_NOT_NULL(view);

    TrackerPattern* pattern = tracker_view_get_current_pattern(view);
    ASSERT_EQ(pattern->num_tracks, 1);

    tracker_view_remove_track(view);

    /* Should still have 1 track */
    ASSERT_EQ(pattern->num_tracks, 1);

    free_test_view(view);
}

TEST(remove_track_removes_track) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    TrackerPattern* pattern = tracker_view_get_current_pattern(view);
    ASSERT_EQ(pattern->num_tracks, 4);

    view->state.cursor_track = 2;
    tracker_view_remove_track(view);

    ASSERT_EQ(pattern->num_tracks, 3);
    ASSERT_TRUE(view->modified);

    free_test_view(view);
}

/*============================================================================
 * Mode Switching Tests
 *============================================================================*/

TEST(set_mode_null_safe) {
    tracker_view_set_mode(NULL, TRACKER_VIEW_MODE_PATTERN);
}

TEST(set_mode_changes_mode) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->state.view_mode = TRACKER_VIEW_MODE_PATTERN;
    tracker_view_set_mode(view, TRACKER_VIEW_MODE_ARRANGE);

    ASSERT_EQ(view->state.view_mode, TRACKER_VIEW_MODE_ARRANGE);

    free_test_view(view);
}

TEST(enter_edit_null_safe) {
    tracker_view_enter_edit(NULL);
}

TEST(enter_edit_sets_mode) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    tracker_view_enter_edit(view);

    ASSERT_EQ(view->state.edit_mode, TRACKER_EDIT_MODE_EDIT);
    ASSERT_NOT_NULL(view->state.edit_buffer);

    free_test_view(view);
}

TEST(exit_edit_null_safe) {
    tracker_view_exit_edit(NULL, true);
    tracker_view_exit_edit(NULL, false);
}

TEST(exit_edit_cancel_returns_to_navigate) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    tracker_view_enter_edit(view);
    tracker_view_exit_edit(view, false);

    ASSERT_EQ(view->state.edit_mode, TRACKER_EDIT_MODE_NAVIGATE);

    free_test_view(view);
}

TEST(enter_command_null_safe) {
    tracker_view_enter_command(NULL);
}

TEST(enter_command_sets_mode) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    tracker_view_enter_command(view);

    ASSERT_EQ(view->state.edit_mode, TRACKER_EDIT_MODE_COMMAND);
    ASSERT_NOT_NULL(view->state.command_buffer);
    ASSERT_EQ(view->state.command_buffer_len, 0);

    free_test_view(view);
}

/*============================================================================
 * Utility Tests
 *============================================================================*/

TEST(get_cursor_cell_null_safe) {
    ASSERT_NULL(tracker_view_get_cursor_cell(NULL));
}

TEST(get_cursor_cell_no_song_returns_null) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    ASSERT_NULL(tracker_view_get_cursor_cell(view));

    free_test_view(view);
}

TEST(get_cursor_cell_returns_cell) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    TrackerCell* cell = tracker_view_get_cursor_cell(view);
    ASSERT_NOT_NULL(cell);

    free_test_view(view);
}

TEST(get_current_pattern_null_safe) {
    ASSERT_NULL(tracker_view_get_current_pattern(NULL));
}

TEST(get_current_pattern_returns_pattern) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    TrackerPattern* pattern = tracker_view_get_current_pattern(view);
    ASSERT_NOT_NULL(pattern);
    ASSERT_EQ(pattern->num_rows, 16);
    ASSERT_EQ(pattern->num_tracks, 4);

    free_test_view(view);
}

TEST(cursor_valid_null_returns_false) {
    ASSERT_FALSE(tracker_view_cursor_valid(NULL));
}

TEST(cursor_valid_no_song_returns_false) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    ASSERT_FALSE(tracker_view_cursor_valid(view));

    free_test_view(view);
}

TEST(cursor_valid_returns_true) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_track = 2;
    view->state.cursor_row = 8;

    ASSERT_TRUE(tracker_view_cursor_valid(view));

    free_test_view(view);
}

TEST(clamp_cursor_null_safe) {
    tracker_view_clamp_cursor(NULL);
}

TEST(clamp_cursor_clamps_values) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_track = 100;
    view->state.cursor_row = 100;
    tracker_view_clamp_cursor(view);

    ASSERT_EQ(view->state.cursor_track, 3);
    ASSERT_EQ(view->state.cursor_row, 15);

    free_test_view(view);
}

TEST(clamp_cursor_clamps_negative) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    view->state.cursor_track = -5;
    view->state.cursor_row = -10;
    tracker_view_clamp_cursor(view);

    ASSERT_EQ(view->state.cursor_track, 0);
    ASSERT_EQ(view->state.cursor_row, 0);

    free_test_view(view);
}

TEST(ensure_visible_null_safe) {
    tracker_view_ensure_visible(NULL);
}

TEST(ensure_visible_adjusts_scroll) {
    TrackerView* view = create_test_view_with_song(64, 8);
    ASSERT_NOT_NULL(view);

    view->state.visible_rows = 16;
    view->state.visible_tracks = 4;
    view->state.scroll_row = 0;
    view->state.scroll_track = 0;

    view->state.cursor_row = 20;
    view->state.cursor_track = 6;
    tracker_view_ensure_visible(view);

    /* Scroll should adjust to show cursor */
    ASSERT_TRUE(view->state.scroll_row > 0);
    ASSERT_TRUE(view->state.scroll_track > 0);

    free_test_view(view);
}

TEST(get_visible_range_null_safe) {
    tracker_view_get_visible_range(NULL, NULL, NULL, NULL, NULL);
}

TEST(get_visible_range_returns_range) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->state.scroll_track = 2;
    view->state.scroll_row = 10;
    view->state.visible_tracks = 4;
    view->state.visible_rows = 16;

    int st, et, sr, er;
    tracker_view_get_visible_range(view, &st, &et, &sr, &er);

    ASSERT_EQ(st, 2);
    ASSERT_EQ(et, 5);
    ASSERT_EQ(sr, 10);
    ASSERT_EQ(er, 25);

    free_test_view(view);
}

/*============================================================================
 * File Path Tests
 *============================================================================*/

TEST(set_file_path_null_safe) {
    tracker_view_set_file_path(NULL, "test.trk");
}

TEST(set_file_path_sets_path) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    tracker_view_set_file_path(view, "test.trk");

    ASSERT_STR_EQ(view->file_path, "test.trk");

    free(view->file_path);
    view->file_path = NULL;
    free_test_view(view);
}

TEST(get_file_path_null_safe) {
    ASSERT_NULL(tracker_view_get_file_path(NULL));
}

TEST(get_file_path_returns_path) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->file_path = strdup("song.trk");

    const char* path = tracker_view_get_file_path(view);
    ASSERT_STR_EQ(path, "song.trk");

    free_test_view(view);
}

/*============================================================================
 * Modified Flag Tests
 *============================================================================*/

TEST(is_modified_null_returns_false) {
    ASSERT_FALSE(tracker_view_is_modified(NULL));
}

TEST(is_modified_returns_flag) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->modified = true;
    ASSERT_TRUE(tracker_view_is_modified(view));

    view->modified = false;
    ASSERT_FALSE(tracker_view_is_modified(view));

    free_test_view(view);
}

TEST(set_modified_null_safe) {
    tracker_view_set_modified(NULL, true);
}

TEST(set_modified_sets_flag) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    tracker_view_set_modified(view, true);
    ASSERT_TRUE(view->modified);

    tracker_view_set_modified(view, false);
    ASSERT_FALSE(view->modified);

    free_test_view(view);
}

/*============================================================================
 * Request Quit Tests
 *============================================================================*/

TEST(request_quit_null_safe) {
    tracker_view_request_quit(NULL);
}

TEST(request_quit_sets_flag) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    ASSERT_FALSE(view->quit_requested);
    tracker_view_request_quit(view);
    ASSERT_TRUE(view->quit_requested);

    free_test_view(view);
}

/*============================================================================
 * Message Tests
 *============================================================================*/

TEST(show_status_null_safe) {
    tracker_view_show_status(NULL, "test");
}

TEST(show_error_null_safe) {
    tracker_view_show_error(NULL, "error");
}

TEST(clear_messages_null_safe) {
    tracker_view_clear_messages(NULL);
}

TEST(clear_messages_clears_all) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->state.status_message = strdup("status");
    view->state.error_message = strdup("error");
    view->state.status_display_time = 5.0;
    view->state.error_display_time = 5.0;

    tracker_view_clear_messages(view);

    ASSERT_NULL(view->state.status_message);
    ASSERT_NULL(view->state.error_message);
    ASSERT_EQ(view->state.status_display_time, 0);
    ASSERT_EQ(view->state.error_display_time, 0);

    free_test_view(view);
}

/*============================================================================
 * Edit Char Tests
 *============================================================================*/

TEST(edit_char_null_safe) {
    tracker_view_edit_char(NULL, 'a');
}

TEST(edit_char_not_in_edit_mode_noop) {
    TrackerView* view = create_test_view();
    ASSERT_NOT_NULL(view);

    view->state.edit_mode = TRACKER_EDIT_MODE_NAVIGATE;
    tracker_view_edit_char(view, 'a');

    /* Should not allocate buffer */
    ASSERT_NULL(view->state.edit_buffer);

    free_test_view(view);
}

TEST(edit_char_adds_character) {
    TrackerView* view = create_test_view_with_song(16, 4);
    ASSERT_NOT_NULL(view);

    tracker_view_enter_edit(view);
    tracker_view_edit_char(view, 'C');
    tracker_view_edit_char(view, '4');

    ASSERT_EQ(view->state.edit_buffer_len, 2);
    ASSERT_EQ(view->state.edit_buffer[0], 'C');
    ASSERT_EQ(view->state.edit_buffer[1], '4');

    free_test_view(view);
}

/*============================================================================
 * Undo/Redo View Integration Tests
 *============================================================================*/

TEST(view_undo_null_safe) {
    ASSERT_FALSE(tracker_view_undo(NULL));
}

TEST(view_redo_null_safe) {
    ASSERT_FALSE(tracker_view_redo(NULL));
}

TEST(view_begin_undo_group_null_safe) {
    tracker_view_begin_undo_group(NULL, "test");
}

TEST(view_end_undo_group_null_safe) {
    tracker_view_end_undo_group(NULL);
}

/*============================================================================
 * Main
 *============================================================================*/

BEGIN_TEST_SUITE("Tracker View Tests")

    /* State init/cleanup */
    RUN_TEST(state_init_null_safe);
    RUN_TEST(state_init_sets_defaults);
    RUN_TEST(state_cleanup_null_safe);
    RUN_TEST(state_cleanup_frees_buffers);

    /* View new/free */
    RUN_TEST(view_new_no_callbacks);
    RUN_TEST(view_free_null_safe);

    /* Attach/detach */
    RUN_TEST(attach_null_safe);
    RUN_TEST(attach_sets_song);
    RUN_TEST(detach_clears_song);
    RUN_TEST(detach_null_safe);

    /* Theme */
    RUN_TEST(set_theme_null_safe);
    RUN_TEST(set_theme_by_name_null_safe);
    RUN_TEST(set_theme_by_name_invalid_returns_false);
    RUN_TEST(set_theme_by_name_valid);
    RUN_TEST(get_theme_null_safe);
    RUN_TEST(get_theme_returns_theme);

    /* Invalidation */
    RUN_TEST(invalidate_null_safe);
    RUN_TEST(invalidate_sets_all_flags);
    RUN_TEST(invalidate_cell_null_safe);
    RUN_TEST(invalidate_cell_sets_flag);
    RUN_TEST(invalidate_row_null_safe);
    RUN_TEST(invalidate_row_sets_flag);
    RUN_TEST(invalidate_track_null_safe);
    RUN_TEST(invalidate_track_sets_flag);
    RUN_TEST(invalidate_cursor_null_safe);
    RUN_TEST(invalidate_cursor_sets_flag);
    RUN_TEST(invalidate_selection_null_safe);
    RUN_TEST(invalidate_status_null_safe);

    /* Render */
    RUN_TEST(render_null_safe);
    RUN_TEST(render_clears_dirty_flags);
    RUN_TEST(render_skip_when_clean);

    /* Update playback */
    RUN_TEST(update_playback_null_safe);
    RUN_TEST(update_playback_updates_state);
    RUN_TEST(update_playback_no_change_no_dirty);

    /* Cursor movement */
    RUN_TEST(cursor_up_null_safe);
    RUN_TEST(cursor_up_moves_cursor);
    RUN_TEST(cursor_up_clamps_to_zero);
    RUN_TEST(cursor_down_null_safe);
    RUN_TEST(cursor_down_moves_cursor);
    RUN_TEST(cursor_down_clamps_to_max);
    RUN_TEST(cursor_left_null_safe);
    RUN_TEST(cursor_left_moves_cursor);
    RUN_TEST(cursor_left_clamps_to_zero);
    RUN_TEST(cursor_right_null_safe);
    RUN_TEST(cursor_right_moves_cursor);
    RUN_TEST(cursor_right_clamps_to_max);
    RUN_TEST(cursor_page_up_null_safe);
    RUN_TEST(cursor_page_up_moves_by_visible_rows);
    RUN_TEST(cursor_page_down_null_safe);
    RUN_TEST(cursor_page_down_moves_by_visible_rows);
    RUN_TEST(cursor_home_null_safe);
    RUN_TEST(cursor_home_moves_to_first_track);
    RUN_TEST(cursor_end_null_safe);
    RUN_TEST(cursor_end_moves_to_last_track);
    RUN_TEST(cursor_pattern_start_null_safe);
    RUN_TEST(cursor_pattern_start_moves_to_row_zero);
    RUN_TEST(cursor_pattern_end_null_safe);
    RUN_TEST(cursor_pattern_end_moves_to_last_row);
    RUN_TEST(cursor_goto_null_safe);
    RUN_TEST(cursor_goto_sets_position);

    /* Pattern navigation */
    RUN_TEST(next_pattern_null_safe);
    RUN_TEST(next_pattern_no_song_noop);
    RUN_TEST(next_pattern_wraps_around);
    RUN_TEST(prev_pattern_null_safe);
    RUN_TEST(prev_pattern_wraps_around);
    RUN_TEST(new_pattern_null_safe);
    RUN_TEST(new_pattern_creates_pattern);
    RUN_TEST(clone_pattern_null_safe);
    RUN_TEST(clone_pattern_copies_pattern);
    RUN_TEST(delete_pattern_null_safe);
    RUN_TEST(delete_pattern_cannot_delete_last);
    RUN_TEST(delete_pattern_removes_pattern);

    /* Track management */
    RUN_TEST(add_track_null_safe);
    RUN_TEST(add_track_adds_track);
    RUN_TEST(remove_track_null_safe);
    RUN_TEST(remove_track_cannot_remove_last);
    RUN_TEST(remove_track_removes_track);

    /* Mode switching */
    RUN_TEST(set_mode_null_safe);
    RUN_TEST(set_mode_changes_mode);
    RUN_TEST(enter_edit_null_safe);
    RUN_TEST(enter_edit_sets_mode);
    RUN_TEST(exit_edit_null_safe);
    RUN_TEST(exit_edit_cancel_returns_to_navigate);
    RUN_TEST(enter_command_null_safe);
    RUN_TEST(enter_command_sets_mode);

    /* Utility */
    RUN_TEST(get_cursor_cell_null_safe);
    RUN_TEST(get_cursor_cell_no_song_returns_null);
    RUN_TEST(get_cursor_cell_returns_cell);
    RUN_TEST(get_current_pattern_null_safe);
    RUN_TEST(get_current_pattern_returns_pattern);
    RUN_TEST(cursor_valid_null_returns_false);
    RUN_TEST(cursor_valid_no_song_returns_false);
    RUN_TEST(cursor_valid_returns_true);
    RUN_TEST(clamp_cursor_null_safe);
    RUN_TEST(clamp_cursor_clamps_values);
    RUN_TEST(clamp_cursor_clamps_negative);
    RUN_TEST(ensure_visible_null_safe);
    RUN_TEST(ensure_visible_adjusts_scroll);
    RUN_TEST(get_visible_range_null_safe);
    RUN_TEST(get_visible_range_returns_range);

    /* File path */
    RUN_TEST(set_file_path_null_safe);
    RUN_TEST(set_file_path_sets_path);
    RUN_TEST(get_file_path_null_safe);
    RUN_TEST(get_file_path_returns_path);

    /* Modified flag */
    RUN_TEST(is_modified_null_returns_false);
    RUN_TEST(is_modified_returns_flag);
    RUN_TEST(set_modified_null_safe);
    RUN_TEST(set_modified_sets_flag);

    /* Request quit */
    RUN_TEST(request_quit_null_safe);
    RUN_TEST(request_quit_sets_flag);

    /* Messages */
    RUN_TEST(show_status_null_safe);
    RUN_TEST(show_error_null_safe);
    RUN_TEST(clear_messages_null_safe);
    RUN_TEST(clear_messages_clears_all);

    /* Edit char */
    RUN_TEST(edit_char_null_safe);
    RUN_TEST(edit_char_not_in_edit_mode_noop);
    RUN_TEST(edit_char_adds_character);

    /* Undo/redo view integration */
    RUN_TEST(view_undo_null_safe);
    RUN_TEST(view_redo_null_safe);
    RUN_TEST(view_begin_undo_group_null_safe);
    RUN_TEST(view_end_undo_group_null_safe);

END_TEST_SUITE()
