/**
 * test_view_undo.c - Tests for tracker undo/redo system
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

/* Create a minimal view for testing (no callbacks needed for undo tests) */
static TrackerView* create_test_view(void) {
    TrackerView* view = calloc(1, sizeof(TrackerView));
    if (view) {
        tracker_view_state_init(&view->state);
        tracker_undo_init(&view->undo, 0);  /* unlimited */
    }
    return view;
}

static void free_test_view(TrackerView* view) {
    if (view) {
        tracker_view_state_cleanup(&view->state);
        tracker_undo_cleanup(&view->undo);
        free(view);
    }
}

/*============================================================================
 * Stack Initialization Tests
 *============================================================================*/

TEST(undo_init_basic) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    ASSERT_NULL(stack.undo_head);
    ASSERT_NULL(stack.redo_head);
    ASSERT_EQ(stack.undo_count, 0);
    ASSERT_EQ(stack.redo_count, 0);
    ASSERT_EQ(stack.max_undo, 0);
    ASSERT_EQ(stack.group_depth, 0);
    ASSERT_FALSE(stack.in_undo);

    tracker_undo_cleanup(&stack);
}

TEST(undo_init_with_limit) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 50);

    ASSERT_EQ(stack.max_undo, 50);

    tracker_undo_cleanup(&stack);
}

TEST(undo_init_null_safe) {
    /* Should not crash */
    tracker_undo_init(NULL, 0);
    tracker_undo_cleanup(NULL);
    tracker_undo_clear(NULL);
}

TEST(undo_cleanup_clears_stacks) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    /* Add some actions */
    tracker_undo_group_begin(&stack, "Test");
    tracker_undo_group_end(&stack);

    ASSERT_TRUE(stack.undo_count > 0);

    tracker_undo_cleanup(&stack);

    ASSERT_NULL(stack.undo_head);
    ASSERT_EQ(stack.undo_count, 0);
}

/*============================================================================
 * Can Undo/Redo Tests
 *============================================================================*/

TEST(undo_can_undo_empty) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    ASSERT_FALSE(tracker_undo_can_undo(&stack));

    tracker_undo_cleanup(&stack);
}

TEST(undo_can_undo_null) {
    ASSERT_FALSE(tracker_undo_can_undo(NULL));
}

TEST(undo_can_redo_empty) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    ASSERT_FALSE(tracker_undo_can_redo(&stack));

    tracker_undo_cleanup(&stack);
}

TEST(undo_can_redo_null) {
    ASSERT_FALSE(tracker_undo_can_redo(NULL));
}

TEST(undo_can_undo_with_action) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    /* Record a row insert action */
    tracker_undo_record_row_insert(&stack, NULL, 0, 0);

    ASSERT_TRUE(tracker_undo_can_undo(&stack));
    ASSERT_FALSE(tracker_undo_can_redo(&stack));

    tracker_undo_cleanup(&stack);
}

TEST(undo_can_undo_skips_group_markers) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    /* Only group markers, no real actions */
    tracker_undo_group_begin(&stack, "Empty");
    tracker_undo_group_end(&stack);

    /* Should return false since there are no real actions */
    ASSERT_FALSE(tracker_undo_can_undo(&stack));

    tracker_undo_cleanup(&stack);
}

/*============================================================================
 * Group Tests
 *============================================================================*/

TEST(undo_group_begin_end) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    ASSERT_EQ(stack.group_depth, 0);

    tracker_undo_group_begin(&stack, "Test Group");
    ASSERT_EQ(stack.group_depth, 1);

    tracker_undo_group_end(&stack);
    ASSERT_EQ(stack.group_depth, 0);

    tracker_undo_cleanup(&stack);
}

TEST(undo_group_nested) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    tracker_undo_group_begin(&stack, "Outer");
    ASSERT_EQ(stack.group_depth, 1);

    tracker_undo_group_begin(&stack, "Inner");
    ASSERT_EQ(stack.group_depth, 2);

    tracker_undo_group_end(&stack);
    ASSERT_EQ(stack.group_depth, 1);

    tracker_undo_group_end(&stack);
    ASSERT_EQ(stack.group_depth, 0);

    tracker_undo_cleanup(&stack);
}

TEST(undo_group_end_without_begin) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    /* Should be safe - no effect */
    tracker_undo_group_end(&stack);
    ASSERT_EQ(stack.group_depth, 0);

    tracker_undo_cleanup(&stack);
}

