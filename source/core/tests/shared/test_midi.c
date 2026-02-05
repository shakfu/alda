/**
 * @file test_midi.c
 * @brief Tests for shared MIDI I/O subsystem (midi.c and midi_input.c).
 *
 * Tests verify:
 * - NULL context handling (defensive programming)
 * - State queries (is_open, port_count) with different states
 * - Invalid port index handling
 * - Note callback registration
 * - Context lifecycle (init/cleanup)
 * - Timing utilities
 *
 * Note: Tests that would require actual MIDI hardware are kept minimal
 * and focus on state tracking and error handling rather than actual I/O.
 */

#include "test_framework.h"
#include "context.h"
#include "midi/midi.h"
#include <string.h>

test_stats_t test_stats;

/* ============================================================================
 * NULL Context Handling Tests
 *
 * All MIDI functions should handle NULL context gracefully without crashing.
 * ============================================================================ */

TEST(midi_null_context_init_observer) {
    /* Should not crash */
    shared_midi_init_observer(NULL);
}

TEST(midi_null_context_cleanup) {
    /* Should not crash */
    shared_midi_cleanup(NULL);
}

TEST(midi_null_context_get_port_count) {
    int count = shared_midi_get_port_count(NULL);
    ASSERT_EQ(count, 0);
}

TEST(midi_null_context_get_port_name) {
    const char *name = shared_midi_get_port_name(NULL, 0);
    ASSERT_NULL(name);
}

TEST(midi_null_context_open_port) {
    int result = shared_midi_open_port(NULL, 0);
    ASSERT_EQ(result, -1);
}

TEST(midi_null_context_open_virtual) {
    int result = shared_midi_open_virtual(NULL, "test");
    ASSERT_EQ(result, -1);
}

TEST(midi_null_context_open_virtual_null_name) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    int result = shared_midi_open_virtual(&ctx, NULL);
    ASSERT_EQ(result, -1);
}

TEST(midi_null_context_open_by_name) {
    int result = shared_midi_open_by_name(NULL, "test");
    ASSERT_EQ(result, -1);
}

TEST(midi_null_context_open_by_name_null_name) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    int result = shared_midi_open_by_name(&ctx, NULL);
    ASSERT_EQ(result, -1);
}

TEST(midi_null_context_open_auto) {
    int result = shared_midi_open_auto(NULL, "test");
    ASSERT_EQ(result, -1);
}

TEST(midi_null_context_close) {
    /* Should not crash */
    shared_midi_close(NULL);
}

TEST(midi_null_context_is_open) {
    int open = shared_midi_is_open(NULL);
    ASSERT_EQ(open, 0);
}

TEST(midi_null_context_send_note_on) {
    /* Should not crash */
    shared_midi_send_note_on(NULL, 1, 60, 100);
}

TEST(midi_null_context_send_note_off) {
    /* Should not crash */
    shared_midi_send_note_off(NULL, 1, 60);
}

TEST(midi_null_context_send_program) {
    /* Should not crash */
    shared_midi_send_program(NULL, 1, 0);
}

TEST(midi_null_context_send_cc) {
    /* Should not crash */
    shared_midi_send_cc(NULL, 1, 7, 100);
}

TEST(midi_null_context_all_notes_off) {
    /* Should not crash */
    shared_midi_all_notes_off(NULL);
}

/* ============================================================================
 * MIDI Input NULL Context Tests
 * ============================================================================ */

TEST(midi_in_null_context_init_observer) {
    /* Should not crash */
    shared_midi_in_init_observer(NULL);
}

TEST(midi_in_null_context_get_port_count) {
    int count = shared_midi_in_get_port_count(NULL);
    ASSERT_EQ(count, 0);
}

TEST(midi_in_null_context_get_port_name) {
    const char *name = shared_midi_in_get_port_name(NULL, 0);
    ASSERT_NULL(name);
}

TEST(midi_in_null_context_open_port) {
    int result = shared_midi_in_open_port(NULL, 0);
    ASSERT_EQ(result, -1);
}

