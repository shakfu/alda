/**
 * @file test_lang_bridge.c
 * @brief Tests for the language bridge dispatch system.
 *
 * Tests the loki_lang_* functions that manage language registration
 * and dispatch to language-specific implementations.
 */

#include "test_framework.h"
#include "loki/lang_bridge.h"
#include "loki/internal.h"
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * Mock Language Implementation
 * ============================================================================ */

/* Tracking variables for mock language */
static int g_mock_init_count = 0;
static int g_mock_cleanup_count = 0;
static int g_mock_eval_count = 0;
static int g_mock_stop_count = 0;
static int g_mock_is_initialized = 0;
static int g_mock_is_playing = 0;
static const char *g_mock_last_eval_code = NULL;
static char g_mock_error[256] = {0};

static void reset_mock_state(void) {
    g_mock_init_count = 0;
    g_mock_cleanup_count = 0;
    g_mock_eval_count = 0;
    g_mock_stop_count = 0;
    g_mock_is_initialized = 0;
    g_mock_is_playing = 0;
    g_mock_last_eval_code = NULL;
    g_mock_error[0] = '\0';
}

/* Mock language operations */
static int mock_init(editor_ctx_t *ctx) {
    (void)ctx;
    g_mock_init_count++;
    g_mock_is_initialized = 1;
    return 0;
}

static void mock_cleanup(editor_ctx_t *ctx) {
    (void)ctx;
    g_mock_cleanup_count++;
    g_mock_is_initialized = 0;
}

static int mock_is_initialized(editor_ctx_t *ctx) {
    (void)ctx;
    return g_mock_is_initialized;
}

static int mock_eval(editor_ctx_t *ctx, const char *code) {
    (void)ctx;
    g_mock_eval_count++;
    g_mock_last_eval_code = code;
    g_mock_is_playing = 1;
    return 0;
}

static void mock_stop(editor_ctx_t *ctx) {
    (void)ctx;
    g_mock_stop_count++;
    g_mock_is_playing = 0;
}

static int mock_is_playing(editor_ctx_t *ctx) {
    (void)ctx;
    return g_mock_is_playing;
}

static const char *mock_get_error(editor_ctx_t *ctx) {
    (void)ctx;
    return g_mock_error[0] ? g_mock_error : NULL;
}

/* Mock language ops struct - using a test extension that won't conflict */
static const LokiLangOps mock_lang_ops = {
    .name = "mock",
    .extensions = {".mock", ".mck", NULL},
    .init = mock_init,
    .cleanup = mock_cleanup,
    .is_initialized = mock_is_initialized,
    .eval = mock_eval,
    .stop = mock_stop,
    .is_playing = mock_is_playing,
    .get_error = mock_get_error,
    .check_callbacks = NULL,
    .has_events = NULL,
    .populate_shared_buffer = NULL,
    .configure_backend = NULL,
    .register_lua_api = NULL,
};

/* Minimal mock for testing multiple registrations */
static const LokiLangOps mock_lang2_ops = {
    .name = "mock2",
    .extensions = {".mk2", NULL},
    .init = mock_init,
    .cleanup = mock_cleanup,
    .is_initialized = mock_is_initialized,
    .eval = mock_eval,
    .stop = mock_stop,
    .is_playing = NULL,  /* Optional */
    .get_error = NULL,   /* Optional */
};

/* ============================================================================
 * Helper: Create minimal editor context for testing
 * ============================================================================ */

static editor_ctx_t *create_test_context(const char *filename) {
    editor_ctx_t *ctx = calloc(1, sizeof(editor_ctx_t));
    if (!ctx) return NULL;

    if (filename) {
        ctx->model.filename = strdup(filename);
    }

    return ctx;
}

static void free_test_context(editor_ctx_t *ctx) {
    if (!ctx) return;
    free(ctx->model.filename);
    free(ctx);
}

/* ============================================================================
 * Registration Tests
 * ============================================================================ */

TEST(bridge_register_null) {
    /* NULL ops should fail */
    int result = loki_lang_register(NULL);
    ASSERT_EQ(result, -1);
}

TEST(bridge_register_unnamed) {
    /* Unnamed language should fail */
    LokiLangOps unnamed = {0};
    unnamed.name = NULL;
    int result = loki_lang_register(&unnamed);
    ASSERT_EQ(result, -1);
}