TEST(undo_group_null_safe) {
    tracker_undo_group_begin(NULL, "Test");
    tracker_undo_group_end(NULL);
}

/*============================================================================
 * Description Tests
 *============================================================================*/

TEST(undo_description_empty) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    ASSERT_NULL(tracker_undo_get_undo_description(&stack));
    ASSERT_NULL(tracker_undo_get_redo_description(&stack));

    tracker_undo_cleanup(&stack);
}

TEST(undo_description_null) {
    ASSERT_NULL(tracker_undo_get_undo_description(NULL));
    ASSERT_NULL(tracker_undo_get_redo_description(NULL));
}

TEST(undo_description_row_insert) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    tracker_undo_record_row_insert(&stack, NULL, 0, 0);

    const char* desc = tracker_undo_get_undo_description(&stack);
    ASSERT_NOT_NULL(desc);
    ASSERT_STR_EQ(desc, "Insert row");

    tracker_undo_cleanup(&stack);
}

TEST(undo_description_group) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    tracker_undo_group_begin(&stack, "Custom Group Action");
    tracker_undo_record_row_insert(&stack, NULL, 0, 0);
    tracker_undo_group_end(&stack);

    const char* desc = tracker_undo_get_undo_description(&stack);
    ASSERT_NOT_NULL(desc);
    ASSERT_STR_EQ(desc, "Custom Group Action");

    tracker_undo_cleanup(&stack);
}

/*============================================================================
 * Record Tests
 *============================================================================*/

TEST(undo_record_null_safe) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    tracker_undo_record(&stack, NULL);
    ASSERT_EQ(stack.undo_count, 0);

    tracker_undo_record(NULL, NULL);

    tracker_undo_cleanup(&stack);
}

TEST(undo_record_clears_redo) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    /* Record action */
    tracker_undo_record_row_insert(&stack, NULL, 0, 0);
    ASSERT_EQ(stack.undo_count, 1);

    /* Undo it */
    tracker_undo_undo(&stack, NULL, song);
    ASSERT_EQ(stack.undo_count, 0);
    ASSERT_EQ(stack.redo_count, 1);

    /* Record new action - should clear redo */
    tracker_undo_record_row_insert(&stack, NULL, 0, 1);
    ASSERT_EQ(stack.undo_count, 1);
    ASSERT_EQ(stack.redo_count, 0);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_record_max_limit) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 3);  /* Max 3 actions */

    /* Record 5 actions */
    for (int i = 0; i < 5; i++) {
        tracker_undo_record_row_insert(&stack, NULL, 0, i);
    }

    /* Should be limited to 3 */
    ASSERT_EQ(stack.undo_count, 3);

    tracker_undo_cleanup(&stack);
}

TEST(undo_record_unlimited) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);  /* Unlimited */

    /* Record many actions */
    for (int i = 0; i < 100; i++) {
        tracker_undo_record_row_insert(&stack, NULL, 0, i);
    }

    ASSERT_EQ(stack.undo_count, 100);

    tracker_undo_cleanup(&stack);
}

/*============================================================================
 * Cell Edit Recording Tests
 *============================================================================*/

TEST(undo_record_cell_edit) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    TrackerCell old_cell;
    tracker_cell_init(&old_cell);
    old_cell.type = TRACKER_CELL_EMPTY;

    TrackerCell new_cell;
    tracker_cell_init(&new_cell);
    new_cell.type = TRACKER_CELL_EXPRESSION;
    new_cell.expression = strdup("c4");

    tracker_undo_record_cell_edit(&stack, NULL, 0, 1, 2, &old_cell, &new_cell);

    ASSERT_EQ(stack.undo_count, 1);

    const char* desc = tracker_undo_get_undo_description(&stack);
    ASSERT_NOT_NULL(desc);
    ASSERT_STR_EQ(desc, "Edit cell");

    tracker_cell_clear(&old_cell);
    tracker_cell_clear(&new_cell);
    tracker_undo_cleanup(&stack);
}

