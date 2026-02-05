/**
 * @file test_model.c
 * @brief Tests for tracker data model (tracker_model.c).
 *
 * Tests verify:
 * - Event params lifecycle
 * - Phrase creation, events, and cloning
 * - FX chain operations
 * - Cell operations
 * - Track lifecycle and resize
 * - Pattern operations
 * - Song management
 * - Phrase library
 * - Utility functions
 */

#include "test_framework.h"
#include "tracker_model.h"
#include <string.h>

test_stats_t test_stats;

/* ============================================================================
 * Event Params Tests
 * ============================================================================ */

TEST(event_params_new_defaults) {
    TrackerEventParams *params = tracker_event_params_new();
    ASSERT_NOT_NULL(params);

    /* Default probability is 100% */
    ASSERT_EQ(params->probability, 100);

    tracker_event_params_free(params);
}

TEST(event_params_free_null_safe) {
    tracker_event_params_free(NULL);
}

TEST(event_params_clone) {
    TrackerEventParams *params = tracker_event_params_new();
    ASSERT_NOT_NULL(params);

    params->probability = 75;
    params->humanize_time_amt = 20;
    params->accent_boost = 30;

    TrackerEventParams *clone = tracker_event_params_clone(params);
    ASSERT_NOT_NULL(clone);
    ASSERT_EQ(clone->probability, 75);
    ASSERT_EQ(clone->humanize_time_amt, 20);
    ASSERT_EQ(clone->accent_boost, 30);

    tracker_event_params_free(params);
    tracker_event_params_free(clone);
}

TEST(event_params_clone_null) {
    TrackerEventParams *clone = tracker_event_params_clone(NULL);
    ASSERT_NULL(clone);
}

/* ============================================================================
 * Phrase Tests
 * ============================================================================ */

TEST(phrase_new_with_capacity) {
    TrackerPhrase *phrase = tracker_phrase_new(16);
    ASSERT_NOT_NULL(phrase);
    ASSERT_NOT_NULL(phrase->events);
    ASSERT_EQ(phrase->count, 0);
    ASSERT_EQ(phrase->capacity, 16);

    tracker_phrase_free(phrase);
}

TEST(phrase_new_zero_capacity) {
    TrackerPhrase *phrase = tracker_phrase_new(0);
    ASSERT_NOT_NULL(phrase);
    ASSERT_NULL(phrase->events);
    ASSERT_EQ(phrase->count, 0);
    ASSERT_EQ(phrase->capacity, 0);

    tracker_phrase_free(phrase);
}

TEST(phrase_free_null_safe) {
    tracker_phrase_free(NULL);
}

TEST(phrase_add_event) {
    TrackerPhrase *phrase = tracker_phrase_new(8);
    ASSERT_NOT_NULL(phrase);

    TrackerEvent event = {
        .offset_rows = 0,
        .offset_ticks = 0,
        .type = TRACKER_EVENT_NOTE_ON,
        .channel = 0,
        .data1 = 60,  /* C4 */
        .data2 = 100, /* velocity */
        .gate_rows = 1,
        .gate_ticks = 0,
        .flags = TRACKER_FLAG_NONE,
        .params = NULL
    };

    bool result = tracker_phrase_add_event(phrase, &event);
    ASSERT_TRUE(result);
    ASSERT_EQ(phrase->count, 1);
    ASSERT_EQ(phrase->events[0].data1, 60);
    ASSERT_EQ(phrase->events[0].data2, 100);

    tracker_phrase_free(phrase);
}

TEST(phrase_add_event_with_params) {
    TrackerPhrase *phrase = tracker_phrase_new(8);
    ASSERT_NOT_NULL(phrase);

    TrackerEventParams *params = tracker_event_params_new();
    params->probability = 50;

    TrackerEvent event = {
        .type = TRACKER_EVENT_NOTE_ON,
        .data1 = 72,
        .data2 = 80,
        .flags = TRACKER_FLAG_PROBABILITY,
        .params = params
    };

    bool result = tracker_phrase_add_event(phrase, &event);
    ASSERT_TRUE(result);

    /* Params should be deep copied */
    ASSERT_NOT_NULL(phrase->events[0].params);
    ASSERT_TRUE(phrase->events[0].params != params);  /* Different pointer */
    ASSERT_EQ(phrase->events[0].params->probability, 50);

    tracker_event_params_free(params);
    tracker_phrase_free(phrase);
}

