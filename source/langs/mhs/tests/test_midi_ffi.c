/**
 * @file test_midi_ffi.c
 * @brief Unit tests for MHS MIDI FFI functions.
 *
 * Tests the MIDI FFI layer used by MicroHaskell for MIDI output.
 * Focuses on pure functions and state management that don't require
 * actual MIDI hardware.
 */

#include "test_framework.h"
#include "midi_ffi.h"
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * Cents to Pitch Bend Conversion Tests
 *
 * The function returns absolute MIDI pitch bend values (0-16383).
 * Center (no bend) = 8192
 * Standard range is +/- 2 semitones (200 cents)
 * Formula: bend = 8192 + (cents * 8192 / 200)
 *============================================================================*/

TEST(cents_to_bend_zero) {
    /* 0 cents should give center (8192) */
    int bend = midi_cents_to_bend(0);
    ASSERT_EQ(bend, 8192);
}

TEST(cents_to_bend_semitone_up) {
    /* +100 cents (1 semitone) = 8192 + 4096 = 12288 */
    int bend = midi_cents_to_bend(100);
    ASSERT_EQ(bend, 12288);
}

TEST(cents_to_bend_semitone_down) {
    /* -100 cents (1 semitone down) = 8192 - 4096 = 4096 */
    int bend = midi_cents_to_bend(-100);
    ASSERT_EQ(bend, 4096);
}

TEST(cents_to_bend_two_semitones_up) {
    /* +200 cents (2 semitones) = 8192 + 8192 = 16384 -> clamps to 16383 */
    int bend = midi_cents_to_bend(200);
    ASSERT_EQ(bend, 16383);
}

TEST(cents_to_bend_two_semitones_down) {
    /* -200 cents (2 semitones down) = 8192 - 8192 = 0 */
    int bend = midi_cents_to_bend(-200);
    ASSERT_EQ(bend, 0);
}

TEST(cents_to_bend_quarter_tone) {
    /* +50 cents (quarter tone) = 8192 + 2048 = 10240 */
    int bend = midi_cents_to_bend(50);
    ASSERT_EQ(bend, 10240);
}

TEST(cents_to_bend_clamps_high) {
    /* Values > 200 cents should clamp to max (16383) */
    int bend = midi_cents_to_bend(300);
    ASSERT_EQ(bend, 16383);
}

TEST(cents_to_bend_clamps_low) {
    /* Values < -200 cents should clamp to min (0) */
    int bend = midi_cents_to_bend(-300);
    ASSERT_EQ(bend, 0);
}

/*============================================================================
 * Random Number Generation Tests
 *============================================================================*/

TEST(random_range_same_min_max) {
    /* When min == max, should return min */
    int result = midi_random_range(42, 42);
    ASSERT_EQ(result, 42);
}

TEST(random_range_min_greater_than_max) {
    /* When min > max, should return min */
    int result = midi_random_range(100, 50);
    ASSERT_EQ(result, 100);
}

TEST(random_range_values_in_range) {
    /* Random values should be within range */
    midi_seed_random(12345);
    for (int i = 0; i < 100; i++) {
        int result = midi_random_range(0, 127);
        ASSERT_TRUE(result >= 0);
        ASSERT_TRUE(result <= 127);
    }
}

TEST(random_range_different_values) {
    /* Multiple calls should produce different values (statistical) */
    midi_seed_random(54321);
    int values[10];
    for (int i = 0; i < 10; i++) {
        values[i] = midi_random_range(0, 1000);
    }
    /* At least one value should differ from the first */
    int all_same = 1;
    for (int i = 1; i < 10; i++) {
        if (values[i] != values[0]) {
            all_same = 0;
            break;
        }
    }
    ASSERT_FALSE(all_same);
}

TEST(seed_random_reproducible) {
    /* Same seed should produce same sequence */
    midi_seed_random(99999);
    int first = midi_random();
    int second = midi_random();

    midi_seed_random(99999);
    ASSERT_EQ(midi_random(), first);
    ASSERT_EQ(midi_random(), second);
}

/*============================================================================
 * Recording State Tests
 *============================================================================*/

TEST(record_initially_inactive) {
    /* Recording should start inactive */
    ASSERT_EQ(midi_record_active(), 0);
}

TEST(record_start_activates) {
    int result = midi_record_start(120);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(midi_record_active(), 1);

    /* Clean up */
    midi_record_stop();
}

TEST(record_stop_deactivates) {
    midi_record_start(120);
    midi_record_stop();
    ASSERT_EQ(midi_record_active(), 0);
}

TEST(record_count_initially_zero) {
    /* After stop, count should reflect captured events */
    midi_record_start(120);
    int count = midi_record_stop();
    /* No events sent, count should be 0 */
    ASSERT_EQ(count, 0);
    ASSERT_EQ(midi_record_count(), 0);
}

TEST(record_bpm_clamped_low) {
    /* BPM < 1 should be clamped to 1 */
    midi_record_start(0);
    ASSERT_EQ(midi_record_active(), 1);
    midi_record_stop();
}