TEST(bridge_register_valid) {
    reset_mock_state();
    int result = loki_lang_register(&mock_lang_ops);
    ASSERT_EQ(result, 0);
}

TEST(bridge_register_duplicate) {
    reset_mock_state();
    /* First registration */
    int result1 = loki_lang_register(&mock_lang_ops);
    ASSERT_EQ(result1, 0);

    /* Duplicate registration should succeed (idempotent) */
    int result2 = loki_lang_register(&mock_lang_ops);
    ASSERT_EQ(result2, 0);
}

/* ============================================================================
 * Lookup Tests
 * ============================================================================ */

TEST(bridge_lookup_by_file_extension) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);

    const LokiLangOps *ops = loki_lang_for_file("test.mock");
    ASSERT_NOT_NULL(ops);
    ASSERT_STR_EQ(ops->name, "mock");
}

TEST(bridge_lookup_by_alternate_extension) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);

    const LokiLangOps *ops = loki_lang_for_file("test.mck");
    ASSERT_NOT_NULL(ops);
    ASSERT_STR_EQ(ops->name, "mock");
}

TEST(bridge_lookup_unknown_extension) {
    const LokiLangOps *ops = loki_lang_for_file("test.unknown");
    ASSERT_NULL(ops);
}

TEST(bridge_lookup_no_extension) {
    const LokiLangOps *ops = loki_lang_for_file("noextension");
    ASSERT_NULL(ops);
}

TEST(bridge_lookup_null_filename) {
    const LokiLangOps *ops = loki_lang_for_file(NULL);
    ASSERT_NULL(ops);
}

TEST(bridge_lookup_by_name) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);

    const LokiLangOps *ops = loki_lang_by_name("mock");
    ASSERT_NOT_NULL(ops);
    ASSERT_STR_EQ(ops->name, "mock");
}

TEST(bridge_lookup_by_name_unknown) {
    const LokiLangOps *ops = loki_lang_by_name("nonexistent");
    ASSERT_NULL(ops);
}

TEST(bridge_lookup_by_name_null) {
    const LokiLangOps *ops = loki_lang_by_name(NULL);
    ASSERT_NULL(ops);
}

/* ============================================================================
 * List All Languages Tests
 * ============================================================================ */

TEST(bridge_list_all) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);
    loki_lang_register(&mock_lang2_ops);

    int count = 0;
    const LokiLangOps **langs = loki_lang_all(&count);

    ASSERT_NOT_NULL(langs);
    ASSERT_GT(count, 0);
}

TEST(bridge_list_all_null_count) {
    /* Should not crash with NULL count pointer */
    const LokiLangOps **langs = loki_lang_all(NULL);
    ASSERT_NOT_NULL(langs);
}

/* ============================================================================
 * Init For File Tests
 * ============================================================================ */

TEST(bridge_init_for_file) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);

    editor_ctx_t *ctx = create_test_context("test.mock");
    ASSERT_NOT_NULL(ctx);

    int result = loki_lang_init_for_file(ctx);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(g_mock_init_count, 1);
    ASSERT_TRUE(g_mock_is_initialized);

    free_test_context(ctx);
}

TEST(bridge_init_for_file_idempotent) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);

    editor_ctx_t *ctx = create_test_context("test.mock");
    ASSERT_NOT_NULL(ctx);

    /* First init */
    loki_lang_init_for_file(ctx);
    ASSERT_EQ(g_mock_init_count, 1);

    /* Second init should be idempotent (already initialized) */
    loki_lang_init_for_file(ctx);
    ASSERT_EQ(g_mock_init_count, 1);  /* Still 1, not 2 */

    free_test_context(ctx);
}

TEST(bridge_init_for_file_unknown) {
    editor_ctx_t *ctx = create_test_context("test.unknown");
    ASSERT_NOT_NULL(ctx);

    int result = loki_lang_init_for_file(ctx);
    ASSERT_EQ(result, -1);  /* No language for this file */

    free_test_context(ctx);
}

TEST(bridge_init_for_file_null_ctx) {
    int result = loki_lang_init_for_file(NULL);
    ASSERT_EQ(result, -1);
}

/* ============================================================================
 * Eval Tests
 * ============================================================================ */