TEST(phrase_add_event_grows_capacity) {
    TrackerPhrase *phrase = tracker_phrase_new(0);
    ASSERT_NOT_NULL(phrase);

    TrackerEvent event = {
        .type = TRACKER_EVENT_NOTE_ON,
        .data1 = 60,
        .data2 = 100
    };

    /* Add events to trigger growth */
    for (int i = 0; i < 20; i++) {
        event.data1 = 60 + i;
        bool result = tracker_phrase_add_event(phrase, &event);
        ASSERT_TRUE(result);
    }

    ASSERT_EQ(phrase->count, 20);
    ASSERT_TRUE(phrase->capacity >= 20);

    tracker_phrase_free(phrase);
}

TEST(phrase_add_event_null_phrase) {
    TrackerEvent event = { .type = TRACKER_EVENT_NOTE_ON };
    bool result = tracker_phrase_add_event(NULL, &event);
    ASSERT_FALSE(result);
}

TEST(phrase_add_event_null_event) {
    TrackerPhrase *phrase = tracker_phrase_new(8);
    bool result = tracker_phrase_add_event(phrase, NULL);
    ASSERT_FALSE(result);
    tracker_phrase_free(phrase);
}

TEST(phrase_clear) {
    TrackerPhrase *phrase = tracker_phrase_new(8);
    ASSERT_NOT_NULL(phrase);

    TrackerEvent event = { .type = TRACKER_EVENT_NOTE_ON, .data1 = 60 };
    tracker_phrase_add_event(phrase, &event);
    tracker_phrase_add_event(phrase, &event);
    ASSERT_EQ(phrase->count, 2);

    tracker_phrase_clear(phrase);
    ASSERT_EQ(phrase->count, 0);
    /* Capacity should remain */
    ASSERT_TRUE(phrase->capacity > 0);

    tracker_phrase_free(phrase);
}

TEST(phrase_clear_null_safe) {
    tracker_phrase_clear(NULL);
}

TEST(phrase_clone) {
    TrackerPhrase *phrase = tracker_phrase_new(8);
    ASSERT_NOT_NULL(phrase);

    TrackerEvent event1 = { .type = TRACKER_EVENT_NOTE_ON, .data1 = 60, .data2 = 100 };
    TrackerEvent event2 = { .type = TRACKER_EVENT_NOTE_OFF, .data1 = 60 };
    tracker_phrase_add_event(phrase, &event1);
    tracker_phrase_add_event(phrase, &event2);

    TrackerPhrase *clone = tracker_phrase_clone(phrase);
    ASSERT_NOT_NULL(clone);
    ASSERT_EQ(clone->count, 2);
    ASSERT_EQ(clone->events[0].data1, 60);
    ASSERT_EQ(clone->events[0].type, TRACKER_EVENT_NOTE_ON);
    ASSERT_EQ(clone->events[1].type, TRACKER_EVENT_NOTE_OFF);

    tracker_phrase_free(phrase);
    tracker_phrase_free(clone);
}

TEST(phrase_clone_null) {
    TrackerPhrase *clone = tracker_phrase_clone(NULL);
    ASSERT_NULL(clone);
}

/* ============================================================================
 * FX Chain Tests
 * ============================================================================ */

TEST(fx_chain_init) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    ASSERT_NULL(chain.entries);
    ASSERT_EQ(chain.count, 0);
    ASSERT_EQ(chain.capacity, 0);
}

TEST(fx_chain_init_null_safe) {
    tracker_fx_chain_init(NULL);
}

TEST(fx_chain_append) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    bool result = tracker_fx_chain_append(&chain, "transpose", "+12", NULL);
    ASSERT_TRUE(result);
    ASSERT_EQ(chain.count, 1);
    ASSERT_STR_EQ(chain.entries[0].name, "transpose");
    ASSERT_STR_EQ(chain.entries[0].params, "+12");
    ASSERT_TRUE(chain.entries[0].enabled);

    tracker_fx_chain_clear(&chain);
}

TEST(fx_chain_append_multiple) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    tracker_fx_chain_append(&chain, "transpose", "+12", NULL);
    tracker_fx_chain_append(&chain, "velocity", "80", NULL);
    tracker_fx_chain_append(&chain, "humanize", "10", NULL);

    ASSERT_EQ(chain.count, 3);
    ASSERT_STR_EQ(chain.entries[0].name, "transpose");
    ASSERT_STR_EQ(chain.entries[1].name, "velocity");
    ASSERT_STR_EQ(chain.entries[2].name, "humanize");

    tracker_fx_chain_clear(&chain);
}

