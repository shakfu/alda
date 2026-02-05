/**
 * @file test_osc.c
 * @brief Unit tests for OSC (Open Sound Control) module.
 *
 * Tests both stub implementations (when OSC disabled) and
 * actual functionality (when PSND_OSC is defined).
 */

#include "test_framework.h"
#include "osc/osc.h"
#include "context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * Helper Functions
 *============================================================================*/

/* Create a minimal SharedContext for testing */
static SharedContext *create_test_context(void) {
    SharedContext *ctx = calloc(1, sizeof(SharedContext));
    if (!ctx) return NULL;

    ctx->tempo = 120;
    ctx->default_channel = 0;
    ctx->osc_enabled = 0;
    ctx->osc_port = 0;
    ctx->osc_server = NULL;
    ctx->osc_broadcast = NULL;
    ctx->osc_user_data = NULL;

    return ctx;
}

static void free_test_context(SharedContext *ctx) {
    if (ctx) {
        shared_osc_cleanup(ctx);
        free(ctx);
    }
}

/*============================================================================
 * Null Safety Tests
 *============================================================================*/

TEST(osc_init_null_context) {
    int result = shared_osc_init(NULL, 7770);
    ASSERT_EQ(result, -1);
}

TEST(osc_init_with_iface_null_context) {
    int result = shared_osc_init_with_iface(NULL, 7770, NULL);
    ASSERT_EQ(result, -1);
}

TEST(osc_init_multicast_null_context) {
    int result = shared_osc_init_multicast(NULL, "224.0.0.1", 7770);
    ASSERT_EQ(result, -1);
}

TEST(osc_set_broadcast_null_context) {
    int result = shared_osc_set_broadcast(NULL, "127.0.0.1", "7771");
    ASSERT_EQ(result, -1);
}

TEST(osc_start_null_context) {
    int result = shared_osc_start(NULL);
    ASSERT_EQ(result, -1);
}

TEST(osc_cleanup_null_context_safe) {
    /* Should not crash */
    shared_osc_cleanup(NULL);
    ASSERT_TRUE(1);
}

TEST(osc_is_running_null_context) {
    int result = shared_osc_is_running(NULL);
    ASSERT_EQ(result, 0);
}

TEST(osc_get_port_null_context) {
    int port = shared_osc_get_port(NULL);
    ASSERT_EQ(port, 0);
}

TEST(osc_set_user_data_null_context_safe) {
    /* Should not crash */
    shared_osc_set_user_data(NULL, (void *)0x1234);
    ASSERT_TRUE(1);
}

/*============================================================================
 * Stub Tests (when OSC disabled)
 *============================================================================*/

#ifndef PSND_OSC

TEST(stub_init_returns_error) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    int result = shared_osc_init(ctx, 7770);
    ASSERT_EQ(result, -1);

    free_test_context(ctx);
}

TEST(stub_init_with_iface_returns_error) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    int result = shared_osc_init_with_iface(ctx, 7770, "127.0.0.1");
    ASSERT_EQ(result, -1);

    free_test_context(ctx);
}

TEST(stub_init_multicast_returns_error) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    int result = shared_osc_init_multicast(ctx, "224.0.0.1", 7770);
    ASSERT_EQ(result, -1);

    free_test_context(ctx);
}

TEST(stub_set_broadcast_returns_error) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    int result = shared_osc_set_broadcast(ctx, "127.0.0.1", "7771");
    ASSERT_EQ(result, -1);

    free_test_context(ctx);
}

TEST(stub_start_returns_error) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    int result = shared_osc_start(ctx);
    ASSERT_EQ(result, -1);

    free_test_context(ctx);
}

TEST(stub_is_running_returns_false) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    int result = shared_osc_is_running(ctx);
    ASSERT_EQ(result, 0);

    free_test_context(ctx);
}

TEST(stub_get_port_returns_zero) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    int port = shared_osc_get_port(ctx);
    ASSERT_EQ(port, 0);

    free_test_context(ctx);
}

TEST(stub_send_playing_does_not_crash) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    shared_osc_send_playing(ctx, 1);
    shared_osc_send_playing(ctx, 0);
    ASSERT_TRUE(1);

    free_test_context(ctx);
}

