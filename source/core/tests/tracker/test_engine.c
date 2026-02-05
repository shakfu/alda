/**
 * @file test_engine.c
 * @brief Tests for tracker playback engine (tracker_engine.c).
 *
 * Tests verify:
 * - Engine lifecycle (create, free, reset, config)
 * - State management and transport controls
 * - Timing calculations and BPM handling
 * - Event queue operations
 * - Active note tracking
 * - Settings (loop, play mode, sync mode)
 * - Query functions and error handling
 * - Statistics tracking
 * - Output dispatch via mock callbacks
 */

#include "test_framework.h"
#include "tracker_engine.h"
#include "tracker_model.h"
#include <string.h>
#include <math.h>

test_stats_t test_stats;

/* ============================================================================
 * Mock Output Tracking
 * ============================================================================ */

typedef struct {
    int note_on_count;
    int note_off_count;
    int cc_count;
    int program_change_count;
    int pitch_bend_count;
    int aftertouch_count;
    int poly_aftertouch_count;
    int all_notes_off_count;
    int start_count;
    int stop_count;
    int continue_count;

    /* Last received values */
    uint8_t last_note_on_channel;
    uint8_t last_note_on_note;
    uint8_t last_note_on_velocity;
    uint8_t last_note_off_channel;
    uint8_t last_note_off_note;
    uint8_t last_cc_channel;
    uint8_t last_cc_number;
    uint8_t last_cc_value;
    uint8_t last_program_channel;
    uint8_t last_program;
} MockOutput;

static MockOutput g_mock;

static void reset_mock(void) {
    memset(&g_mock, 0, sizeof(g_mock));
}

static void mock_note_on(void* user_data, uint8_t channel, uint8_t note, uint8_t velocity) {
    (void)user_data;
    g_mock.note_on_count++;
    g_mock.last_note_on_channel = channel;
    g_mock.last_note_on_note = note;
    g_mock.last_note_on_velocity = velocity;
}

static void mock_note_off(void* user_data, uint8_t channel, uint8_t note, uint8_t velocity) {
    (void)user_data;
    (void)velocity;
    g_mock.note_off_count++;
    g_mock.last_note_off_channel = channel;
    g_mock.last_note_off_note = note;
}

static void mock_cc(void* user_data, uint8_t channel, uint8_t cc, uint8_t value) {
    (void)user_data;
    g_mock.cc_count++;
    g_mock.last_cc_channel = channel;
    g_mock.last_cc_number = cc;
    g_mock.last_cc_value = value;
}

static void mock_program_change(void* user_data, uint8_t channel, uint8_t program) {
    (void)user_data;
    g_mock.program_change_count++;
    g_mock.last_program_channel = channel;
    g_mock.last_program = program;
}

static void mock_pitch_bend(void* user_data, uint8_t channel, int16_t value) {
    (void)user_data;
    (void)channel;
    (void)value;
    g_mock.pitch_bend_count++;
}

static void mock_aftertouch(void* user_data, uint8_t channel, uint8_t pressure) {
    (void)user_data;
    (void)channel;
    (void)pressure;
    g_mock.aftertouch_count++;
}

static void mock_poly_aftertouch(void* user_data, uint8_t channel, uint8_t note, uint8_t pressure) {
    (void)user_data;
    (void)channel;
    (void)note;
    (void)pressure;
    g_mock.poly_aftertouch_count++;
}

static void mock_all_notes_off(void* user_data, uint8_t channel) {
    (void)user_data;
    (void)channel;
    g_mock.all_notes_off_count++;
}

static void mock_start(void* user_data) {
    (void)user_data;
    g_mock.start_count++;
}

static void mock_stop(void* user_data) {
    (void)user_data;
    g_mock.stop_count++;
}

static void mock_continue(void* user_data) {
    (void)user_data;
    g_mock.continue_count++;
}

static TrackerOutput create_mock_output(void) {
    TrackerOutput output = {0};
    output.note_on = mock_note_on;
    output.note_off = mock_note_off;
    output.cc = mock_cc;
    output.program_change = mock_program_change;
    output.pitch_bend = mock_pitch_bend;
    output.aftertouch = mock_aftertouch;
    output.poly_aftertouch = mock_poly_aftertouch;
    output.all_notes_off = mock_all_notes_off;
    output.start = mock_start;
    output.stop = mock_stop;
    output.cont = mock_continue;
    return output;
}

/* ============================================================================
 * Configuration Tests
 * ============================================================================ */

TEST(config_init_defaults) {
    TrackerEngineConfig config;
    tracker_engine_config_init(&config);

    ASSERT_EQ(config.sync_mode, TRACKER_SYNC_INTERNAL);
    ASSERT_EQ(config.default_play_mode, TRACKER_PLAY_MODE_PATTERN);
    ASSERT_FALSE(config.send_midi_clock);
    ASSERT_TRUE(config.auto_recompile);
    ASSERT_FALSE(config.chase_notes);
    ASSERT_TRUE(config.send_all_notes_off_on_stop);
    ASSERT_EQ(config.lookahead_ms, 10);
    ASSERT_EQ(config.max_pending_events, TRACKER_ENGINE_MAX_PENDING_EVENTS);
    ASSERT_EQ(config.max_active_notes, TRACKER_ENGINE_MAX_ACTIVE_NOTES);
}