TEST(fx_chain_append_null_name) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    bool result = tracker_fx_chain_append(&chain, NULL, NULL, NULL);
    ASSERT_FALSE(result);
    ASSERT_EQ(chain.count, 0);
}

TEST(fx_chain_insert) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    tracker_fx_chain_append(&chain, "first", NULL, NULL);
    tracker_fx_chain_append(&chain, "third", NULL, NULL);

    bool result = tracker_fx_chain_insert(&chain, 1, "second", NULL, NULL);
    ASSERT_TRUE(result);
    ASSERT_EQ(chain.count, 3);
    ASSERT_STR_EQ(chain.entries[0].name, "first");
    ASSERT_STR_EQ(chain.entries[1].name, "second");
    ASSERT_STR_EQ(chain.entries[2].name, "third");

    tracker_fx_chain_clear(&chain);
}

TEST(fx_chain_insert_invalid_index) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    bool result = tracker_fx_chain_insert(&chain, -1, "test", NULL, NULL);
    ASSERT_FALSE(result);

    result = tracker_fx_chain_insert(&chain, 10, "test", NULL, NULL);
    ASSERT_FALSE(result);
}

TEST(fx_chain_remove) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    tracker_fx_chain_append(&chain, "first", NULL, NULL);
    tracker_fx_chain_append(&chain, "second", NULL, NULL);
    tracker_fx_chain_append(&chain, "third", NULL, NULL);

    bool result = tracker_fx_chain_remove(&chain, 1);
    ASSERT_TRUE(result);
    ASSERT_EQ(chain.count, 2);
    ASSERT_STR_EQ(chain.entries[0].name, "first");
    ASSERT_STR_EQ(chain.entries[1].name, "third");

    tracker_fx_chain_clear(&chain);
}

TEST(fx_chain_remove_invalid_index) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    bool result = tracker_fx_chain_remove(&chain, 0);
    ASSERT_FALSE(result);

    result = tracker_fx_chain_remove(&chain, -1);
    ASSERT_FALSE(result);
}

TEST(fx_chain_move) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    tracker_fx_chain_append(&chain, "a", NULL, NULL);
    tracker_fx_chain_append(&chain, "b", NULL, NULL);
    tracker_fx_chain_append(&chain, "c", NULL, NULL);

    /* Move "a" from index 0 to index 2 */
    bool result = tracker_fx_chain_move(&chain, 0, 2);
    ASSERT_TRUE(result);
    ASSERT_STR_EQ(chain.entries[0].name, "b");
    ASSERT_STR_EQ(chain.entries[1].name, "c");
    ASSERT_STR_EQ(chain.entries[2].name, "a");

    tracker_fx_chain_clear(&chain);
}

TEST(fx_chain_move_same_index) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    tracker_fx_chain_append(&chain, "a", NULL, NULL);

    bool result = tracker_fx_chain_move(&chain, 0, 0);
    ASSERT_TRUE(result);  /* No-op, but should succeed */

    tracker_fx_chain_clear(&chain);
}

TEST(fx_chain_set_enabled) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    tracker_fx_chain_append(&chain, "test", NULL, NULL);
    ASSERT_TRUE(chain.entries[0].enabled);

    tracker_fx_chain_set_enabled(&chain, 0, false);
    ASSERT_FALSE(chain.entries[0].enabled);

    tracker_fx_chain_set_enabled(&chain, 0, true);
    ASSERT_TRUE(chain.entries[0].enabled);

    tracker_fx_chain_clear(&chain);
}

TEST(fx_chain_get) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    tracker_fx_chain_append(&chain, "test", "params", "lang");

    TrackerFxEntry *entry = tracker_fx_chain_get(&chain, 0);
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->name, "test");

    entry = tracker_fx_chain_get(&chain, 1);
    ASSERT_NULL(entry);

    entry = tracker_fx_chain_get(&chain, -1);
    ASSERT_NULL(entry);

    tracker_fx_chain_clear(&chain);
}

TEST(fx_chain_clone) {
    TrackerFxChain src, dest;
    tracker_fx_chain_init(&src);
    tracker_fx_chain_init(&dest);

    tracker_fx_chain_append(&src, "transpose", "+12", NULL);
    tracker_fx_chain_append(&src, "velocity", "80", NULL);
    src.entries[1].enabled = false;

    bool result = tracker_fx_chain_clone(&dest, &src);
    ASSERT_TRUE(result);
    ASSERT_EQ(dest.count, 2);
    ASSERT_STR_EQ(dest.entries[0].name, "transpose");
    ASSERT_STR_EQ(dest.entries[1].name, "velocity");
    ASSERT_FALSE(dest.entries[1].enabled);

    tracker_fx_chain_clear(&src);
    tracker_fx_chain_clear(&dest);
}