TEST(stub_send_tempo_does_not_crash) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    shared_osc_send_tempo(ctx, 120.0f);
    shared_osc_send_tempo(ctx, 60.5f);
    ASSERT_TRUE(1);

    free_test_context(ctx);
}

TEST(stub_send_note_does_not_crash) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    shared_osc_send_note(ctx, 0, 60, 100);
    shared_osc_send_note(ctx, 9, 36, 127);
    ASSERT_TRUE(1);

    free_test_context(ctx);
}

TEST(stub_set_lang_callbacks_does_not_crash) {
    shared_osc_set_lang_callbacks(NULL, NULL, NULL);
    ASSERT_TRUE(1);
}

TEST(stub_set_query_callbacks_does_not_crash) {
    shared_osc_set_query_callbacks(NULL, NULL, NULL);
    ASSERT_TRUE(1);
}

TEST(stub_rate_limit_returns_zero) {
    shared_osc_set_note_rate_limit(100);
    int limit = shared_osc_get_note_rate_limit();
    ASSERT_EQ(limit, 0);  /* Stubs always return 0 */
}

#endif /* !PSND_OSC */

/*============================================================================
 * Rate Limit API Tests (work in both modes)
 *============================================================================*/

#ifdef PSND_OSC

TEST(rate_limit_set_and_get) {
    shared_osc_set_note_rate_limit(50);
    int limit = shared_osc_get_note_rate_limit();
    ASSERT_EQ(limit, 50);
}

TEST(rate_limit_zero_means_unlimited) {
    shared_osc_set_note_rate_limit(0);
    int limit = shared_osc_get_note_rate_limit();
    ASSERT_EQ(limit, 0);
}

TEST(rate_limit_negative_treated_as_zero) {
    shared_osc_set_note_rate_limit(-10);
    int limit = shared_osc_get_note_rate_limit();
    ASSERT_EQ(limit, 0);
}

TEST(rate_limit_high_value) {
    shared_osc_set_note_rate_limit(10000);
    int limit = shared_osc_get_note_rate_limit();
    ASSERT_EQ(limit, 10000);
}

/*============================================================================
 * Context State Tests (OSC enabled)
 *============================================================================*/

TEST(osc_context_initial_state) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    ASSERT_EQ(ctx->osc_enabled, 0);
    ASSERT_EQ(ctx->osc_port, 0);
    ASSERT_NULL(ctx->osc_server);
    ASSERT_NULL(ctx->osc_broadcast);

    free_test_context(ctx);
}

TEST(osc_cleanup_resets_state) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    /* Try to init (may fail if port in use, that's OK) */
    shared_osc_init(ctx, 0);  /* Use default port */

    /* Cleanup */
    shared_osc_cleanup(ctx);

    ASSERT_EQ(ctx->osc_enabled, 0);
    ASSERT_EQ(ctx->osc_port, 0);
    ASSERT_NULL(ctx->osc_server);

    free(ctx);  /* Don't use free_test_context to avoid double cleanup */
}

TEST(osc_set_user_data) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    void *data = (void *)0xDEADBEEF;
    shared_osc_set_user_data(ctx, data);

    ASSERT_EQ(ctx->osc_user_data, data);

    free_test_context(ctx);
}

TEST(osc_set_user_data_null) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    ctx->osc_user_data = (void *)0x1234;
    shared_osc_set_user_data(ctx, NULL);

    ASSERT_NULL(ctx->osc_user_data);

    free_test_context(ctx);
}

/*============================================================================
 * Callback Registration Tests
 *============================================================================*/

static int dummy_eval(struct editor_ctx *ctx, const char *code) {
    (void)ctx; (void)code;
    return 0;
}

static int dummy_eval_buffer(struct editor_ctx *ctx) {
    (void)ctx;
    return 0;
}

static void dummy_stop_all(struct editor_ctx *ctx) {
    (void)ctx;
}

static int dummy_is_playing(struct editor_ctx *ctx) {
    (void)ctx;
    return 0;
}

static const char *dummy_get_filename(struct editor_ctx *ctx) {
    (void)ctx;
    return "test.alda";
}