TEST(undo_record_cell_edit_null_safe) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    TrackerCell cell;
    tracker_cell_init(&cell);

    tracker_undo_record_cell_edit(&stack, NULL, 0, 0, 0, NULL, &cell);
    tracker_undo_record_cell_edit(&stack, NULL, 0, 0, 0, &cell, NULL);
    tracker_undo_record_cell_edit(NULL, NULL, 0, 0, 0, &cell, &cell);

    ASSERT_EQ(stack.undo_count, 0);

    tracker_cell_clear(&cell);
    tracker_undo_cleanup(&stack);
}

TEST(undo_record_cell_edit_with_cursor) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    TrackerView* view = create_test_view();
    view->state.cursor_pattern = 2;
    view->state.cursor_track = 3;
    view->state.cursor_row = 4;

    TrackerCell old_cell, new_cell;
    tracker_cell_init(&old_cell);
    tracker_cell_init(&new_cell);

    tracker_undo_record_cell_edit(&stack, view, 0, 0, 0, &old_cell, &new_cell);

    ASSERT_EQ(stack.undo_count, 1);
    /* Cursor position is stored in the action */
    ASSERT_NOT_NULL(stack.undo_head);

    tracker_cell_clear(&old_cell);
    tracker_cell_clear(&new_cell);
    tracker_undo_cleanup(&stack);
    free_test_view(view);
}

/*============================================================================
 * Row Recording Tests
 *============================================================================*/

TEST(undo_record_row_insert) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    tracker_undo_record_row_insert(&stack, NULL, 0, 5);

    ASSERT_EQ(stack.undo_count, 1);

    const char* desc = tracker_undo_get_undo_description(&stack);
    ASSERT_STR_EQ(desc, "Insert row");

    tracker_undo_cleanup(&stack);
}

TEST(undo_record_row_insert_null_safe) {
    tracker_undo_record_row_insert(NULL, NULL, 0, 0);
}

TEST(undo_record_row_delete) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    TrackerCell cells[2];
    tracker_cell_init(&cells[0]);
    tracker_cell_init(&cells[1]);
    cells[0].type = TRACKER_CELL_EXPRESSION;
    cells[0].expression = strdup("c4");

    tracker_undo_record_row_delete(&stack, NULL, 0, 3, cells, 2);

    ASSERT_EQ(stack.undo_count, 1);

    const char* desc = tracker_undo_get_undo_description(&stack);
    ASSERT_STR_EQ(desc, "Delete row");

    tracker_cell_clear(&cells[0]);
    tracker_cell_clear(&cells[1]);
    tracker_undo_cleanup(&stack);
}

TEST(undo_record_row_delete_null_cells) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);

    tracker_undo_record_row_delete(&stack, NULL, 0, 3, NULL, 0);

    ASSERT_EQ(stack.undo_count, 1);

    tracker_undo_cleanup(&stack);
}

/*============================================================================
 * Undo Execution Tests
 *============================================================================*/