TEST(fx_chain_clear_null_safe) {
    tracker_fx_chain_clear(NULL);
}

/* ============================================================================
 * Cell Tests
 * ============================================================================ */

TEST(cell_init) {
    TrackerCell cell;
    tracker_cell_init(&cell);

    ASSERT_EQ(cell.type, TRACKER_CELL_EMPTY);
    ASSERT_NULL(cell.expression);
    ASSERT_NULL(cell.language_id);
    ASSERT_FALSE(cell.dirty);
}

TEST(cell_init_null_safe) {
    tracker_cell_init(NULL);
}

TEST(cell_set_expression) {
    TrackerCell cell;
    tracker_cell_init(&cell);

    tracker_cell_set_expression(&cell, "C4 E4 G4", "alda");

    ASSERT_EQ(cell.type, TRACKER_CELL_EXPRESSION);
    ASSERT_STR_EQ(cell.expression, "C4 E4 G4");
    ASSERT_STR_EQ(cell.language_id, "alda");
    ASSERT_TRUE(cell.dirty);

    tracker_cell_clear(&cell);
}

TEST(cell_set_expression_empty) {
    TrackerCell cell;
    tracker_cell_init(&cell);

    tracker_cell_set_expression(&cell, "", NULL);
    ASSERT_EQ(cell.type, TRACKER_CELL_EMPTY);

    tracker_cell_set_expression(&cell, NULL, NULL);
    ASSERT_EQ(cell.type, TRACKER_CELL_EMPTY);

    tracker_cell_clear(&cell);
}

TEST(cell_mark_dirty) {
    TrackerCell cell;
    tracker_cell_init(&cell);

    ASSERT_FALSE(cell.dirty);
    tracker_cell_mark_dirty(&cell);
    ASSERT_TRUE(cell.dirty);
}

TEST(cell_mark_dirty_null_safe) {
    tracker_cell_mark_dirty(NULL);
}

TEST(cell_clone) {
    TrackerCell src, dest;
    tracker_cell_init(&src);

    tracker_cell_set_expression(&src, "test", "lang");
    tracker_fx_chain_append(&src.fx_chain, "fx", NULL, NULL);

    bool result = tracker_cell_clone(&dest, &src);
    ASSERT_TRUE(result);
    ASSERT_EQ(dest.type, TRACKER_CELL_EXPRESSION);
    ASSERT_STR_EQ(dest.expression, "test");
    ASSERT_STR_EQ(dest.language_id, "lang");
    ASSERT_EQ(dest.fx_chain.count, 1);
    ASSERT_TRUE(dest.dirty);  /* Clone needs compilation */

    tracker_cell_clear(&src);
    tracker_cell_clear(&dest);
}

TEST(cell_clear_null_safe) {
    tracker_cell_clear(NULL);
}

/* ============================================================================
 * Track Tests
 * ============================================================================ */

TEST(track_new) {
    TrackerTrack *track = tracker_track_new(16, "Lead", 0);
    ASSERT_NOT_NULL(track);
    ASSERT_STR_EQ(track->name, "Lead");
    ASSERT_EQ(track->default_channel, 0);
    ASSERT_EQ(track->volume, 100);
    ASSERT_EQ(track->pan, 0);
    ASSERT_FALSE(track->muted);
    ASSERT_FALSE(track->solo);
    ASSERT_NOT_NULL(track->cells);

    tracker_track_free(track, 16);
}

TEST(track_new_no_name) {
    TrackerTrack *track = tracker_track_new(8, NULL, 5);
    ASSERT_NOT_NULL(track);
    ASSERT_NULL(track->name);
    ASSERT_EQ(track->default_channel, 5);

    tracker_track_free(track, 8);
}

TEST(track_free_null_safe) {
    tracker_track_free(NULL, 0);
}

TEST(track_resize_grow) {
    TrackerTrack *track = tracker_track_new(8, "Test", 0);
    ASSERT_NOT_NULL(track);

    tracker_track_resize(track, 8, 16);
    /* Can't directly verify size, but shouldn't crash */

    tracker_track_free(track, 16);
}