TEST(midi_in_null_context_open_virtual) {
    int result = shared_midi_in_open_virtual(NULL, "test");
    ASSERT_EQ(result, -1);
}

TEST(midi_in_null_context_open_virtual_null_name) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    int result = shared_midi_in_open_virtual(&ctx, NULL);
    ASSERT_EQ(result, -1);
}

TEST(midi_in_null_context_close) {
    /* Should not crash */
    shared_midi_in_close(NULL);
}

TEST(midi_in_null_context_is_open) {
    int open = shared_midi_in_is_open(NULL);
    ASSERT_EQ(open, 0);
}

TEST(midi_in_null_context_cleanup) {
    /* Should not crash */
    shared_midi_in_cleanup(NULL);
}

TEST(midi_in_null_context_set_note_callback) {
    /* Should not crash */
    shared_midi_set_note_callback(NULL, NULL, NULL);
}

/* ============================================================================
 * Context State Tests
 * ============================================================================ */

TEST(midi_context_init_sets_defaults) {
    SharedContext ctx;
    int result = shared_context_init(&ctx);
    ASSERT_EQ(result, 0);

    /* Default values should be set */
    ASSERT_EQ(ctx.tempo, 120);
    ASSERT_EQ(ctx.default_channel, 1);

    /* Backend flags should be off */
    ASSERT_EQ(ctx.minihost_enabled, 0);
    ASSERT_EQ(ctx.builtin_synth_enabled, 0);
    ASSERT_EQ(ctx.csound_enabled, 0);
    ASSERT_EQ(ctx.link_enabled, 0);

    shared_context_cleanup(&ctx);
}

TEST(midi_context_init_null) {
    int result = shared_context_init(NULL);
    ASSERT_EQ(result, -1);
}

TEST(midi_context_cleanup_null) {
    /* Should not crash */
    shared_context_cleanup(NULL);
}

TEST(midi_is_open_with_no_output) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* No midi_out set, should report closed */
    int open = shared_midi_is_open(&ctx);
    ASSERT_EQ(open, 0);
}

TEST(midi_in_is_open_with_no_input) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* No midi_in set, should report closed */
    int open = shared_midi_in_is_open(&ctx);
    ASSERT_EQ(open, 0);
}

TEST(midi_port_count_before_init) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Observer not initialized, port count should be 0 */
    int count = shared_midi_get_port_count(&ctx);
    ASSERT_EQ(count, 0);
}

TEST(midi_in_port_count_before_init) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Observer not initialized, port count should be 0 */
    int count = shared_midi_in_get_port_count(&ctx);
    ASSERT_EQ(count, 0);
}

TEST(midi_port_name_invalid_index_negative) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    const char *name = shared_midi_get_port_name(&ctx, -1);
    ASSERT_NULL(name);
}

TEST(midi_port_name_invalid_index_too_large) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    const char *name = shared_midi_get_port_name(&ctx, 100);
    ASSERT_NULL(name);
}

TEST(midi_in_port_name_invalid_index_negative) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    const char *name = shared_midi_in_get_port_name(&ctx, -1);
    ASSERT_NULL(name);
}

TEST(midi_in_port_name_invalid_index_too_large) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    const char *name = shared_midi_in_get_port_name(&ctx, 100);
    ASSERT_NULL(name);
}

/* ============================================================================
 * Invalid Port Index Tests
 * ============================================================================ */

TEST(midi_open_port_invalid_negative) {
    SharedContext ctx;
    int result = shared_context_init(&ctx);
    ASSERT_EQ(result, 0);

    result = shared_midi_open_port(&ctx, -1);
    ASSERT_EQ(result, -1);

    shared_context_cleanup(&ctx);
}

TEST(midi_open_port_invalid_too_large) {
    SharedContext ctx;
    int result = shared_context_init(&ctx);
    ASSERT_EQ(result, 0);

    /* Try to open port index beyond what's available */
    result = shared_midi_open_port(&ctx, 9999);
    ASSERT_EQ(result, -1);

    shared_context_cleanup(&ctx);
}