TEST(config_init_null_safe) {
    tracker_engine_config_init(NULL);
    /* Should not crash */
}

/* ============================================================================
 * Lifecycle Tests
 * ============================================================================ */

TEST(engine_new_defaults) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_EQ(engine->state, TRACKER_ENGINE_STOPPED);
    ASSERT_EQ(engine->play_mode, TRACKER_PLAY_MODE_PATTERN);
    ASSERT_EQ(engine->bpm, TRACKER_DEFAULT_BPM);
    ASSERT_EQ(engine->rows_per_beat, TRACKER_DEFAULT_RPB);
    ASSERT_EQ(engine->ticks_per_row, TRACKER_DEFAULT_TPR);
    ASSERT_TRUE(engine->loop_enabled);
    ASSERT_EQ(engine->loop_start_row, -1);
    ASSERT_EQ(engine->loop_end_row, -1);
    ASSERT_NULL(engine->song);
    ASSERT_NOT_NULL(engine->active_notes);

    tracker_engine_free(engine);
}

TEST(engine_new_with_config) {
    TrackerEngineConfig config;
    tracker_engine_config_init(&config);
    config.default_play_mode = TRACKER_PLAY_MODE_SONG;
    config.max_active_notes = 64;

    TrackerEngine* engine = tracker_engine_new_with_config(&config);
    ASSERT_NOT_NULL(engine);

    ASSERT_EQ(engine->play_mode, TRACKER_PLAY_MODE_SONG);
    ASSERT_EQ(engine->config.max_active_notes, 64);

    tracker_engine_free(engine);
}

TEST(engine_new_with_null_config) {
    TrackerEngine* engine = tracker_engine_new_with_config(NULL);
    ASSERT_NOT_NULL(engine);

    /* Should use defaults */
    ASSERT_EQ(engine->play_mode, TRACKER_PLAY_MODE_PATTERN);

    tracker_engine_free(engine);
}

TEST(engine_free_null_safe) {
    tracker_engine_free(NULL);
    /* Should not crash */
}

TEST(engine_reset) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    /* Modify some state */
    engine->current_pattern = 5;
    engine->current_row = 10;
    engine->current_tick = 1000;
    engine->loop_count = 3;
    engine->events_fired = 100;

    tracker_engine_reset(engine);

    ASSERT_EQ(engine->current_pattern, 0);
    ASSERT_EQ(engine->current_row, 0);
    ASSERT_EQ(engine->current_tick, 0);
    ASSERT_EQ(engine->loop_count, 0);
    ASSERT_EQ(engine->events_fired, 0);
    ASSERT_EQ(engine->state, TRACKER_ENGINE_STOPPED);

    tracker_engine_free(engine);
}

TEST(engine_reset_null_safe) {
    tracker_engine_reset(NULL);
    /* Should not crash */
}

/* ============================================================================
 * State Tests
 * ============================================================================ */

TEST(engine_initial_state_stopped) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_TRUE(tracker_engine_is_stopped(engine));
    ASSERT_FALSE(tracker_engine_is_playing(engine));
    ASSERT_FALSE(tracker_engine_is_paused(engine));

    tracker_engine_free(engine);
}

TEST(engine_play_without_song_fails) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    bool result = tracker_engine_play(engine);
    ASSERT_FALSE(result);
    ASSERT_TRUE(tracker_engine_is_stopped(engine));

    tracker_engine_free(engine);
}

TEST(engine_stop_null_safe) {
    tracker_engine_stop(NULL);
    /* Should not crash */
}

TEST(engine_pause_null_safe) {
    tracker_engine_pause(NULL);
    /* Should not crash */
}

TEST(engine_toggle_null_safe) {
    tracker_engine_toggle(NULL);
    /* Should not crash */
}

/* ============================================================================
 * Timing Tests
 * ============================================================================ */

TEST(engine_timing_cache_calculated) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    /* Default: 120 BPM, 4 rows/beat, 6 ticks/row */
    /* ms_per_beat = 60000 / 120 = 500 */
    /* row_duration = 500 / 4 = 125 ms */
    /* tick_duration = 125 / 6 = ~20.833 ms */

    double expected_row_duration = 60000.0 / 120.0 / 4.0;
    double expected_tick_duration = expected_row_duration / 6.0;

    ASSERT_TRUE(fabs(engine->row_duration_ms - expected_row_duration) < 0.001);
    ASSERT_TRUE(fabs(engine->tick_duration_ms - expected_tick_duration) < 0.001);

    tracker_engine_free(engine);
}

TEST(engine_set_bpm) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    tracker_engine_set_bpm(engine, 60);
    ASSERT_EQ(engine->bpm, 60);

    /* At 60 BPM: ms_per_beat = 1000, row_duration = 250 */
    double expected_row_duration = 60000.0 / 60.0 / 4.0;
    ASSERT_TRUE(fabs(engine->row_duration_ms - expected_row_duration) < 0.001);

    tracker_engine_free(engine);
}