TEST(track_resize_shrink) {
    TrackerTrack *track = tracker_track_new(16, "Test", 0);
    ASSERT_NOT_NULL(track);

    /* Set expression on cell that will be removed */
    tracker_cell_set_expression(&track->cells[15], "test", NULL);

    tracker_track_resize(track, 16, 8);
    /* Cell 15 should be cleared */

    tracker_track_free(track, 8);
}

TEST(track_resize_to_zero) {
    TrackerTrack *track = tracker_track_new(8, "Test", 0);
    ASSERT_NOT_NULL(track);

    tracker_track_resize(track, 8, 0);
    ASSERT_NULL(track->cells);

    tracker_track_free(track, 0);
}

/* ============================================================================
 * Pattern Tests
 * ============================================================================ */

TEST(pattern_new) {
    TrackerPattern *pattern = tracker_pattern_new(64, 4, "Intro");
    ASSERT_NOT_NULL(pattern);
    ASSERT_STR_EQ(pattern->name, "Intro");
    ASSERT_EQ(pattern->num_rows, 64);
    ASSERT_EQ(pattern->num_tracks, 4);

    tracker_pattern_free(pattern);
}

TEST(pattern_free_null_safe) {
    tracker_pattern_free(NULL);
}

TEST(pattern_get_cell) {
    TrackerPattern *pattern = tracker_pattern_new(16, 4, NULL);
    ASSERT_NOT_NULL(pattern);

    TrackerCell *cell = tracker_pattern_get_cell(pattern, 0, 0);
    ASSERT_NOT_NULL(cell);
    ASSERT_EQ(cell->type, TRACKER_CELL_EMPTY);

    cell = tracker_pattern_get_cell(pattern, 15, 3);
    ASSERT_NOT_NULL(cell);

    /* Invalid indices */
    cell = tracker_pattern_get_cell(pattern, -1, 0);
    ASSERT_NULL(cell);

    cell = tracker_pattern_get_cell(pattern, 0, -1);
    ASSERT_NULL(cell);

    cell = tracker_pattern_get_cell(pattern, 100, 0);
    ASSERT_NULL(cell);

    cell = tracker_pattern_get_cell(pattern, 0, 100);
    ASSERT_NULL(cell);

    tracker_pattern_free(pattern);
}

TEST(pattern_add_track) {
    TrackerPattern *pattern = tracker_pattern_new(16, 2, NULL);
    ASSERT_NOT_NULL(pattern);
    ASSERT_EQ(pattern->num_tracks, 2);

    bool result = tracker_pattern_add_track(pattern, "New Track", 3);
    ASSERT_TRUE(result);
    ASSERT_EQ(pattern->num_tracks, 3);

    tracker_pattern_free(pattern);
}

TEST(pattern_remove_track) {
    TrackerPattern *pattern = tracker_pattern_new(16, 3, NULL);
    ASSERT_NOT_NULL(pattern);

    bool result = tracker_pattern_remove_track(pattern, 1);
    ASSERT_TRUE(result);
    ASSERT_EQ(pattern->num_tracks, 2);

    /* Invalid index */
    result = tracker_pattern_remove_track(pattern, 10);
    ASSERT_FALSE(result);

    tracker_pattern_free(pattern);
}

TEST(pattern_set_rows) {
    TrackerPattern *pattern = tracker_pattern_new(16, 2, NULL);
    ASSERT_NOT_NULL(pattern);

    tracker_pattern_set_rows(pattern, 32);
    ASSERT_EQ(pattern->num_rows, 32);

    tracker_pattern_set_rows(pattern, 8);
    ASSERT_EQ(pattern->num_rows, 8);

    tracker_pattern_free(pattern);
}

/* ============================================================================
 * Song Tests
 * ============================================================================ */

TEST(song_new) {
    TrackerSong *song = tracker_song_new("My Song");
    ASSERT_NOT_NULL(song);
    ASSERT_STR_EQ(song->name, "My Song");
    ASSERT_EQ(song->bpm, TRACKER_DEFAULT_BPM);
    ASSERT_EQ(song->rows_per_beat, TRACKER_DEFAULT_RPB);
    ASSERT_EQ(song->ticks_per_row, TRACKER_DEFAULT_TPR);
    ASSERT_EQ(song->num_patterns, 0);
    ASSERT_EQ(song->sequence_length, 0);

    tracker_song_free(song);
}

TEST(song_free_null_safe) {
    tracker_song_free(NULL);
}