TEST(midi_in_open_port_invalid_negative) {
    SharedContext ctx;
    int result = shared_context_init(&ctx);
    ASSERT_EQ(result, 0);

    result = shared_midi_in_open_port(&ctx, -1);
    ASSERT_EQ(result, -1);

    shared_context_cleanup(&ctx);
}

TEST(midi_in_open_port_invalid_too_large) {
    SharedContext ctx;
    int result = shared_context_init(&ctx);
    ASSERT_EQ(result, 0);

    /* Try to open port index beyond what's available */
    result = shared_midi_in_open_port(&ctx, 9999);
    ASSERT_EQ(result, -1);

    shared_context_cleanup(&ctx);
}

/* ============================================================================
 * Note Callback Tests
 * ============================================================================ */

static int callback_invoked = 0;
static int callback_channel = 0;
static int callback_note = 0;
static int callback_velocity = 0;
static int callback_is_note_on = 0;

static void test_note_callback(void *user_data, int channel, int note,
                               int velocity, int is_note_on) {
    (void)user_data;
    callback_invoked = 1;
    callback_channel = channel;
    callback_note = note;
    callback_velocity = velocity;
    callback_is_note_on = is_note_on;
}

TEST(midi_set_note_callback) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Register callback */
    shared_midi_set_note_callback(&ctx, test_note_callback, &ctx);

    /* Verify callback is stored */
    ASSERT_TRUE(ctx.midi_note_callback == test_note_callback);
    ASSERT_TRUE(ctx.midi_note_user_data == &ctx);
}

TEST(midi_unset_note_callback) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Register callback */
    shared_midi_set_note_callback(&ctx, test_note_callback, &ctx);

    /* Unregister callback */
    shared_midi_set_note_callback(&ctx, NULL, NULL);

    /* Verify callback is cleared */
    ASSERT_NULL(ctx.midi_note_callback);
    ASSERT_NULL(ctx.midi_note_user_data);
}

/* ============================================================================
 * Message Send Without Open Port Tests
 *
 * These test that send functions don't crash when no port is open.
 * ============================================================================ */

TEST(midi_send_note_on_no_port) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Should not crash when midi_out is NULL */
    shared_midi_send_note_on(&ctx, 1, 60, 100);
}

TEST(midi_send_note_off_no_port) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Should not crash when midi_out is NULL */
    shared_midi_send_note_off(&ctx, 1, 60);
}

TEST(midi_send_program_no_port) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Should not crash when midi_out is NULL */
    shared_midi_send_program(&ctx, 1, 0);
}

TEST(midi_send_cc_no_port) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Should not crash when midi_out is NULL */
    shared_midi_send_cc(&ctx, 1, 7, 100);
}

TEST(midi_all_notes_off_no_port) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Should not crash when midi_out is NULL */
    shared_midi_all_notes_off(&ctx);
}

/* ============================================================================
 * Timing Utility Tests
 * ============================================================================ */

TEST(midi_ticks_to_ms_120bpm) {
    /* At 120 BPM, one beat = 500ms, 128 ticks per beat */
    /* So 128 ticks = 500ms, 1 tick = ~3.9ms */
    int ms = shared_ticks_to_ms(128, 120);
    ASSERT_EQ(ms, 500);
}

TEST(midi_ticks_to_ms_60bpm) {
    /* At 60 BPM, one beat = 1000ms, 128 ticks per beat */
    int ms = shared_ticks_to_ms(128, 60);
    ASSERT_EQ(ms, 1000);
}

TEST(midi_ticks_to_ms_zero_tempo) {
    /* Zero tempo should default to 120 BPM */
    int ms = shared_ticks_to_ms(128, 0);
    ASSERT_EQ(ms, 500);
}

TEST(midi_ticks_to_ms_negative_tempo) {
    /* Negative tempo should default to 120 BPM */
    int ms = shared_ticks_to_ms(128, -10);
    ASSERT_EQ(ms, 500);
}