TEST(bridge_eval) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);

    editor_ctx_t *ctx = create_test_context("test.mock");
    ASSERT_NOT_NULL(ctx);

    int result = loki_lang_eval(ctx, "test code");
    ASSERT_EQ(result, 0);
    ASSERT_EQ(g_mock_eval_count, 1);
    ASSERT_STR_EQ(g_mock_last_eval_code, "test code");

    /* Should have auto-initialized */
    ASSERT_EQ(g_mock_init_count, 1);

    free_test_context(ctx);
}

TEST(bridge_eval_null_ctx) {
    int result = loki_lang_eval(NULL, "test");
    ASSERT_EQ(result, -1);
}

TEST(bridge_eval_null_code) {
    editor_ctx_t *ctx = create_test_context("test.mock");
    int result = loki_lang_eval(ctx, NULL);
    ASSERT_EQ(result, -1);
    free_test_context(ctx);
}

TEST(bridge_eval_unknown_file) {
    editor_ctx_t *ctx = create_test_context("test.unknown");
    int result = loki_lang_eval(ctx, "test");
    ASSERT_EQ(result, -1);
    free_test_context(ctx);
}

/* ============================================================================
 * Stop Tests
 * ============================================================================ */

TEST(bridge_stop_all) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);

    editor_ctx_t *ctx = create_test_context("test.mock");
    ASSERT_NOT_NULL(ctx);

    /* Initialize and start playing */
    loki_lang_eval(ctx, "test");
    ASSERT_TRUE(g_mock_is_playing);

    /* Stop all - may call stop on multiple mock languages if mock2 is registered too */
    loki_lang_stop_all(ctx);
    ASSERT_GTE(g_mock_stop_count, 1);  /* At least one stop was called */
    ASSERT_FALSE(g_mock_is_playing);

    free_test_context(ctx);
}

TEST(bridge_stop_all_null_ctx) {
    /* Should not crash */
    loki_lang_stop_all(NULL);
}

TEST(bridge_stop_all_not_initialized) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);

    editor_ctx_t *ctx = create_test_context("test.mock");

    /* Stop without init should not call stop (not initialized) */
    loki_lang_stop_all(ctx);
    ASSERT_EQ(g_mock_stop_count, 0);

    free_test_context(ctx);
}

/* ============================================================================
 * Is Playing Tests
 * ============================================================================ */

TEST(bridge_is_playing) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);

    editor_ctx_t *ctx = create_test_context("test.mock");

    /* Not playing initially */
    ASSERT_FALSE(loki_lang_is_playing(ctx));

    /* Start playing */
    loki_lang_eval(ctx, "test");
    ASSERT_TRUE(loki_lang_is_playing(ctx));

    /* Stop */
    loki_lang_stop_all(ctx);
    ASSERT_FALSE(loki_lang_is_playing(ctx));

    free_test_context(ctx);
}

TEST(bridge_is_playing_null_ctx) {
    int result = loki_lang_is_playing(NULL);
    ASSERT_EQ(result, 0);
}

/* ============================================================================
 * Cleanup Tests
 * ============================================================================ */

TEST(bridge_cleanup_all) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);

    editor_ctx_t *ctx = create_test_context("test.mock");

    /* Initialize */
    loki_lang_init_for_file(ctx);
    ASSERT_EQ(g_mock_init_count, 1);

    /* Cleanup - may cleanup multiple mock languages if mock2 is registered too */
    loki_lang_cleanup_all(ctx);
    ASSERT_GTE(g_mock_cleanup_count, 1);  /* At least one cleanup was called */

    free_test_context(ctx);
}

TEST(bridge_cleanup_all_null_ctx) {
    /* Should not crash */
    loki_lang_cleanup_all(NULL);
}

/* ============================================================================
 * Get Error Tests
 * ============================================================================ */

TEST(bridge_get_error_none) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);

    editor_ctx_t *ctx = create_test_context("test.mock");

    const char *err = loki_lang_get_error(ctx);
    ASSERT_NULL(err);  /* No error set */

    free_test_context(ctx);
}

TEST(bridge_get_error_with_error) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);
    strcpy(g_mock_error, "Test error message");

    editor_ctx_t *ctx = create_test_context("test.mock");

    const char *err = loki_lang_get_error(ctx);
    ASSERT_NOT_NULL(err);
    ASSERT_STR_EQ(err, "Test error message");

    free_test_context(ctx);
}