TEST(song_add_pattern) {
    TrackerSong *song = tracker_song_new("Test");
    ASSERT_NOT_NULL(song);

    TrackerPattern *pattern = tracker_pattern_new(16, 4, "Pattern 1");
    int index = tracker_song_add_pattern(song, pattern);
    ASSERT_EQ(index, 0);
    ASSERT_EQ(song->num_patterns, 1);

    TrackerPattern *pattern2 = tracker_pattern_new(16, 4, "Pattern 2");
    index = tracker_song_add_pattern(song, pattern2);
    ASSERT_EQ(index, 1);
    ASSERT_EQ(song->num_patterns, 2);

    tracker_song_free(song);
}

TEST(song_add_pattern_null) {
    TrackerSong *song = tracker_song_new("Test");
    int index = tracker_song_add_pattern(song, NULL);
    ASSERT_EQ(index, -1);

    index = tracker_song_add_pattern(NULL, NULL);
    ASSERT_EQ(index, -1);

    tracker_song_free(song);
}

TEST(song_get_pattern) {
    TrackerSong *song = tracker_song_new("Test");
    TrackerPattern *pattern = tracker_pattern_new(16, 4, "Test Pattern");
    tracker_song_add_pattern(song, pattern);

    TrackerPattern *retrieved = tracker_song_get_pattern(song, 0);
    ASSERT_TRUE(retrieved == pattern);

    retrieved = tracker_song_get_pattern(song, 1);
    ASSERT_NULL(retrieved);

    retrieved = tracker_song_get_pattern(song, -1);
    ASSERT_NULL(retrieved);

    tracker_song_free(song);
}

TEST(song_remove_pattern) {
    TrackerSong *song = tracker_song_new("Test");
    tracker_song_add_pattern(song, tracker_pattern_new(16, 4, "P1"));
    tracker_song_add_pattern(song, tracker_pattern_new(16, 4, "P2"));
    ASSERT_EQ(song->num_patterns, 2);

    bool result = tracker_song_remove_pattern(song, 0);
    ASSERT_TRUE(result);
    ASSERT_EQ(song->num_patterns, 1);

    result = tracker_song_remove_pattern(song, 10);
    ASSERT_FALSE(result);

    tracker_song_free(song);
}

TEST(song_append_to_sequence) {
    TrackerSong *song = tracker_song_new("Test");
    TrackerPattern *pattern = tracker_pattern_new(16, 4, NULL);
    tracker_song_add_pattern(song, pattern);

    bool result = tracker_song_append_to_sequence(song, 0, 1);
    ASSERT_TRUE(result);
    ASSERT_EQ(song->sequence_length, 1);
    ASSERT_EQ(song->sequence[0].pattern_index, 0);
    ASSERT_EQ(song->sequence[0].repeat_count, 1);

    result = tracker_song_append_to_sequence(song, 0, 4);
    ASSERT_TRUE(result);
    ASSERT_EQ(song->sequence_length, 2);
    ASSERT_EQ(song->sequence[1].repeat_count, 4);

    tracker_song_free(song);
}

TEST(song_append_to_sequence_invalid) {
    TrackerSong *song = tracker_song_new("Test");

    /* Invalid pattern index */
    bool result = tracker_song_append_to_sequence(song, 0, 1);
    ASSERT_FALSE(result);

    result = tracker_song_append_to_sequence(song, -1, 1);
    ASSERT_FALSE(result);

    tracker_song_free(song);
}

/* ============================================================================
 * Phrase Library Tests
 * ============================================================================ */

TEST(phrase_library_init) {
    TrackerPhraseLibrary lib;
    tracker_phrase_library_init(&lib);

    ASSERT_NULL(lib.entries);
    ASSERT_EQ(lib.count, 0);
    ASSERT_EQ(lib.capacity, 0);
}

TEST(phrase_library_add) {
    TrackerPhraseLibrary lib;
    tracker_phrase_library_init(&lib);

    bool result = tracker_phrase_library_add(&lib, "chord", "C4 E4 G4", "alda");
    ASSERT_TRUE(result);
    ASSERT_EQ(lib.count, 1);

    TrackerPhraseEntry *entry = tracker_phrase_library_get(&lib, "chord");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->name, "chord");
    ASSERT_STR_EQ(entry->expression, "C4 E4 G4");
    ASSERT_STR_EQ(entry->language_id, "alda");

    tracker_phrase_library_clear(&lib);
}