TEST(engine_set_bpm_invalid) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    int original_bpm = engine->bpm;
    tracker_engine_set_bpm(engine, 0);
    ASSERT_EQ(engine->bpm, original_bpm);  /* Unchanged */

    tracker_engine_set_bpm(engine, -10);
    ASSERT_EQ(engine->bpm, original_bpm);  /* Unchanged */

    tracker_engine_free(engine);
}

TEST(engine_set_bpm_null_safe) {
    tracker_engine_set_bpm(NULL, 120);
    /* Should not crash */
}

TEST(engine_get_bpm) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_EQ(tracker_engine_get_bpm(engine), TRACKER_DEFAULT_BPM);

    tracker_engine_set_bpm(engine, 180);
    ASSERT_EQ(tracker_engine_get_bpm(engine), 180);

    tracker_engine_free(engine);
}

TEST(engine_get_bpm_null) {
    ASSERT_EQ(tracker_engine_get_bpm(NULL), 0);
}

TEST(engine_row_to_tick) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    /* Default: 6 ticks per row */
    ASSERT_EQ(tracker_engine_row_to_tick(engine, 0, 0), 0);
    ASSERT_EQ(tracker_engine_row_to_tick(engine, 0, 1), 6);
    ASSERT_EQ(tracker_engine_row_to_tick(engine, 0, 10), 60);

    tracker_engine_free(engine);
}

TEST(engine_tick_to_row) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    /* Default: 6 ticks per row */
    ASSERT_EQ(tracker_engine_tick_to_row(engine, 0), 0);
    ASSERT_EQ(tracker_engine_tick_to_row(engine, 5), 0);
    ASSERT_EQ(tracker_engine_tick_to_row(engine, 6), 1);
    ASSERT_EQ(tracker_engine_tick_to_row(engine, 60), 10);

    tracker_engine_free(engine);
}

TEST(engine_ms_to_ticks) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    /* With default timing, one tick is ~20.833 ms */
    int64_t ticks = tracker_engine_ms_to_ticks(engine, engine->tick_duration_ms);
    ASSERT_EQ(ticks, 1);

    /* One row is ~125 ms = 6 ticks */
    ticks = tracker_engine_ms_to_ticks(engine, engine->row_duration_ms);
    ASSERT_EQ(ticks, 6);

    tracker_engine_free(engine);
}

TEST(engine_ticks_to_ms) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    double ms = tracker_engine_ticks_to_ms(engine, 1);
    ASSERT_TRUE(fabs(ms - engine->tick_duration_ms) < 0.001);

    /* 6 ticks = 1 row */
    ms = tracker_engine_ticks_to_ms(engine, 6);
    ASSERT_TRUE(fabs(ms - engine->row_duration_ms) < 0.001);

    tracker_engine_free(engine);
}

/* ============================================================================
 * Settings Tests
 * ============================================================================ */

TEST(engine_set_play_mode) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_EQ(engine->play_mode, TRACKER_PLAY_MODE_PATTERN);

    tracker_engine_set_play_mode(engine, TRACKER_PLAY_MODE_SONG);
    ASSERT_EQ(engine->play_mode, TRACKER_PLAY_MODE_SONG);

    tracker_engine_set_play_mode(engine, TRACKER_PLAY_MODE_PATTERN);
    ASSERT_EQ(engine->play_mode, TRACKER_PLAY_MODE_PATTERN);

    tracker_engine_free(engine);
}

TEST(engine_set_play_mode_null_safe) {
    tracker_engine_set_play_mode(NULL, TRACKER_PLAY_MODE_SONG);
    /* Should not crash */
}

TEST(engine_set_loop) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_TRUE(engine->loop_enabled);

    tracker_engine_set_loop(engine, false);
    ASSERT_FALSE(engine->loop_enabled);

    tracker_engine_set_loop(engine, true);
    ASSERT_TRUE(engine->loop_enabled);

    tracker_engine_free(engine);
}

TEST(engine_set_loop_null_safe) {
    tracker_engine_set_loop(NULL, true);
    /* Should not crash */
}

TEST(engine_set_loop_points) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_EQ(engine->loop_start_row, -1);
    ASSERT_EQ(engine->loop_end_row, -1);

    tracker_engine_set_loop_points(engine, 4, 16);
    ASSERT_EQ(engine->loop_start_row, 4);
    ASSERT_EQ(engine->loop_end_row, 16);

    tracker_engine_set_loop_points(engine, -1, -1);
    ASSERT_EQ(engine->loop_start_row, -1);
    ASSERT_EQ(engine->loop_end_row, -1);

    tracker_engine_free(engine);
}

TEST(engine_set_loop_points_null_safe) {
    tracker_engine_set_loop_points(NULL, 0, 16);
    /* Should not crash */
}

TEST(engine_set_sync_mode) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_EQ(engine->config.sync_mode, TRACKER_SYNC_INTERNAL);

    tracker_engine_set_sync_mode(engine, TRACKER_SYNC_EXTERNAL_MIDI);
    ASSERT_EQ(engine->config.sync_mode, TRACKER_SYNC_EXTERNAL_MIDI);

    tracker_engine_set_sync_mode(engine, TRACKER_SYNC_EXTERNAL_LINK);
    ASSERT_EQ(engine->config.sync_mode, TRACKER_SYNC_EXTERNAL_LINK);

    tracker_engine_free(engine);
}

