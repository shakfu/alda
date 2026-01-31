/**
 * @file test_link.c
 * @brief Tests for Ableton Link shared backend.
 *
 * Tests verify basic Link functionality:
 * - Initialization and cleanup cycles
 * - Enable/disable state
 * - Tempo get/set
 * - Peer count (will be 0 in tests)
 * - Start/stop sync
 */

#include "test_framework.h"
#include "link/link.h"

/* ============================================================================
 * Initialization Tests
 * ============================================================================ */

TEST(link_init_cleanup_cycle) {
    /* Init should succeed */
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);
    ASSERT_TRUE(shared_link_is_initialized());

    /* Cleanup should not crash */
    shared_link_cleanup();
    ASSERT_FALSE(shared_link_is_initialized());
}

TEST(link_double_init) {
    /* First init */
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    /* Second init should succeed (idempotent) */
    result = shared_link_init(140.0);
    ASSERT_EQ(result, 0);

    shared_link_cleanup();
}

TEST(link_cleanup_without_init) {
    /* Cleanup without init should not crash */
    shared_link_cleanup();
}

/* ============================================================================
 * Enable/Disable Tests
 * ============================================================================ */

TEST(link_disabled_initially) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    /* Link should be disabled initially */
    ASSERT_FALSE(shared_link_is_enabled());

    shared_link_cleanup();
}

TEST(link_enable_disable) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    /* Enable Link */
    shared_link_enable(1);
    ASSERT_TRUE(shared_link_is_enabled());

    /* Disable Link */
    shared_link_enable(0);
    ASSERT_FALSE(shared_link_is_enabled());

    shared_link_cleanup();
}

TEST(link_enable_when_not_initialized) {
    /* Should not crash when not initialized */
    shared_link_enable(1);
    ASSERT_FALSE(shared_link_is_enabled());
}

/* ============================================================================
 * Tempo Tests
 * ============================================================================ */

TEST(link_initial_tempo) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    /* Should have initial tempo */
    double tempo = shared_link_get_tempo();
    ASSERT_TRUE(tempo > 0);

    shared_link_cleanup();
}

TEST(link_set_tempo) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    shared_link_enable(1);

    /* Set tempo */
    shared_link_set_tempo(140.0);
    double tempo = shared_link_get_tempo();
    /* Allow small floating point difference */
    ASSERT_TRUE(tempo >= 139.0 && tempo <= 141.0);

    shared_link_cleanup();
}

TEST(link_effective_tempo_disabled) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    /* When disabled, should return fallback */
    double tempo = shared_link_effective_tempo(90.0);
    ASSERT_TRUE(tempo >= 89.0 && tempo <= 91.0);

    shared_link_cleanup();
}

TEST(link_effective_tempo_enabled) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    shared_link_enable(1);
    shared_link_set_tempo(150.0);

    /* When enabled, should return Link tempo */
    double tempo = shared_link_effective_tempo(90.0);
    ASSERT_TRUE(tempo >= 149.0 && tempo <= 151.0);

    shared_link_cleanup();
}

/* ============================================================================
 * Peer Tests
 * ============================================================================ */

TEST(link_no_peers_in_test) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    shared_link_enable(1);

    /* In test environment, we have no peers */
    uint64_t peers = shared_link_num_peers();
    ASSERT_EQ(peers, 0);

    shared_link_cleanup();
}

/* ============================================================================
 * Start/Stop Sync Tests
 * ============================================================================ */

TEST(link_start_stop_sync_disabled_initially) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    ASSERT_FALSE(shared_link_is_start_stop_sync_enabled());

    shared_link_cleanup();
}

TEST(link_enable_start_stop_sync) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    shared_link_enable_start_stop_sync(1);
    ASSERT_TRUE(shared_link_is_start_stop_sync_enabled());

    shared_link_enable_start_stop_sync(0);
    ASSERT_FALSE(shared_link_is_start_stop_sync_enabled());

    shared_link_cleanup();
}

TEST(link_playing_state) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    shared_link_enable(1);

    /* Initially not playing */
    ASSERT_FALSE(shared_link_is_playing());

    /* Set playing */
    shared_link_set_playing(1);
    ASSERT_TRUE(shared_link_is_playing());

    /* Stop playing */
    shared_link_set_playing(0);
    ASSERT_FALSE(shared_link_is_playing());

    shared_link_cleanup();
}

/* ============================================================================
 * Beat/Phase Tests
 * ============================================================================ */

TEST(link_get_beat) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    shared_link_enable(1);

    /* Should return a beat value (may be 0 or any value) */
    double beat = shared_link_get_beat(4.0);
    /* Just verify it doesn't crash and returns a reasonable value */
    ASSERT_TRUE(beat >= 0.0);

    shared_link_cleanup();
}

TEST(link_get_phase) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    shared_link_enable(1);

    /* Phase should be in range [0, quantum) */
    double phase = shared_link_get_phase(4.0);
    ASSERT_TRUE(phase >= 0.0 && phase < 4.0);

    shared_link_cleanup();
}

/* ============================================================================
 * Callback Tests
 * ============================================================================ */

/* Callback tracking variables */
static int g_peers_callback_count = 0;
static uint64_t g_peers_callback_value = 0;
static void *g_peers_callback_userdata = NULL;

static int g_tempo_callback_count = 0;
static double g_tempo_callback_value = 0.0;
static void *g_tempo_callback_userdata = NULL;

static int g_transport_callback_count = 0;
static int g_transport_callback_value = 0;
static void *g_transport_callback_userdata = NULL;