TEST(phrase_library_add_update) {
    TrackerPhraseLibrary lib;
    tracker_phrase_library_init(&lib);

    tracker_phrase_library_add(&lib, "test", "original", NULL);
    ASSERT_EQ(lib.count, 1);

    /* Adding with same name should update */
    tracker_phrase_library_add(&lib, "test", "updated", "lang");
    ASSERT_EQ(lib.count, 1);  /* Still 1 */

    TrackerPhraseEntry *entry = tracker_phrase_library_get(&lib, "test");
    ASSERT_STR_EQ(entry->expression, "updated");
    ASSERT_STR_EQ(entry->language_id, "lang");

    tracker_phrase_library_clear(&lib);
}

TEST(phrase_library_find) {
    TrackerPhraseLibrary lib;
    tracker_phrase_library_init(&lib);

    tracker_phrase_library_add(&lib, "first", "1", NULL);
    tracker_phrase_library_add(&lib, "second", "2", NULL);

    int idx = tracker_phrase_library_find(&lib, "first");
    ASSERT_EQ(idx, 0);

    idx = tracker_phrase_library_find(&lib, "second");
    ASSERT_EQ(idx, 1);

    idx = tracker_phrase_library_find(&lib, "notfound");
    ASSERT_EQ(idx, -1);

    tracker_phrase_library_clear(&lib);
}

TEST(phrase_library_remove) {
    TrackerPhraseLibrary lib;
    tracker_phrase_library_init(&lib);

    tracker_phrase_library_add(&lib, "first", "1", NULL);
    tracker_phrase_library_add(&lib, "second", "2", NULL);
    ASSERT_EQ(lib.count, 2);

    bool result = tracker_phrase_library_remove(&lib, "first");
    ASSERT_TRUE(result);
    ASSERT_EQ(lib.count, 1);

    /* "second" should now be at index 0 */
    ASSERT_STR_EQ(lib.entries[0].name, "second");

    result = tracker_phrase_library_remove(&lib, "notfound");
    ASSERT_FALSE(result);

    tracker_phrase_library_clear(&lib);
}

TEST(phrase_library_clear_null_safe) {
    tracker_phrase_library_clear(NULL);
}

/* ============================================================================
 * Utility Function Tests
 * ============================================================================ */

TEST(calc_absolute_tick) {
    int64_t tick = tracker_calc_absolute_tick(0, 0, 6);
    ASSERT_EQ(tick, 0);

    tick = tracker_calc_absolute_tick(1, 0, 6);
    ASSERT_EQ(tick, 6);

    tick = tracker_calc_absolute_tick(10, 3, 6);
    ASSERT_EQ(tick, 63);  /* 10 * 6 + 3 */
}

TEST(tick_to_ms) {
    /* At 120 BPM, 4 rows/beat, 6 ticks/row:
     * 24 ticks = 1 beat = 500ms
     */
    double ms = tracker_tick_to_ms(24, 120, 4, 6);
    ASSERT_TRUE(ms > 499.0 && ms < 501.0);

    /* At 60 BPM: 24 ticks = 1000ms */
    ms = tracker_tick_to_ms(24, 60, 4, 6);
    ASSERT_TRUE(ms > 999.0 && ms < 1001.0);
}

TEST(cell_has_content) {
    TrackerCell cell;

    cell.type = TRACKER_CELL_EMPTY;
    ASSERT_FALSE(tracker_cell_has_content(&cell));

    cell.type = TRACKER_CELL_CONTINUATION;
    ASSERT_FALSE(tracker_cell_has_content(&cell));

    cell.type = TRACKER_CELL_EXPRESSION;
    ASSERT_TRUE(tracker_cell_has_content(&cell));

    cell.type = TRACKER_CELL_NOTE_OFF;
    ASSERT_TRUE(tracker_cell_has_content(&cell));
}

TEST(event_has_flags) {
    TrackerEvent event;

    event.flags = TRACKER_FLAG_NONE;
    ASSERT_FALSE(tracker_event_has_flags(&event));

    event.flags = TRACKER_FLAG_ACCENT;
    ASSERT_TRUE(tracker_event_has_flags(&event));

    event.flags = TRACKER_FLAG_PROBABILITY | TRACKER_FLAG_LEGATO;
    ASSERT_TRUE(tracker_event_has_flags(&event));
}