static void dummy_get_position(struct editor_ctx *ctx, int *line, int *col) {
    (void)ctx;
    *line = 10;
    *col = 5;
}

TEST(osc_set_lang_callbacks) {
    /* Should not crash and callbacks should be registered */
    shared_osc_set_lang_callbacks(dummy_eval, dummy_eval_buffer, dummy_stop_all);
    ASSERT_TRUE(1);

    /* Reset */
    shared_osc_set_lang_callbacks(NULL, NULL, NULL);
}

TEST(osc_set_query_callbacks) {
    /* Should not crash and callbacks should be registered */
    shared_osc_set_query_callbacks(dummy_is_playing, dummy_get_filename, dummy_get_position);
    ASSERT_TRUE(1);

    /* Reset */
    shared_osc_set_query_callbacks(NULL, NULL, NULL);
}

#endif /* PSND_OSC */

/*============================================================================
 * Default Port Tests
 *============================================================================*/

TEST(osc_default_port_constant) {
    ASSERT_EQ(PSND_OSC_DEFAULT_PORT, 7770);
}

/*============================================================================
 * Broadcast Parameter Validation Tests
 *============================================================================*/

TEST(osc_set_broadcast_null_host) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    int result = shared_osc_set_broadcast(ctx, NULL, "7771");
    ASSERT_EQ(result, -1);

    free_test_context(ctx);
}

TEST(osc_set_broadcast_null_port) {
    SharedContext *ctx = create_test_context();
    ASSERT_NOT_NULL(ctx);

    int result = shared_osc_set_broadcast(ctx, "127.0.0.1", NULL);
    ASSERT_EQ(result, -1);

    free_test_context(ctx);
}

/*============================================================================
 * Main Test Runner
 *============================================================================*/

BEGIN_TEST_SUITE("OSC Tests")
    /* Null safety */
    RUN_TEST(osc_init_null_context);
    RUN_TEST(osc_init_with_iface_null_context);
    RUN_TEST(osc_init_multicast_null_context);
    RUN_TEST(osc_set_broadcast_null_context);
    RUN_TEST(osc_start_null_context);
    RUN_TEST(osc_cleanup_null_context_safe);
    RUN_TEST(osc_is_running_null_context);
    RUN_TEST(osc_get_port_null_context);
    RUN_TEST(osc_set_user_data_null_context_safe);

#ifndef PSND_OSC
    /* Stub tests */
    RUN_TEST(stub_init_returns_error);
    RUN_TEST(stub_init_with_iface_returns_error);
    RUN_TEST(stub_init_multicast_returns_error);
    RUN_TEST(stub_set_broadcast_returns_error);
    RUN_TEST(stub_start_returns_error);
    RUN_TEST(stub_is_running_returns_false);
    RUN_TEST(stub_get_port_returns_zero);
    RUN_TEST(stub_send_playing_does_not_crash);
    RUN_TEST(stub_send_tempo_does_not_crash);
    RUN_TEST(stub_send_note_does_not_crash);
    RUN_TEST(stub_set_lang_callbacks_does_not_crash);
    RUN_TEST(stub_set_query_callbacks_does_not_crash);
    RUN_TEST(stub_rate_limit_returns_zero);
#endif

#ifdef PSND_OSC
    /* Rate limit tests */
    RUN_TEST(rate_limit_set_and_get);
    RUN_TEST(rate_limit_zero_means_unlimited);
    RUN_TEST(rate_limit_negative_treated_as_zero);
    RUN_TEST(rate_limit_high_value);

    /* Context state tests */
    RUN_TEST(osc_context_initial_state);
    RUN_TEST(osc_cleanup_resets_state);
    RUN_TEST(osc_set_user_data);
    RUN_TEST(osc_set_user_data_null);

    /* Callback tests */
    RUN_TEST(osc_set_lang_callbacks);
    RUN_TEST(osc_set_query_callbacks);
#endif

    /* Default port */
    RUN_TEST(osc_default_port_constant);

    /* Broadcast validation */
    RUN_TEST(osc_set_broadcast_null_host);
    RUN_TEST(osc_set_broadcast_null_port);
END_TEST_SUITE()