TEST(undo_undo_empty) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    bool result = tracker_undo_undo(&stack, NULL, song);
    ASSERT_FALSE(result);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_undo_null_safe) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    ASSERT_FALSE(tracker_undo_undo(NULL, NULL, song));
    ASSERT_FALSE(tracker_undo_undo(&stack, NULL, NULL));

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_undo_moves_to_redo) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    tracker_undo_record_row_insert(&stack, NULL, 0, 0);
    ASSERT_EQ(stack.undo_count, 1);
    ASSERT_EQ(stack.redo_count, 0);

    tracker_undo_undo(&stack, NULL, song);
    ASSERT_EQ(stack.undo_count, 0);
    ASSERT_EQ(stack.redo_count, 1);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_undo_cell_edit) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");
    TrackerPattern* pattern = tracker_pattern_new(16, 4, "Pattern 1");
    tracker_song_add_pattern(song, pattern);

    /* Get cell and set initial value */
    TrackerCell* cell = tracker_pattern_get_cell(pattern, 0, 0);
    TrackerCell old_state;
    tracker_cell_init(&old_state);
    old_state.type = cell->type;

    /* Modify cell */
    tracker_cell_set_expression(cell, "c4 d4 e4", "alda");

    TrackerCell new_state;
    tracker_cell_init(&new_state);
    new_state.type = cell->type;
    new_state.expression = strdup(cell->expression);

    /* Record the edit */
    tracker_undo_record_cell_edit(&stack, NULL, 0, 0, 0, &old_state, &new_state);

    /* Undo should restore old state */
    tracker_undo_undo(&stack, NULL, song);

    /* Cell should be back to empty */
    ASSERT_EQ(cell->type, TRACKER_CELL_EMPTY);

    tracker_cell_clear(&old_state);
    tracker_cell_clear(&new_state);
    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_undo_song_settings) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    /* Create settings change action manually */
    /* We need to use the internal action creation since there's no
       convenience function for song settings */
    TrackerUndoAction* action = calloc(1, sizeof(TrackerUndoAction));
    action->type = TRACKER_UNDO_SONG_SETTINGS;
    action->data.settings.old_bpm = 120;
    action->data.settings.new_bpm = 140;
    action->data.settings.old_rpb = 4;
    action->data.settings.new_rpb = 8;
    action->data.settings.old_tpr = 6;
    action->data.settings.new_tpr = 12;
    action->data.settings.old_spillover = TRACKER_SPILLOVER_LAYER;
    action->data.settings.new_spillover = TRACKER_SPILLOVER_TRUNCATE;

    /* Apply new settings to song */
    song->bpm = 140;
    song->rows_per_beat = 8;
    song->ticks_per_row = 12;
    song->spillover_mode = TRACKER_SPILLOVER_TRUNCATE;

    tracker_undo_record(&stack, action);

    /* Undo should restore old settings */
    tracker_undo_undo(&stack, NULL, song);

    ASSERT_EQ(song->bpm, 120);
    ASSERT_EQ(song->rows_per_beat, 4);
    ASSERT_EQ(song->ticks_per_row, 6);
    ASSERT_EQ(song->spillover_mode, TRACKER_SPILLOVER_LAYER);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_undo_restores_cursor) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");
    TrackerView* view = create_test_view();

    /* Set cursor position */
    view->state.cursor_pattern = 1;
    view->state.cursor_track = 2;
    view->state.cursor_row = 3;

    tracker_undo_record_row_insert(&stack, view, 0, 0);

    /* Move cursor somewhere else */
    view->state.cursor_pattern = 0;
    view->state.cursor_track = 0;
    view->state.cursor_row = 0;

    /* Undo should restore cursor */
    tracker_undo_undo(&stack, view, song);

    ASSERT_EQ(view->state.cursor_pattern, 1);
    ASSERT_EQ(view->state.cursor_track, 2);
    ASSERT_EQ(view->state.cursor_row, 3);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
    free_test_view(view);
}

TEST(undo_undo_grouped_actions) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    /* Record grouped actions */
    tracker_undo_group_begin(&stack, "Multi-row insert");
    tracker_undo_record_row_insert(&stack, NULL, 0, 0);
    tracker_undo_record_row_insert(&stack, NULL, 0, 1);
    tracker_undo_record_row_insert(&stack, NULL, 0, 2);
    tracker_undo_group_end(&stack);

    /* 5 items: GROUP_BEGIN + 3 inserts + GROUP_END */
    ASSERT_EQ(stack.undo_count, 5);

    /* Single undo should undo entire group */
    tracker_undo_undo(&stack, NULL, song);

    /* All should be on redo stack now */
    ASSERT_EQ(stack.undo_count, 0);
    ASSERT_EQ(stack.redo_count, 5);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

/*============================================================================
 * Redo Execution Tests
 *============================================================================*/