TEST(bridge_get_error_null_ctx) {
    const char *err = loki_lang_get_error(NULL);
    ASSERT_NULL(err);
}

TEST(bridge_get_error_unknown_file) {
    editor_ctx_t *ctx = create_test_context("test.unknown");

    const char *err = loki_lang_get_error(ctx);
    ASSERT_NULL(err);

    free_test_context(ctx);
}

/* ============================================================================
 * Has Events Tests
 * ============================================================================ */

TEST(bridge_has_events_null) {
    int result = loki_lang_has_events(NULL);
    ASSERT_EQ(result, 0);
}

TEST(bridge_has_events_no_handler) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);  /* No has_events handler */

    editor_ctx_t *ctx = create_test_context("test.mock");

    int result = loki_lang_has_events(ctx);
    ASSERT_EQ(result, 0);

    free_test_context(ctx);
}

/* ============================================================================
 * Configure Backend Tests
 * ============================================================================ */

TEST(bridge_configure_backend_null_ctx) {
    int result = loki_lang_configure_backend(NULL, "/path/to/sf", NULL);
    ASSERT_EQ(result, -1);
}

TEST(bridge_configure_backend_unknown_file) {
    editor_ctx_t *ctx = create_test_context("test.unknown");

    int result = loki_lang_configure_backend(ctx, "/path/to/sf", NULL);
    ASSERT_EQ(result, -1);

    free_test_context(ctx);
}

TEST(bridge_configure_backend_no_handler) {
    reset_mock_state();
    loki_lang_register(&mock_lang_ops);  /* No configure_backend handler */

    editor_ctx_t *ctx = create_test_context("test.mock");

    int result = loki_lang_configure_backend(ctx, "/path/to/sf", NULL);
    ASSERT_EQ(result, -1);  /* No handler */

    free_test_context(ctx);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

BEGIN_TEST_SUITE("Language Bridge Tests")

    /* Registration */
    RUN_TEST(bridge_register_null);
    RUN_TEST(bridge_register_unnamed);
    RUN_TEST(bridge_register_valid);
    RUN_TEST(bridge_register_duplicate);

    /* Lookup */
    RUN_TEST(bridge_lookup_by_file_extension);
    RUN_TEST(bridge_lookup_by_alternate_extension);
    RUN_TEST(bridge_lookup_unknown_extension);
    RUN_TEST(bridge_lookup_no_extension);
    RUN_TEST(bridge_lookup_null_filename);
    RUN_TEST(bridge_lookup_by_name);
    RUN_TEST(bridge_lookup_by_name_unknown);
    RUN_TEST(bridge_lookup_by_name_null);

    /* List all */
    RUN_TEST(bridge_list_all);
    RUN_TEST(bridge_list_all_null_count);

    /* Init for file */
    RUN_TEST(bridge_init_for_file);
    RUN_TEST(bridge_init_for_file_idempotent);
    RUN_TEST(bridge_init_for_file_unknown);
    RUN_TEST(bridge_init_for_file_null_ctx);

    /* Eval */
    RUN_TEST(bridge_eval);
    RUN_TEST(bridge_eval_null_ctx);
    RUN_TEST(bridge_eval_null_code);
    RUN_TEST(bridge_eval_unknown_file);

    /* Stop */
    RUN_TEST(bridge_stop_all);
    RUN_TEST(bridge_stop_all_null_ctx);
    RUN_TEST(bridge_stop_all_not_initialized);

    /* Is playing */
    RUN_TEST(bridge_is_playing);
    RUN_TEST(bridge_is_playing_null_ctx);

    /* Cleanup */
    RUN_TEST(bridge_cleanup_all);
    RUN_TEST(bridge_cleanup_all_null_ctx);

    /* Get error */
    RUN_TEST(bridge_get_error_none);
    RUN_TEST(bridge_get_error_with_error);
    RUN_TEST(bridge_get_error_null_ctx);
    RUN_TEST(bridge_get_error_unknown_file);

    /* Has events */
    RUN_TEST(bridge_has_events_null);
    RUN_TEST(bridge_has_events_no_handler);

    /* Configure backend */
    RUN_TEST(bridge_configure_backend_null_ctx);
    RUN_TEST(bridge_configure_backend_unknown_file);
    RUN_TEST(bridge_configure_backend_no_handler);

END_TEST_SUITE()