static void reset_callback_state(void) {
    g_peers_callback_count = 0;
    g_peers_callback_value = 0;
    g_peers_callback_userdata = NULL;
    g_tempo_callback_count = 0;
    g_tempo_callback_value = 0.0;
    g_tempo_callback_userdata = NULL;
    g_transport_callback_count = 0;
    g_transport_callback_value = 0;
    g_transport_callback_userdata = NULL;
}

static void test_peers_callback(uint64_t num_peers, void *userdata) {
    g_peers_callback_count++;
    g_peers_callback_value = num_peers;
    g_peers_callback_userdata = userdata;
}

static void test_tempo_callback(double tempo, void *userdata) {
    g_tempo_callback_count++;
    g_tempo_callback_value = tempo;
    g_tempo_callback_userdata = userdata;
}

static void test_transport_callback(int is_playing, void *userdata) {
    g_transport_callback_count++;
    g_transport_callback_value = is_playing;
    g_transport_callback_userdata = userdata;
}

TEST(link_set_peers_callback) {
    reset_callback_state();
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    int userdata_marker = 42;

    /* Set callback */
    shared_link_set_peers_callback(test_peers_callback, &userdata_marker);

    /* Clear callback */
    shared_link_set_peers_callback(NULL, NULL);

    shared_link_cleanup();
}

TEST(link_set_tempo_callback) {
    reset_callback_state();
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    int userdata_marker = 42;

    /* Set callback */
    shared_link_set_tempo_callback(test_tempo_callback, &userdata_marker);

    /* Clear callback */
    shared_link_set_tempo_callback(NULL, NULL);

    shared_link_cleanup();
}

TEST(link_set_transport_callback) {
    reset_callback_state();
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    int userdata_marker = 42;

    /* Set callback */
    shared_link_set_transport_callback(test_transport_callback, &userdata_marker);

    /* Clear callback */
    shared_link_set_transport_callback(NULL, NULL);

    shared_link_cleanup();
}

TEST(link_check_callbacks_not_initialized) {
    /* Should not crash when not initialized */
    shared_link_check_callbacks();
}

TEST(link_check_callbacks_no_callbacks) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    /* Should not crash with no callbacks set */
    shared_link_check_callbacks();

    shared_link_cleanup();
}

TEST(link_callbacks_not_called_when_not_initialized) {
    reset_callback_state();

    /* Set callbacks before init */
    shared_link_set_peers_callback(test_peers_callback, NULL);
    shared_link_set_tempo_callback(test_tempo_callback, NULL);
    shared_link_set_transport_callback(test_transport_callback, NULL);

    /* Check callbacks (Link not initialized) */
    shared_link_check_callbacks();

    /* Should not have been called */
    ASSERT_EQ(g_peers_callback_count, 0);
    ASSERT_EQ(g_tempo_callback_count, 0);
    ASSERT_EQ(g_transport_callback_count, 0);
}

TEST(link_ms_to_next_beat_disabled) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    /* When Link is disabled, should return 0 */
    int ms = shared_link_ms_to_next_beat(4.0);
    ASSERT_EQ(ms, 0);

    shared_link_cleanup();
}

TEST(link_ms_to_next_beat_enabled) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    shared_link_enable(1);

    /* When enabled, should return some value (may be 0 or positive) */
    int ms = shared_link_ms_to_next_beat(4.0);
    ASSERT_TRUE(ms >= 0);

    shared_link_cleanup();
}

TEST(link_tempo_clamping_low) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    shared_link_enable(1);

    /* Set tempo below minimum (20 BPM) */
    shared_link_set_tempo(10.0);
    double tempo = shared_link_get_tempo();
    /* Should be clamped to minimum */
    ASSERT_TRUE(tempo >= 20.0);

    shared_link_cleanup();
}

TEST(link_tempo_clamping_high) {
    int result = shared_link_init(120.0);
    ASSERT_EQ(result, 0);

    shared_link_enable(1);

    /* Set tempo above maximum (999 BPM) */
    shared_link_set_tempo(1500.0);
    double tempo = shared_link_get_tempo();
    /* Should be clamped to maximum */
    ASSERT_TRUE(tempo <= 999.0);

    shared_link_cleanup();
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

BEGIN_TEST_SUITE("Ableton Link Tests")

    /* Initialization */
    RUN_TEST(link_init_cleanup_cycle);
    RUN_TEST(link_double_init);
    RUN_TEST(link_cleanup_without_init);

    /* Enable/Disable */
    RUN_TEST(link_disabled_initially);
    RUN_TEST(link_enable_disable);
    RUN_TEST(link_enable_when_not_initialized);

    /* Tempo */
    RUN_TEST(link_initial_tempo);
    RUN_TEST(link_set_tempo);
    RUN_TEST(link_effective_tempo_disabled);
    RUN_TEST(link_effective_tempo_enabled);

    /* Peers */
    RUN_TEST(link_no_peers_in_test);

    /* Start/Stop Sync */
    RUN_TEST(link_start_stop_sync_disabled_initially);
    RUN_TEST(link_enable_start_stop_sync);
    RUN_TEST(link_playing_state);

    /* Beat/Phase */
    RUN_TEST(link_get_beat);
    RUN_TEST(link_get_phase);

    /* Callbacks */
    RUN_TEST(link_set_peers_callback);
    RUN_TEST(link_set_tempo_callback);
    RUN_TEST(link_set_transport_callback);
    RUN_TEST(link_check_callbacks_not_initialized);
    RUN_TEST(link_check_callbacks_no_callbacks);
    RUN_TEST(link_callbacks_not_called_when_not_initialized);

    /* Timing */
    RUN_TEST(link_ms_to_next_beat_disabled);
    RUN_TEST(link_ms_to_next_beat_enabled);

    /* Tempo clamping */
    RUN_TEST(link_tempo_clamping_low);
    RUN_TEST(link_tempo_clamping_high);

END_TEST_SUITE()