TEST(engine_set_sync_mode_null_safe) {
    tracker_engine_set_sync_mode(NULL, TRACKER_SYNC_INTERNAL);
    /* Should not crash */
}

/* ============================================================================
 * Position Query Tests
 * ============================================================================ */

TEST(engine_get_position) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    engine->current_pattern = 2;
    engine->current_row = 8;
    engine->current_tick = 8 * 6 + 3;  /* Row 8, tick 3 (6 ticks per row) */

    int pattern, row, tick;
    tracker_engine_get_position(engine, &pattern, &row, &tick);

    ASSERT_EQ(pattern, 2);
    ASSERT_EQ(row, 8);
    ASSERT_EQ(tick, 3);

    tracker_engine_free(engine);
}

TEST(engine_get_position_null_params) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    engine->current_pattern = 1;
    engine->current_row = 5;

    /* Should handle NULL output params */
    tracker_engine_get_position(engine, NULL, NULL, NULL);
    /* Should not crash */

    int pattern;
    tracker_engine_get_position(engine, &pattern, NULL, NULL);
    ASSERT_EQ(pattern, 1);

    tracker_engine_free(engine);
}

TEST(engine_get_position_null_engine) {
    tracker_engine_get_position(NULL, NULL, NULL, NULL);
    /* Should not crash */
}

TEST(engine_get_time_ms) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_TRUE(fabs(tracker_engine_get_time_ms(engine)) < 0.001);

    engine->current_time_ms = 1234.5;
    ASSERT_TRUE(fabs(tracker_engine_get_time_ms(engine) - 1234.5) < 0.001);

    tracker_engine_free(engine);
}

TEST(engine_get_time_ms_null) {
    ASSERT_TRUE(fabs(tracker_engine_get_time_ms(NULL)) < 0.001);
}

/* ============================================================================
 * Error Handling Tests
 * ============================================================================ */

TEST(engine_error_initially_null) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_NULL(tracker_engine_get_error(engine));

    tracker_engine_free(engine);
}

TEST(engine_get_error_null) {
    ASSERT_NULL(tracker_engine_get_error(NULL));
}

TEST(engine_clear_error) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    /* Simulate error state */
    engine->last_error = strdup("Test error");
    engine->error_pattern = 1;
    engine->error_track = 2;
    engine->error_row = 3;

    tracker_engine_clear_error(engine);

    ASSERT_NULL(engine->last_error);
    ASSERT_EQ(engine->error_pattern, -1);
    ASSERT_EQ(engine->error_track, -1);
    ASSERT_EQ(engine->error_row, -1);

    tracker_engine_free(engine);
}

TEST(engine_clear_error_null_safe) {
    tracker_engine_clear_error(NULL);
    /* Should not crash */
}

TEST(engine_get_error_location) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    engine->error_pattern = 5;
    engine->error_track = 3;
    engine->error_row = 12;

    int pattern, track, row;
    tracker_engine_get_error_location(engine, &pattern, &track, &row);

    ASSERT_EQ(pattern, 5);
    ASSERT_EQ(track, 3);
    ASSERT_EQ(row, 12);

    tracker_engine_free(engine);
}

TEST(engine_get_error_location_null_safe) {
    tracker_engine_get_error_location(NULL, NULL, NULL, NULL);
    /* Should not crash */
}

/* ============================================================================
 * Event Queue Tests
 * ============================================================================ */

TEST(engine_pending_count_initially_zero) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_EQ(tracker_engine_pending_count(engine), 0);

    tracker_engine_free(engine);
}

TEST(engine_pending_count_null) {
    ASSERT_EQ(tracker_engine_pending_count(NULL), 0);
}

TEST(engine_schedule_event) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    TrackerEvent event = {
        .type = TRACKER_EVENT_NOTE_ON,
        .channel = 1,
        .data1 = 60,
        .data2 = 100,
    };

    bool result = tracker_engine_schedule_event(engine, 100, &event, NULL);
    ASSERT_TRUE(result);
    ASSERT_EQ(tracker_engine_pending_count(engine), 1);

    tracker_engine_free(engine);
}

TEST(engine_schedule_event_null_engine) {
    TrackerEvent event = {0};
    bool result = tracker_engine_schedule_event(NULL, 100, &event, NULL);
    ASSERT_FALSE(result);
}

TEST(engine_schedule_event_null_event) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    bool result = tracker_engine_schedule_event(engine, 100, NULL, NULL);
    ASSERT_FALSE(result);

    tracker_engine_free(engine);
}

TEST(engine_schedule_multiple_events_sorted) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    TrackerEvent event = {.type = TRACKER_EVENT_NOTE_ON};

    /* Schedule out of order */
    tracker_engine_schedule_event(engine, 300, &event, NULL);
    tracker_engine_schedule_event(engine, 100, &event, NULL);
    tracker_engine_schedule_event(engine, 200, &event, NULL);

    ASSERT_EQ(tracker_engine_pending_count(engine), 3);

    /* Verify sorted order */
    ASSERT_EQ(engine->pending_head->due_tick, 100);
    ASSERT_EQ(engine->pending_head->next->due_tick, 200);
    ASSERT_EQ(engine->pending_head->next->next->due_tick, 300);

    tracker_engine_free(engine);
}