TEST(event_needs_params) {
    ASSERT_FALSE(tracker_event_needs_params(TRACKER_FLAG_NONE));
    ASSERT_FALSE(tracker_event_needs_params(TRACKER_FLAG_LEGATO));
    ASSERT_FALSE(tracker_event_needs_params(TRACKER_FLAG_MUTE));

    ASSERT_TRUE(tracker_event_needs_params(TRACKER_FLAG_PROBABILITY));
    ASSERT_TRUE(tracker_event_needs_params(TRACKER_FLAG_HUMANIZE_TIME));
    ASSERT_TRUE(tracker_event_needs_params(TRACKER_FLAG_HUMANIZE_VEL));
    ASSERT_TRUE(tracker_event_needs_params(TRACKER_FLAG_ACCENT));
    ASSERT_TRUE(tracker_event_needs_params(TRACKER_FLAG_RETRIGGER));
    ASSERT_TRUE(tracker_event_needs_params(TRACKER_FLAG_SLIDE));
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

BEGIN_TEST_SUITE("Tracker Model Tests")

    /* Event Params */
    RUN_TEST(event_params_new_defaults);
    RUN_TEST(event_params_free_null_safe);
    RUN_TEST(event_params_clone);
    RUN_TEST(event_params_clone_null);

    /* Phrase */
    RUN_TEST(phrase_new_with_capacity);
    RUN_TEST(phrase_new_zero_capacity);
    RUN_TEST(phrase_free_null_safe);
    RUN_TEST(phrase_add_event);
    RUN_TEST(phrase_add_event_with_params);
    RUN_TEST(phrase_add_event_grows_capacity);
    RUN_TEST(phrase_add_event_null_phrase);
    RUN_TEST(phrase_add_event_null_event);
    RUN_TEST(phrase_clear);
    RUN_TEST(phrase_clear_null_safe);
    RUN_TEST(phrase_clone);
    RUN_TEST(phrase_clone_null);

    /* FX Chain */
    RUN_TEST(fx_chain_init);
    RUN_TEST(fx_chain_init_null_safe);
    RUN_TEST(fx_chain_append);
    RUN_TEST(fx_chain_append_multiple);
    RUN_TEST(fx_chain_append_null_name);
    RUN_TEST(fx_chain_insert);
    RUN_TEST(fx_chain_insert_invalid_index);
    RUN_TEST(fx_chain_remove);
    RUN_TEST(fx_chain_remove_invalid_index);
    RUN_TEST(fx_chain_move);
    RUN_TEST(fx_chain_move_same_index);
    RUN_TEST(fx_chain_set_enabled);
    RUN_TEST(fx_chain_get);
    RUN_TEST(fx_chain_clone);
    RUN_TEST(fx_chain_clear_null_safe);

    /* Cell */
    RUN_TEST(cell_init);
    RUN_TEST(cell_init_null_safe);
    RUN_TEST(cell_set_expression);
    RUN_TEST(cell_set_expression_empty);
    RUN_TEST(cell_mark_dirty);
    RUN_TEST(cell_mark_dirty_null_safe);
    RUN_TEST(cell_clone);
    RUN_TEST(cell_clear_null_safe);

    /* Track */
    RUN_TEST(track_new);
    RUN_TEST(track_new_no_name);
    RUN_TEST(track_free_null_safe);
    RUN_TEST(track_resize_grow);
    RUN_TEST(track_resize_shrink);
    RUN_TEST(track_resize_to_zero);

    /* Pattern */
    RUN_TEST(pattern_new);
    RUN_TEST(pattern_free_null_safe);
    RUN_TEST(pattern_get_cell);
    RUN_TEST(pattern_add_track);
    RUN_TEST(pattern_remove_track);
    RUN_TEST(pattern_set_rows);

    /* Song */
    RUN_TEST(song_new);
    RUN_TEST(song_free_null_safe);
    RUN_TEST(song_add_pattern);
    RUN_TEST(song_add_pattern_null);
    RUN_TEST(song_get_pattern);
    RUN_TEST(song_remove_pattern);
    RUN_TEST(song_append_to_sequence);
    RUN_TEST(song_append_to_sequence_invalid);

    /* Phrase Library */
    RUN_TEST(phrase_library_init);
    RUN_TEST(phrase_library_add);
    RUN_TEST(phrase_library_add_update);
    RUN_TEST(phrase_library_find);
    RUN_TEST(phrase_library_remove);
    RUN_TEST(phrase_library_clear_null_safe);

    /* Utility Functions */
    RUN_TEST(calc_absolute_tick);
    RUN_TEST(tick_to_ms);
    RUN_TEST(cell_has_content);
    RUN_TEST(event_has_flags);
    RUN_TEST(event_needs_params);

END_TEST_SUITE()