TEST(record_bpm_clamped_high) {
    /* BPM > 300 should be clamped to 300 */
    midi_record_start(500);
    ASSERT_EQ(midi_record_active(), 1);
    midi_record_stop();
}

/*============================================================================
 * Channel Validation Tests (without actual MIDI output)
 *============================================================================*/

TEST(note_on_invalid_channel_zero) {
    /* Channel 0 is invalid (valid range: 1-16) */
    int result = midi_note_on(0, 60, 100);
    ASSERT_EQ(result, -1);
}

TEST(note_on_invalid_channel_17) {
    /* Channel 17 is invalid */
    int result = midi_note_on(17, 60, 100);
    ASSERT_EQ(result, -1);
}

TEST(note_off_invalid_channel_zero) {
    int result = midi_note_off(0, 60);
    ASSERT_EQ(result, -1);
}

TEST(note_off_invalid_channel_17) {
    int result = midi_note_off(17, 60);
    ASSERT_EQ(result, -1);
}

TEST(cc_invalid_channel_zero) {
    int result = midi_cc(0, 7, 100);
    ASSERT_EQ(result, -1);
}

TEST(cc_invalid_channel_17) {
    int result = midi_cc(17, 7, 100);
    ASSERT_EQ(result, -1);
}

TEST(program_invalid_channel_zero) {
    int result = midi_program(0, 0);
    ASSERT_EQ(result, -1);
}

TEST(program_invalid_channel_17) {
    int result = midi_program(17, 0);
    ASSERT_EQ(result, -1);
}

/*============================================================================
 * Port Name Tests
 *============================================================================*/

TEST(port_name_invalid_negative) {
    /* Invalid index should return empty string */
    const char *name = midi_port_name(-1);
    ASSERT_NOT_NULL(name);
    ASSERT_EQ(strlen(name), (size_t)0);
}

TEST(port_name_invalid_large) {
    /* Invalid index should return empty string */
    const char *name = midi_port_name(9999);
    ASSERT_NOT_NULL(name);
    ASSERT_EQ(strlen(name), (size_t)0);
}

/*============================================================================
 * Initialization State Tests
 *============================================================================*/

TEST(is_open_initially_false) {
    /* Before opening any port, should return false */
    /* Note: cleanup may have been called by previous tests */
    mhs_midi_cleanup();
    ASSERT_EQ(midi_is_open(), 0);
}

TEST(open_invalid_port_fails) {
    /* Opening non-existent port should fail */
    mhs_midi_init();
    int result = midi_open(-1);
    ASSERT_EQ(result, -1);
    mhs_midi_cleanup();
}

TEST(open_port_out_of_range_fails) {
    /* Opening port beyond count should fail */
    mhs_midi_init();
    int result = midi_open(9999);
    ASSERT_EQ(result, -1);
    mhs_midi_cleanup();
}

/*============================================================================
 * Main Test Runner
 *============================================================================*/

BEGIN_TEST_SUITE("MHS MIDI FFI Tests")
    /* Cents to bend conversion */
    RUN_TEST(cents_to_bend_zero);
    RUN_TEST(cents_to_bend_semitone_up);
    RUN_TEST(cents_to_bend_semitone_down);
    RUN_TEST(cents_to_bend_two_semitones_up);
    RUN_TEST(cents_to_bend_two_semitones_down);
    RUN_TEST(cents_to_bend_quarter_tone);
    RUN_TEST(cents_to_bend_clamps_high);
    RUN_TEST(cents_to_bend_clamps_low);

    /* Random number generation */
    RUN_TEST(random_range_same_min_max);
    RUN_TEST(random_range_min_greater_than_max);
    RUN_TEST(random_range_values_in_range);
    RUN_TEST(random_range_different_values);
    RUN_TEST(seed_random_reproducible);

    /* Recording state */
    RUN_TEST(record_initially_inactive);
    RUN_TEST(record_start_activates);
    RUN_TEST(record_stop_deactivates);
    RUN_TEST(record_count_initially_zero);
    RUN_TEST(record_bpm_clamped_low);
    RUN_TEST(record_bpm_clamped_high);

    /* Channel validation */
    RUN_TEST(note_on_invalid_channel_zero);
    RUN_TEST(note_on_invalid_channel_17);
    RUN_TEST(note_off_invalid_channel_zero);
    RUN_TEST(note_off_invalid_channel_17);
    RUN_TEST(cc_invalid_channel_zero);
    RUN_TEST(cc_invalid_channel_17);
    RUN_TEST(program_invalid_channel_zero);
    RUN_TEST(program_invalid_channel_17);

    /* Port names */
    RUN_TEST(port_name_invalid_negative);
    RUN_TEST(port_name_invalid_large);

    /* Initialization */
    RUN_TEST(is_open_initially_false);
    RUN_TEST(open_invalid_port_fails);
    RUN_TEST(open_port_out_of_range_fails);
END_TEST_SUITE()