TEST(engine_cancel_all) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    TrackerEvent event = {.type = TRACKER_EVENT_NOTE_ON};
    tracker_engine_schedule_event(engine, 100, &event, NULL);
    tracker_engine_schedule_event(engine, 200, &event, NULL);
    tracker_engine_schedule_event(engine, 300, &event, NULL);

    ASSERT_EQ(tracker_engine_pending_count(engine), 3);

    tracker_engine_cancel_all(engine);
    ASSERT_EQ(tracker_engine_pending_count(engine), 0);

    tracker_engine_free(engine);
}

TEST(engine_cancel_all_null_safe) {
    tracker_engine_cancel_all(NULL);
    /* Should not crash */
}

TEST(engine_cancel_track) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    TrackerEvent event = {.type = TRACKER_EVENT_NOTE_ON};
    TrackerEventSource source0 = {.track_index = 0};
    TrackerEventSource source1 = {.track_index = 1};

    tracker_engine_schedule_event(engine, 100, &event, &source0);
    tracker_engine_schedule_event(engine, 200, &event, &source1);
    tracker_engine_schedule_event(engine, 300, &event, &source0);

    ASSERT_EQ(tracker_engine_pending_count(engine), 3);

    tracker_engine_cancel_track(engine, 0);
    ASSERT_EQ(tracker_engine_pending_count(engine), 1);

    tracker_engine_free(engine);
}

TEST(engine_cancel_track_null_safe) {
    tracker_engine_cancel_track(NULL, 0);
    /* Should not crash */
}

TEST(engine_cancel_phrase) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    TrackerEvent event = {.type = TRACKER_EVENT_NOTE_ON};
    TrackerEventSource source1 = {.phrase_id = 1};
    TrackerEventSource source2 = {.phrase_id = 2};

    tracker_engine_schedule_event(engine, 100, &event, &source1);
    tracker_engine_schedule_event(engine, 200, &event, &source2);
    tracker_engine_schedule_event(engine, 300, &event, &source1);

    ASSERT_EQ(tracker_engine_pending_count(engine), 3);

    tracker_engine_cancel_phrase(engine, 1);
    ASSERT_EQ(tracker_engine_pending_count(engine), 1);

    tracker_engine_free(engine);
}

TEST(engine_cancel_phrase_null_safe) {
    tracker_engine_cancel_phrase(NULL, 0);
    /* Should not crash */
}

/* ============================================================================
 * Active Note Tests
 * ============================================================================ */

TEST(engine_active_note_count_initially_zero) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_EQ(tracker_engine_active_note_count(engine), 0);

    tracker_engine_free(engine);
}

TEST(engine_active_note_count_null) {
    ASSERT_EQ(tracker_engine_active_note_count(NULL), 0);
}

TEST(engine_all_notes_off_null_safe) {
    tracker_engine_all_notes_off(NULL);
    /* Should not crash */
}

TEST(engine_channel_notes_off_null_safe) {
    tracker_engine_channel_notes_off(NULL, 0);
    /* Should not crash */
}

TEST(engine_track_notes_off_null_safe) {
    tracker_engine_track_notes_off(NULL, 0);
    /* Should not crash */
}

/* ============================================================================
 * Output Configuration Tests
 * ============================================================================ */

TEST(engine_set_output) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    TrackerOutput output = create_mock_output();
    output.user_data = (void*)0x12345678;

    tracker_engine_set_output(engine, &output);

    const TrackerOutput* retrieved = tracker_engine_get_output(engine);
    ASSERT_NOT_NULL(retrieved);
    ASSERT_NOT_NULL(retrieved->user_data);
    ASSERT_TRUE(retrieved->note_on == mock_note_on);

    tracker_engine_free(engine);
}

TEST(engine_set_output_null_safe) {
    TrackerEngine* engine = tracker_engine_new();
    tracker_engine_set_output(engine, NULL);
    tracker_engine_set_output(NULL, NULL);
    tracker_engine_free(engine);
    /* Should not crash */
}

TEST(engine_get_output_null) {
    ASSERT_NULL(tracker_engine_get_output(NULL));
}

/* ============================================================================
 * Statistics Tests
 * ============================================================================ */

TEST(engine_stats_initially_zero) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    TrackerEngineStats stats;
    tracker_engine_get_stats(engine, &stats);

    ASSERT_EQ(stats.events_fired, 0);
    ASSERT_EQ(stats.events_scheduled, 0);
    ASSERT_EQ(stats.notes_on, 0);
    ASSERT_EQ(stats.notes_off, 0);
    ASSERT_EQ(stats.underruns, 0);
    ASSERT_EQ(stats.pending_events, 0);
    ASSERT_EQ(stats.active_notes, 0);

    tracker_engine_free(engine);
}