TEST(midi_effective_tempo_null_ctx) {
    int tempo = shared_effective_tempo(NULL);
    ASSERT_EQ(tempo, 120);  /* Default */
}

TEST(midi_effective_tempo_no_link) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.tempo = 90;
    ctx.link_enabled = 0;

    int tempo = shared_effective_tempo(&ctx);
    ASSERT_EQ(tempo, 90);  /* Should use context tempo */
}

TEST(midi_sleep_no_sleep_mode) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.no_sleep_mode = 1;

    /* Should return immediately, not actually sleep */
    shared_sleep_ms(&ctx, 1000);
    /* If we get here without delay, test passes */
}

TEST(midi_sleep_zero_ms) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Zero sleep should return immediately */
    shared_sleep_ms(&ctx, 0);
}

TEST(midi_sleep_negative_ms) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Negative sleep should return immediately */
    shared_sleep_ms(&ctx, -100);
}

/* ============================================================================
 * Context Event Dispatch Tests
 *
 * Test the higher-level shared_send_* functions that route to backends.
 * ============================================================================ */

TEST(midi_send_note_on_dispatch_null) {
    /* Should not crash */
    shared_send_note_on(NULL, 1, 60, 100);
}

TEST(midi_send_note_off_dispatch_null) {
    /* Should not crash */
    shared_send_note_off(NULL, 1, 60);
}

TEST(midi_send_program_dispatch_null) {
    /* Should not crash */
    shared_send_program(NULL, 1, 0);
}

TEST(midi_send_cc_dispatch_null) {
    /* Should not crash */
    shared_send_cc(NULL, 1, 7, 100);
}

TEST(midi_send_panic_null) {
    /* Should not crash */
    shared_send_panic(NULL);
}

TEST(midi_send_note_on_freq_null) {
    /* Should not crash */
    shared_send_note_on_freq(NULL, 1, 440.0, 100, 69);
}

TEST(midi_context_full_lifecycle) {
    SharedContext ctx;

    /* Initialize */
    int result = shared_context_init(&ctx);
    ASSERT_EQ(result, 0);

    /* Verify MIDI is not open after init */
    ASSERT_FALSE(shared_midi_is_open(&ctx));
    ASSERT_FALSE(shared_midi_in_is_open(&ctx));

    /* Send some events (should be no-op with no port) */
    shared_send_note_on(&ctx, 1, 60, 100);
    shared_send_note_off(&ctx, 1, 60);
    shared_send_program(&ctx, 1, 0);
    shared_send_cc(&ctx, 1, 7, 100);
    shared_send_panic(&ctx);

    /* Cleanup */
    shared_context_cleanup(&ctx);

    /* After cleanup, queries should still be safe */
    ASSERT_FALSE(shared_midi_is_open(&ctx));
}

/* ============================================================================
 * MIDI Close Tests
 * ============================================================================ */

TEST(midi_close_already_closed) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Closing when already closed should be safe */
    shared_midi_close(&ctx);
    ASSERT_FALSE(shared_midi_is_open(&ctx));
}

TEST(midi_in_close_already_closed) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Closing when already closed should be safe */
    shared_midi_in_close(&ctx);
    ASSERT_FALSE(shared_midi_in_is_open(&ctx));
}