TEST(undo_redo_empty) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    bool result = tracker_undo_redo(&stack, NULL, song);
    ASSERT_FALSE(result);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_redo_null_safe) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    ASSERT_FALSE(tracker_undo_redo(NULL, NULL, song));
    ASSERT_FALSE(tracker_undo_redo(&stack, NULL, NULL));

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_redo_moves_to_undo) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    tracker_undo_record_row_insert(&stack, NULL, 0, 0);
    tracker_undo_undo(&stack, NULL, song);

    ASSERT_EQ(stack.undo_count, 0);
    ASSERT_EQ(stack.redo_count, 1);

    tracker_undo_redo(&stack, NULL, song);

    ASSERT_EQ(stack.undo_count, 1);
    ASSERT_EQ(stack.redo_count, 0);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_redo_cell_edit) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");
    TrackerPattern* pattern = tracker_pattern_new(16, 4, "Pattern 1");
    tracker_song_add_pattern(song, pattern);
    TrackerCell* cell = tracker_pattern_get_cell(pattern, 0, 0);

    TrackerCell old_state, new_state;
    tracker_cell_init(&old_state);
    tracker_cell_init(&new_state);
    new_state.type = TRACKER_CELL_EXPRESSION;
    new_state.expression = strdup("c4");

    /* Modify cell */
    tracker_cell_set_expression(cell, "c4", NULL);

    tracker_undo_record_cell_edit(&stack, NULL, 0, 0, 0, &old_state, &new_state);

    /* Undo */
    tracker_undo_undo(&stack, NULL, song);
    ASSERT_EQ(cell->type, TRACKER_CELL_EMPTY);

    /* Redo */
    tracker_undo_redo(&stack, NULL, song);
    ASSERT_EQ(cell->type, TRACKER_CELL_EXPRESSION);
    ASSERT_STR_EQ(cell->expression, "c4");

    tracker_cell_clear(&old_state);
    tracker_cell_clear(&new_state);
    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_redo_song_settings) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    TrackerUndoAction* action = calloc(1, sizeof(TrackerUndoAction));
    action->type = TRACKER_UNDO_SONG_SETTINGS;
    action->data.settings.old_bpm = 120;
    action->data.settings.new_bpm = 180;
    action->data.settings.old_rpb = 4;
    action->data.settings.new_rpb = 4;
    action->data.settings.old_tpr = 6;
    action->data.settings.new_tpr = 6;

    song->bpm = 180;
    tracker_undo_record(&stack, action);

    /* Undo */
    tracker_undo_undo(&stack, NULL, song);
    ASSERT_EQ(song->bpm, 120);

    /* Redo */
    tracker_undo_redo(&stack, NULL, song);
    ASSERT_EQ(song->bpm, 180);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_redo_grouped_actions) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    tracker_undo_group_begin(&stack, "Group");
    tracker_undo_record_row_insert(&stack, NULL, 0, 0);
    tracker_undo_record_row_insert(&stack, NULL, 0, 1);
    tracker_undo_group_end(&stack);

    /* Undo entire group */
    tracker_undo_undo(&stack, NULL, song);
    ASSERT_EQ(stack.undo_count, 0);
    ASSERT_EQ(stack.redo_count, 4);  /* GROUP_BEGIN + 2 inserts + GROUP_END */

    /* Redo entire group */
    tracker_undo_redo(&stack, NULL, song);
    ASSERT_EQ(stack.undo_count, 4);
    ASSERT_EQ(stack.redo_count, 0);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

/*============================================================================
 * In-Undo Flag Tests
 *============================================================================*/

TEST(undo_in_undo_prevents_recording) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    tracker_undo_record_row_insert(&stack, NULL, 0, 0);
    ASSERT_EQ(stack.undo_count, 1);

    /* Manually set in_undo flag */
    stack.in_undo = true;

    /* This should be ignored */
    tracker_undo_record_row_insert(&stack, NULL, 0, 1);
    ASSERT_EQ(stack.undo_count, 1);  /* Still 1 */

    stack.in_undo = false;

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

/*============================================================================
 * Multiple Undo/Redo Cycles
 *============================================================================*/

TEST(undo_multiple_undo_redo_cycles) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    /* Record 3 actions */
    tracker_undo_record_row_insert(&stack, NULL, 0, 0);
    tracker_undo_record_row_insert(&stack, NULL, 0, 1);
    tracker_undo_record_row_insert(&stack, NULL, 0, 2);
    ASSERT_EQ(stack.undo_count, 3);

    /* Undo all */
    tracker_undo_undo(&stack, NULL, song);
    tracker_undo_undo(&stack, NULL, song);
    tracker_undo_undo(&stack, NULL, song);
    ASSERT_EQ(stack.undo_count, 0);
    ASSERT_EQ(stack.redo_count, 3);

    /* Redo 2 */
    tracker_undo_redo(&stack, NULL, song);
    tracker_undo_redo(&stack, NULL, song);
    ASSERT_EQ(stack.undo_count, 2);
    ASSERT_EQ(stack.redo_count, 1);

    /* Undo 1 */
    tracker_undo_undo(&stack, NULL, song);
    ASSERT_EQ(stack.undo_count, 1);
    ASSERT_EQ(stack.redo_count, 2);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