TEST(engine_get_stats_null_safe) {
    TrackerEngine* engine = tracker_engine_new();
    tracker_engine_get_stats(engine, NULL);
    tracker_engine_get_stats(NULL, NULL);
    tracker_engine_free(engine);
    /* Should not crash */
}

TEST(engine_reset_stats) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    /* Simulate some activity */
    engine->events_fired = 100;
    engine->events_scheduled = 150;
    engine->notes_on = 50;
    engine->notes_off = 45;
    engine->underruns = 2;

    tracker_engine_reset_stats(engine);

    ASSERT_EQ(engine->events_fired, 0);
    ASSERT_EQ(engine->events_scheduled, 0);
    ASSERT_EQ(engine->notes_on, 0);
    ASSERT_EQ(engine->notes_off, 0);
    ASSERT_EQ(engine->underruns, 0);

    tracker_engine_free(engine);
}

TEST(engine_reset_stats_null_safe) {
    tracker_engine_reset_stats(NULL);
    /* Should not crash */
}

/* ============================================================================
 * Song Management Tests
 * ============================================================================ */

TEST(engine_load_song_null_engine) {
    TrackerSong* song = tracker_song_new("Test");
    bool result = tracker_engine_load_song(NULL, song);
    ASSERT_FALSE(result);
    tracker_song_free(song);
}

TEST(engine_load_null_song) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    bool result = tracker_engine_load_song(engine, NULL);
    ASSERT_TRUE(result);  /* Loading NULL unloads current song */
    ASSERT_NULL(engine->song);

    tracker_engine_free(engine);
}

TEST(engine_load_song_sets_timing) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    TrackerSong* song = tracker_song_new("Test");
    song->bpm = 140;
    song->rows_per_beat = 8;
    song->ticks_per_row = 12;

    /* Disable auto-recompile to avoid compilation issues */
    engine->config.auto_recompile = false;

    bool result = tracker_engine_load_song(engine, song);
    ASSERT_TRUE(result);

    ASSERT_EQ(engine->bpm, 140);
    ASSERT_EQ(engine->rows_per_beat, 8);
    ASSERT_EQ(engine->ticks_per_row, 12);

    tracker_engine_free(engine);
    tracker_song_free(song);
}

TEST(engine_unload_song) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    TrackerSong* song = tracker_song_new("Test");
    engine->config.auto_recompile = false;
    tracker_engine_load_song(engine, song);

    ASSERT_TRUE(engine->song == song);

    tracker_engine_unload_song(engine);
    ASSERT_NULL(engine->song);
    ASSERT_EQ(engine->current_pattern, 0);
    ASSERT_EQ(engine->current_row, 0);

    tracker_engine_free(engine);
    tracker_song_free(song);
}

TEST(engine_unload_song_null_safe) {
    tracker_engine_unload_song(NULL);
    /* Should not crash */
}

TEST(engine_mark_dirty_null_safe) {
    tracker_engine_mark_dirty(NULL, 0, 0, 0);
    /* Should not crash */
}

/* ============================================================================
 * External Sync Tests
 * ============================================================================ */

TEST(engine_external_clock_ignores_internal_sync) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    /* Default is internal sync - external clock should be ignored */
    int64_t tick_before = engine->current_tick;
    tracker_engine_external_clock(engine);
    ASSERT_EQ(engine->current_tick, tick_before);

    tracker_engine_free(engine);
}

TEST(engine_external_clock_null_safe) {
    tracker_engine_external_clock(NULL);
    /* Should not crash */
}

TEST(engine_external_start_null_safe) {
    tracker_engine_external_start(NULL);
    /* Should not crash */
}

TEST(engine_external_stop_null_safe) {
    tracker_engine_external_stop(NULL);
    /* Should not crash */
}

TEST(engine_external_continue_null_safe) {
    tracker_engine_external_continue(NULL);
    /* Should not crash */
}

TEST(engine_external_position_null_safe) {
    tracker_engine_external_position(NULL, 0);
    /* Should not crash */
}

TEST(engine_link_update_null_safe) {
    tracker_engine_link_update(NULL, 0.0, 120.0, false);
    /* Should not crash */
}

TEST(engine_link_update_ignores_internal_sync) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    /* Default is internal sync */
    int bpm_before = engine->bpm;
    tracker_engine_link_update(engine, 10.0, 180.0, true);
    ASSERT_EQ(engine->bpm, bpm_before);  /* Should be unchanged */

    tracker_engine_free(engine);
}

/* ============================================================================
 * Process Tests (without song)
 * ============================================================================ */

TEST(engine_process_returns_zero_when_stopped) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    int events = tracker_engine_process(engine, 100.0);
    ASSERT_EQ(events, 0);

    tracker_engine_free(engine);
}

TEST(engine_process_null_safe) {
    int events = tracker_engine_process(NULL, 100.0);
    ASSERT_EQ(events, 0);
}

TEST(engine_process_until_null_safe) {
    int events = tracker_engine_process_until(NULL, 1000.0);
    ASSERT_EQ(events, 0);
}

TEST(engine_step_row_null_safe) {
    tracker_engine_step_row(NULL);
    /* Should not crash */
}