TEST(midi_in_cleanup_already_cleaned) {
    SharedContext ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Double cleanup should be safe */
    shared_midi_in_cleanup(&ctx);
    shared_midi_in_cleanup(&ctx);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

BEGIN_TEST_SUITE("Shared MIDI I/O Tests")

    /* NULL context handling - output */
    RUN_TEST(midi_null_context_init_observer);
    RUN_TEST(midi_null_context_cleanup);
    RUN_TEST(midi_null_context_get_port_count);
    RUN_TEST(midi_null_context_get_port_name);
    RUN_TEST(midi_null_context_open_port);
    RUN_TEST(midi_null_context_open_virtual);
    RUN_TEST(midi_null_context_open_virtual_null_name);
    RUN_TEST(midi_null_context_open_by_name);
    RUN_TEST(midi_null_context_open_by_name_null_name);
    RUN_TEST(midi_null_context_open_auto);
    RUN_TEST(midi_null_context_close);
    RUN_TEST(midi_null_context_is_open);
    RUN_TEST(midi_null_context_send_note_on);
    RUN_TEST(midi_null_context_send_note_off);
    RUN_TEST(midi_null_context_send_program);
    RUN_TEST(midi_null_context_send_cc);
    RUN_TEST(midi_null_context_all_notes_off);

    /* NULL context handling - input */
    RUN_TEST(midi_in_null_context_init_observer);
    RUN_TEST(midi_in_null_context_get_port_count);
    RUN_TEST(midi_in_null_context_get_port_name);
    RUN_TEST(midi_in_null_context_open_port);
    RUN_TEST(midi_in_null_context_open_virtual);
    RUN_TEST(midi_in_null_context_open_virtual_null_name);
    RUN_TEST(midi_in_null_context_close);
    RUN_TEST(midi_in_null_context_is_open);
    RUN_TEST(midi_in_null_context_cleanup);
    RUN_TEST(midi_in_null_context_set_note_callback);

    /* Context state tests */
    RUN_TEST(midi_context_init_sets_defaults);
    RUN_TEST(midi_context_init_null);
    RUN_TEST(midi_context_cleanup_null);
    RUN_TEST(midi_is_open_with_no_output);
    RUN_TEST(midi_in_is_open_with_no_input);
    RUN_TEST(midi_port_count_before_init);
    RUN_TEST(midi_in_port_count_before_init);
    RUN_TEST(midi_port_name_invalid_index_negative);
    RUN_TEST(midi_port_name_invalid_index_too_large);
    RUN_TEST(midi_in_port_name_invalid_index_negative);
    RUN_TEST(midi_in_port_name_invalid_index_too_large);

    /* Invalid port index tests */
    RUN_TEST(midi_open_port_invalid_negative);
    RUN_TEST(midi_open_port_invalid_too_large);
    RUN_TEST(midi_in_open_port_invalid_negative);
    RUN_TEST(midi_in_open_port_invalid_too_large);

    /* Note callback tests */
    RUN_TEST(midi_set_note_callback);
    RUN_TEST(midi_unset_note_callback);

    /* Message send without port tests */
    RUN_TEST(midi_send_note_on_no_port);
    RUN_TEST(midi_send_note_off_no_port);
    RUN_TEST(midi_send_program_no_port);
    RUN_TEST(midi_send_cc_no_port);
    RUN_TEST(midi_all_notes_off_no_port);

    /* Timing utility tests */
    RUN_TEST(midi_ticks_to_ms_120bpm);
    RUN_TEST(midi_ticks_to_ms_60bpm);
    RUN_TEST(midi_ticks_to_ms_zero_tempo);
    RUN_TEST(midi_ticks_to_ms_negative_tempo);
    RUN_TEST(midi_effective_tempo_null_ctx);
    RUN_TEST(midi_effective_tempo_no_link);
    RUN_TEST(midi_sleep_no_sleep_mode);
    RUN_TEST(midi_sleep_zero_ms);
    RUN_TEST(midi_sleep_negative_ms);

    /* Event dispatch tests */
    RUN_TEST(midi_send_note_on_dispatch_null);
    RUN_TEST(midi_send_note_off_dispatch_null);
    RUN_TEST(midi_send_program_dispatch_null);
    RUN_TEST(midi_send_cc_dispatch_null);
    RUN_TEST(midi_send_panic_null);
    RUN_TEST(midi_send_note_on_freq_null);

    /* Full lifecycle test */
    RUN_TEST(midi_context_full_lifecycle);

    /* Close tests */
    RUN_TEST(midi_close_already_closed);
    RUN_TEST(midi_in_close_already_closed);
    RUN_TEST(midi_in_cleanup_already_cleaned);

END_TEST_SUITE()