TEST(undo_redo_after_new_action_clears_redo) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    tracker_undo_record_row_insert(&stack, NULL, 0, 0);
    tracker_undo_undo(&stack, NULL, song);
    ASSERT_EQ(stack.redo_count, 1);

    /* New action should clear redo */
    tracker_undo_record_row_insert(&stack, NULL, 0, 1);
    ASSERT_EQ(stack.redo_count, 0);

    tracker_undo_cleanup(&stack);
    tracker_song_free(song);
}

/*============================================================================
 * Action Free Tests
 *============================================================================*/

TEST(undo_action_free_null_safe) {
    tracker_undo_action_free(NULL);
}

/*============================================================================
 * Clear Tests
 *============================================================================*/

TEST(undo_clear_resets_all) {
    TrackerUndoStack stack;
    tracker_undo_init(&stack, 0);
    TrackerSong* song = tracker_song_new("Test");

    tracker_undo_record_row_insert(&stack, NULL, 0, 0);
    tracker_undo_undo(&stack, NULL, song);

    ASSERT_TRUE(stack.redo_count > 0);

    tracker_undo_clear(&stack);

    ASSERT_NULL(stack.undo_head);
    ASSERT_NULL(stack.redo_head);
    ASSERT_EQ(stack.undo_count, 0);
    ASSERT_EQ(stack.redo_count, 0);
    ASSERT_EQ(stack.group_depth, 0);

    tracker_song_free(song);
}

/*============================================================================
 * Main
 *============================================================================*/

BEGIN_TEST_SUITE("tracker_view_undo")
    /* Stack initialization */
    RUN_TEST(undo_init_basic);
    RUN_TEST(undo_init_with_limit);
    RUN_TEST(undo_init_null_safe);
    RUN_TEST(undo_cleanup_clears_stacks);

    /* Can undo/redo */
    RUN_TEST(undo_can_undo_empty);
    RUN_TEST(undo_can_undo_null);
    RUN_TEST(undo_can_redo_empty);
    RUN_TEST(undo_can_redo_null);
    RUN_TEST(undo_can_undo_with_action);
    RUN_TEST(undo_can_undo_skips_group_markers);

    /* Groups */
    RUN_TEST(undo_group_begin_end);
    RUN_TEST(undo_group_nested);
    RUN_TEST(undo_group_end_without_begin);
    RUN_TEST(undo_group_null_safe);

    /* Descriptions */
    RUN_TEST(undo_description_empty);
    RUN_TEST(undo_description_null);
    RUN_TEST(undo_description_row_insert);
    RUN_TEST(undo_description_group);

    /* Recording */
    RUN_TEST(undo_record_null_safe);
    RUN_TEST(undo_record_clears_redo);
    RUN_TEST(undo_record_max_limit);
    RUN_TEST(undo_record_unlimited);

    /* Cell edit recording */
    RUN_TEST(undo_record_cell_edit);
    RUN_TEST(undo_record_cell_edit_null_safe);
    RUN_TEST(undo_record_cell_edit_with_cursor);

    /* Row recording */
    RUN_TEST(undo_record_row_insert);
    RUN_TEST(undo_record_row_insert_null_safe);
    RUN_TEST(undo_record_row_delete);
    RUN_TEST(undo_record_row_delete_null_cells);

    /* Undo execution */
    RUN_TEST(undo_undo_empty);
    RUN_TEST(undo_undo_null_safe);
    RUN_TEST(undo_undo_moves_to_redo);
    RUN_TEST(undo_undo_cell_edit);
    RUN_TEST(undo_undo_song_settings);
    RUN_TEST(undo_undo_restores_cursor);
    RUN_TEST(undo_undo_grouped_actions);

    /* Redo execution */
    RUN_TEST(undo_redo_empty);
    RUN_TEST(undo_redo_null_safe);
    RUN_TEST(undo_redo_moves_to_undo);
    RUN_TEST(undo_redo_cell_edit);
    RUN_TEST(undo_redo_song_settings);
    RUN_TEST(undo_redo_grouped_actions);

    /* In-undo flag */
    RUN_TEST(undo_in_undo_prevents_recording);

    /* Multiple cycles */
    RUN_TEST(undo_multiple_undo_redo_cycles);
    RUN_TEST(undo_redo_after_new_action_clears_redo);

    /* Action free */
    RUN_TEST(undo_action_free_null_safe);

    /* Clear */
    RUN_TEST(undo_clear_resets_all);
END_TEST_SUITE()