TEST(engine_step_tick_null_safe) {
    tracker_engine_step_tick(NULL);
    /* Should not crash */
}

TEST(engine_trigger_cell_null_safe) {
    tracker_engine_trigger_cell(NULL, 0, 0, 0);
    /* Should not crash */
}

/* ============================================================================
 * Track Control Tests
 * ============================================================================ */

TEST(engine_mute_track_null_safe) {
    tracker_engine_mute_track(NULL, 0, true);
    /* Should not crash */
}

TEST(engine_solo_track_null_safe) {
    tracker_engine_solo_track(NULL, 0, true);
    /* Should not crash */
}

TEST(engine_has_solo_null) {
    ASSERT_FALSE(tracker_engine_has_solo(NULL));
}

TEST(engine_has_solo_no_song) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    ASSERT_FALSE(tracker_engine_has_solo(engine));

    tracker_engine_free(engine);
}

TEST(engine_clear_solo_null_safe) {
    tracker_engine_clear_solo(NULL);
    /* Should not crash */
}

/* ============================================================================
 * Seek Tests
 * ============================================================================ */

TEST(engine_seek_null_safe) {
    tracker_engine_seek(NULL, 0, 0);
    /* Should not crash */
}

TEST(engine_seek_no_song) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    /* Should not crash without song */
    tracker_engine_seek(engine, 0, 0);

    tracker_engine_free(engine);
}

TEST(engine_next_pattern_null_safe) {
    tracker_engine_next_pattern(NULL);
    /* Should not crash */
}

TEST(engine_prev_pattern_null_safe) {
    tracker_engine_prev_pattern(NULL);
    /* Should not crash */
}

/* ============================================================================
 * Eval Immediate Tests
 * ============================================================================ */

TEST(engine_eval_immediate_null_engine) {
    bool result = tracker_engine_eval_immediate(NULL, "c4", NULL, 0);
    ASSERT_FALSE(result);
}

TEST(engine_eval_immediate_null_expression) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    bool result = tracker_engine_eval_immediate(engine, NULL, NULL, 0);
    ASSERT_FALSE(result);

    tracker_engine_free(engine);
}

/* ============================================================================
 * Compile Tests
 * ============================================================================ */

TEST(engine_compile_pattern_null_safe) {
    bool result = tracker_engine_compile_pattern(NULL, 0);
    ASSERT_FALSE(result);
}

TEST(engine_compile_pattern_no_song) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    bool result = tracker_engine_compile_pattern(engine, 0);
    ASSERT_FALSE(result);

    tracker_engine_free(engine);
}

TEST(engine_compile_all_null_safe) {
    bool result = tracker_engine_compile_all(NULL);
    ASSERT_FALSE(result);
}

TEST(engine_compile_all_no_song) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    bool result = tracker_engine_compile_all(engine);
    ASSERT_FALSE(result);

    tracker_engine_free(engine);
}

/* ============================================================================
 * Reset BPM Tests
 * ============================================================================ */

TEST(engine_reset_bpm_null_safe) {
    tracker_engine_reset_bpm(NULL);
    /* Should not crash */
}

TEST(engine_reset_bpm_no_song) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    tracker_engine_set_bpm(engine, 180);
    tracker_engine_reset_bpm(engine);
    /* Without song, should remain unchanged */
    ASSERT_EQ(engine->bpm, 180);

    tracker_engine_free(engine);
}

TEST(engine_reset_bpm_with_song) {
    TrackerEngine* engine = tracker_engine_new();
    ASSERT_NOT_NULL(engine);

    TrackerSong* song = tracker_song_new("Test");
    song->bpm = 90;
    engine->config.auto_recompile = false;
    tracker_engine_load_song(engine, song);

    tracker_engine_set_bpm(engine, 180);
    ASSERT_EQ(engine->bpm, 180);

    tracker_engine_reset_bpm(engine);
    ASSERT_EQ(engine->bpm, 90);

    tracker_engine_free(engine);
    tracker_song_free(song);
}

/* ============================================================================
 * Test Suite
 * ============================================================================ */

BEGIN_TEST_SUITE("Tracker Engine Tests")

    reset_mock();

    /* Configuration */
    RUN_TEST(config_init_defaults);
    RUN_TEST(config_init_null_safe);

    /* Lifecycle */
    RUN_TEST(engine_new_defaults);
    RUN_TEST(engine_new_with_config);
    RUN_TEST(engine_new_with_null_config);
    RUN_TEST(engine_free_null_safe);
    RUN_TEST(engine_reset);
    RUN_TEST(engine_reset_null_safe);

    /* State */
    RUN_TEST(engine_initial_state_stopped);
    RUN_TEST(engine_play_without_song_fails);
    RUN_TEST(engine_stop_null_safe);
    RUN_TEST(engine_pause_null_safe);
    RUN_TEST(engine_toggle_null_safe);

    /* Timing */
    RUN_TEST(engine_timing_cache_calculated);
    RUN_TEST(engine_set_bpm);
    RUN_TEST(engine_set_bpm_invalid);
    RUN_TEST(engine_set_bpm_null_safe);
    RUN_TEST(engine_get_bpm);
    RUN_TEST(engine_get_bpm_null);
    RUN_TEST(engine_row_to_tick);
    RUN_TEST(engine_tick_to_row);
    RUN_TEST(engine_ms_to_ticks);
    RUN_TEST(engine_ticks_to_ms);

    /* Settings */
    RUN_TEST(engine_set_play_mode);
    RUN_TEST(engine_set_play_mode_null_safe);
    RUN_TEST(engine_set_loop);
    RUN_TEST(engine_set_loop_null_safe);
    RUN_TEST(engine_set_loop_points);
    RUN_TEST(engine_set_loop_points_null_safe);
    RUN_TEST(engine_set_sync_mode);
    RUN_TEST(engine_set_sync_mode_null_safe);

    /* Position */
    RUN_TEST(engine_get_position);
    RUN_TEST(engine_get_position_null_params);
    RUN_TEST(engine_get_position_null_engine);
    RUN_TEST(engine_get_time_ms);
    RUN_TEST(engine_get_time_ms_null);

    /* Error handling */
    RUN_TEST(engine_error_initially_null);
    RUN_TEST(engine_get_error_null);
    RUN_TEST(engine_clear_error);
    RUN_TEST(engine_clear_error_null_safe);
    RUN_TEST(engine_get_error_location);
    RUN_TEST(engine_get_error_location_null_safe);

    /* Event queue */
    RUN_TEST(engine_pending_count_initially_zero);
    RUN_TEST(engine_pending_count_null);
    RUN_TEST(engine_schedule_event);
    RUN_TEST(engine_schedule_event_null_engine);
    RUN_TEST(engine_schedule_event_null_event);
    RUN_TEST(engine_schedule_multiple_events_sorted);
    RUN_TEST(engine_cancel_all);
    RUN_TEST(engine_cancel_all_null_safe);
    RUN_TEST(engine_cancel_track);
    RUN_TEST(engine_cancel_track_null_safe);
    RUN_TEST(engine_cancel_phrase);
    RUN_TEST(engine_cancel_phrase_null_safe);

    /* Active notes */
    RUN_TEST(engine_active_note_count_initially_zero);
    RUN_TEST(engine_active_note_count_null);
    RUN_TEST(engine_all_notes_off_null_safe);
    RUN_TEST(engine_channel_notes_off_null_safe);
    RUN_TEST(engine_track_notes_off_null_safe);

    /* Output */
    RUN_TEST(engine_set_output);
    RUN_TEST(engine_set_output_null_safe);
    RUN_TEST(engine_get_output_null);

    /* Statistics */
    RUN_TEST(engine_stats_initially_zero);
    RUN_TEST(engine_get_stats_null_safe);
    RUN_TEST(engine_reset_stats);
    RUN_TEST(engine_reset_stats_null_safe);

    /* Song management */
    RUN_TEST(engine_load_song_null_engine);
    RUN_TEST(engine_load_null_song);
    RUN_TEST(engine_load_song_sets_timing);
    RUN_TEST(engine_unload_song);
    RUN_TEST(engine_unload_song_null_safe);
    RUN_TEST(engine_mark_dirty_null_safe);

    /* External sync */
    RUN_TEST(engine_external_clock_ignores_internal_sync);
    RUN_TEST(engine_external_clock_null_safe);
    RUN_TEST(engine_external_start_null_safe);
    RUN_TEST(engine_external_stop_null_safe);
    RUN_TEST(engine_external_continue_null_safe);
    RUN_TEST(engine_external_position_null_safe);
    RUN_TEST(engine_link_update_null_safe);
    RUN_TEST(engine_link_update_ignores_internal_sync);

    /* Process */
    RUN_TEST(engine_process_returns_zero_when_stopped);
    RUN_TEST(engine_process_null_safe);
    RUN_TEST(engine_process_until_null_safe);
    RUN_TEST(engine_step_row_null_safe);
    RUN_TEST(engine_step_tick_null_safe);
    RUN_TEST(engine_trigger_cell_null_safe);

    /* Track control */
    RUN_TEST(engine_mute_track_null_safe);
    RUN_TEST(engine_solo_track_null_safe);
    RUN_TEST(engine_has_solo_null);
    RUN_TEST(engine_has_solo_no_song);
    RUN_TEST(engine_clear_solo_null_safe);

    /* Seek */
    RUN_TEST(engine_seek_null_safe);
    RUN_TEST(engine_seek_no_song);
    RUN_TEST(engine_next_pattern_null_safe);
    RUN_TEST(engine_prev_pattern_null_safe);

    /* Eval */
    RUN_TEST(engine_eval_immediate_null_engine);
    RUN_TEST(engine_eval_immediate_null_expression);

    /* Compile */
    RUN_TEST(engine_compile_pattern_null_safe);
    RUN_TEST(engine_compile_pattern_no_song);
    RUN_TEST(engine_compile_all_null_safe);
    RUN_TEST(engine_compile_all_no_song);

    /* Reset BPM */
    RUN_TEST(engine_reset_bpm_null_safe);
    RUN_TEST(engine_reset_bpm_no_song);
    RUN_TEST(engine_reset_bpm_with_song);

END_TEST_SUITE()
