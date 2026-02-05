/**
 * @file test_plugin.c
 * @brief Tests for tracker plugin registry and compilation (tracker_plugin.c).
 *
 * Tests verify:
 * - Plugin registry lifecycle
 * - Plugin registration and unregistration
 * - Plugin lookup and default handling
 * - Transform lookup
 * - Context initialization and helpers
 * - Random number generation
 * - Cell and FX chain compilation
 */

#include "test_framework.h"
#include "tracker_plugin.h"
#include "tracker_model.h"
#include <string.h>
#include <math.h>

test_stats_t test_stats;

/* ============================================================================
 * Mock Plugin Implementation
 * ============================================================================ */

static bool mock_init_called = false;
static bool mock_cleanup_called = false;
static int mock_evaluate_calls = 0;

static bool mock_plugin_init(void) {
    mock_init_called = true;
    return true;
}

static void mock_plugin_cleanup(void) {
    mock_cleanup_called = true;
}

static bool mock_plugin_init_fail(void) {
    return false;
}

static TrackerPhrase* mock_plugin_evaluate(const char* expression, TrackerContext* ctx) {
    (void)ctx;
    mock_evaluate_calls++;

    /* Create a simple phrase with one note */
    TrackerPhrase* phrase = tracker_phrase_new(4);
    if (!phrase) return NULL;

    TrackerEvent event = {
        .type = TRACKER_EVENT_NOTE_ON,
        .channel = 0,
        .data1 = 60,  /* C4 */
        .data2 = 100,
        .gate_rows = 1,
    };

    /* Parse simple note from expression if it looks like a note */
    if (expression && strlen(expression) >= 2) {
        char note = expression[0];
        if (note >= 'a' && note <= 'g') {
            event.data1 = 60 + (note - 'c');  /* Simple mapping */
        }
    }

    tracker_phrase_add_event(phrase, &event);
    return phrase;
}

static bool mock_plugin_validate(const char* expression, const char** error_msg, int* error_pos) {
    if (!expression || strlen(expression) == 0) {
        if (error_msg) *error_msg = "Empty expression";
        if (error_pos) *error_pos = 0;
        return false;
    }
    return true;
}

static bool mock_plugin_is_generator(const char* expression) {
    /* Expressions starting with "gen:" are generators */
    return expression && strncmp(expression, "gen:", 4) == 0;
}

static TrackerPhrase* mock_transform_transpose(const TrackerPhrase* input,
                                                const char* params,
                                                TrackerContext* ctx) {
    (void)ctx;
    if (!input) return NULL;

    int semitones = params ? atoi(params) : 0;

    TrackerPhrase* output = tracker_phrase_clone(input);
    if (!output) return NULL;

    for (int i = 0; i < output->count; i++) {
        if (output->events[i].type == TRACKER_EVENT_NOTE_ON ||
            output->events[i].type == TRACKER_EVENT_NOTE_OFF) {
            int new_note = output->events[i].data1 + semitones;
            if (new_note < 0) new_note = 0;
            if (new_note > 127) new_note = 127;
            output->events[i].data1 = (uint8_t)new_note;
        }
    }

    return output;
}

static const char* mock_transform_names[] = {"transpose", "velocity"};

static TrackerTransformFn mock_get_transform(const char* fx_name) {
    if (strcmp(fx_name, "transpose") == 0) {
        return mock_transform_transpose;
    }
    return NULL;
}

static const char** mock_list_transforms(int* count) {
    if (count) *count = 2;
    return mock_transform_names;
}

/* Create mock plugins */
static TrackerPlugin mock_plugin_basic = {
    .name = "Mock Basic",
    .language_id = "mock",
    .version = "1.0",
    .description = "Basic mock plugin for testing",
    .capabilities = TRACKER_CAP_EVALUATE,
    .priority = 0,
    .init = mock_plugin_init,
    .cleanup = mock_plugin_cleanup,
    .evaluate = mock_plugin_evaluate,
};

static TrackerPlugin mock_plugin_full = {
    .name = "Mock Full",
    .language_id = "mock_full",
    .version = "1.0",
    .description = "Full-featured mock plugin",
    .capabilities = TRACKER_CAP_EVALUATE | TRACKER_CAP_VALIDATION |
                    TRACKER_CAP_GENERATORS | TRACKER_CAP_TRANSFORMS,
    .priority = 10,
    .init = mock_plugin_init,
    .cleanup = mock_plugin_cleanup,
    .evaluate = mock_plugin_evaluate,
    .validate = mock_plugin_validate,
    .is_generator = mock_plugin_is_generator,
    .get_transform = mock_get_transform,
    .list_transforms = mock_list_transforms,
};

static TrackerPlugin mock_plugin_fail_init = {
    .name = "Mock Fail",
    .language_id = "mock_fail",
    .version = "1.0",
    .capabilities = TRACKER_CAP_EVALUATE,
    .init = mock_plugin_init_fail,
};

static TrackerPlugin mock_plugin_no_id = {
    .name = "Mock No ID",
    .language_id = NULL,
    .capabilities = TRACKER_CAP_EVALUATE,
};

static void reset_mock_state(void) {
    mock_init_called = false;
    mock_cleanup_called = false;
    mock_evaluate_calls = 0;
}

/* ============================================================================
 * Registry Lifecycle Tests
 * ============================================================================ */

TEST(registry_init) {
    tracker_plugin_registry_cleanup();  /* Ensure clean state */

    bool result = tracker_plugin_registry_init();
    ASSERT_TRUE(result);

    /* Second init should also succeed (idempotent) */
    result = tracker_plugin_registry_init();
    ASSERT_TRUE(result);

    tracker_plugin_registry_cleanup();
}

TEST(registry_cleanup_safe) {
    /* Multiple cleanups should be safe */
    tracker_plugin_registry_cleanup();
    tracker_plugin_registry_cleanup();
    /* Should not crash */
}

/* ============================================================================
 * Plugin Registration Tests
 * ============================================================================ */

TEST(register_plugin_basic) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    bool result = tracker_plugin_register(&mock_plugin_basic);
    ASSERT_TRUE(result);
    ASSERT_TRUE(mock_init_called);

    TrackerPlugin* found = tracker_plugin_find("mock");
    ASSERT_NOT_NULL(found);
    ASSERT_TRUE(found == &mock_plugin_basic);

    tracker_plugin_registry_cleanup();
    ASSERT_TRUE(mock_cleanup_called);
}

TEST(register_null_plugin) {
    tracker_plugin_registry_cleanup();

    bool result = tracker_plugin_register(NULL);
    ASSERT_FALSE(result);

    tracker_plugin_registry_cleanup();
}

TEST(register_plugin_no_language_id) {
    tracker_plugin_registry_cleanup();

    bool result = tracker_plugin_register(&mock_plugin_no_id);
    ASSERT_FALSE(result);

    tracker_plugin_registry_cleanup();
}

TEST(register_duplicate_plugin) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    bool result = tracker_plugin_register(&mock_plugin_basic);
    ASSERT_TRUE(result);

    /* Try to register same language_id again */
    result = tracker_plugin_register(&mock_plugin_basic);
    ASSERT_FALSE(result);

    tracker_plugin_registry_cleanup();
}

TEST(register_plugin_init_fails) {
    tracker_plugin_registry_cleanup();

    bool result = tracker_plugin_register(&mock_plugin_fail_init);
    ASSERT_FALSE(result);

    /* Plugin should not be in registry */
    TrackerPlugin* found = tracker_plugin_find("mock_fail");
    ASSERT_NULL(found);

    tracker_plugin_registry_cleanup();
}

TEST(register_multiple_plugins) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    bool result1 = tracker_plugin_register(&mock_plugin_basic);
    bool result2 = tracker_plugin_register(&mock_plugin_full);

    ASSERT_TRUE(result1);
    ASSERT_TRUE(result2);

    TrackerPlugin* found1 = tracker_plugin_find("mock");
    TrackerPlugin* found2 = tracker_plugin_find("mock_full");

    ASSERT_NOT_NULL(found1);
    ASSERT_NOT_NULL(found2);
    ASSERT_TRUE(found1 != found2);

    tracker_plugin_registry_cleanup();
}

/* ============================================================================
 * Plugin Unregistration Tests
 * ============================================================================ */

TEST(unregister_plugin) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_basic);
    mock_cleanup_called = false;

    bool result = tracker_plugin_unregister("mock");
    ASSERT_TRUE(result);
    ASSERT_TRUE(mock_cleanup_called);

    TrackerPlugin* found = tracker_plugin_find("mock");
    ASSERT_NULL(found);

    tracker_plugin_registry_cleanup();
}

TEST(unregister_nonexistent_plugin) {
    tracker_plugin_registry_cleanup();

    bool result = tracker_plugin_unregister("nonexistent");
    ASSERT_FALSE(result);

    tracker_plugin_registry_cleanup();
}

TEST(unregister_null_language_id) {
    tracker_plugin_registry_cleanup();

    bool result = tracker_plugin_unregister(NULL);
    ASSERT_FALSE(result);

    tracker_plugin_registry_cleanup();
}

TEST(unregister_updates_default) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_basic);
    tracker_plugin_register(&mock_plugin_full);

    /* First registered becomes default */
    TrackerPlugin* def = tracker_plugin_get_default();
    ASSERT_TRUE(def == &mock_plugin_basic);

    /* Unregister default - should update to next */
    tracker_plugin_unregister("mock");

    def = tracker_plugin_get_default();
    ASSERT_TRUE(def == &mock_plugin_full);

    tracker_plugin_registry_cleanup();
}

/* ============================================================================
 * Plugin Lookup Tests
 * ============================================================================ */

TEST(find_plugin_by_id) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_basic);
    tracker_plugin_register(&mock_plugin_full);

    TrackerPlugin* found = tracker_plugin_find("mock");
    ASSERT_TRUE(found == &mock_plugin_basic);

    found = tracker_plugin_find("mock_full");
    ASSERT_TRUE(found == &mock_plugin_full);

    tracker_plugin_registry_cleanup();
}

TEST(find_plugin_null_returns_default) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_basic);

    TrackerPlugin* found = tracker_plugin_find(NULL);
    ASSERT_NOT_NULL(found);
    ASSERT_TRUE(found == &mock_plugin_basic);

    tracker_plugin_registry_cleanup();
}

TEST(find_plugin_not_found) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_basic);

    TrackerPlugin* found = tracker_plugin_find("nonexistent");
    ASSERT_NULL(found);

    tracker_plugin_registry_cleanup();
}

TEST(find_plugin_uninitialized_registry) {
    tracker_plugin_registry_cleanup();

    TrackerPlugin* found = tracker_plugin_find("anything");
    ASSERT_NULL(found);
}

/* ============================================================================
 * Default Plugin Tests
 * ============================================================================ */

TEST(get_default_plugin) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_basic);

    TrackerPlugin* def = tracker_plugin_get_default();
    ASSERT_NOT_NULL(def);
    ASSERT_TRUE(def == &mock_plugin_basic);

    tracker_plugin_registry_cleanup();
}

TEST(get_default_uninitialized) {
    tracker_plugin_registry_cleanup();

    TrackerPlugin* def = tracker_plugin_get_default();
    ASSERT_NULL(def);
}

TEST(set_default_plugin) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_basic);
    tracker_plugin_register(&mock_plugin_full);

    /* Initially mock_plugin_basic is default */
    TrackerPlugin* def = tracker_plugin_get_default();
    ASSERT_TRUE(def == &mock_plugin_basic);

    /* Change default */
    bool result = tracker_plugin_set_default("mock_full");
    ASSERT_TRUE(result);

    def = tracker_plugin_get_default();
    ASSERT_TRUE(def == &mock_plugin_full);

    tracker_plugin_registry_cleanup();
}

TEST(set_default_nonexistent) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_basic);

    bool result = tracker_plugin_set_default("nonexistent");
    ASSERT_FALSE(result);

    /* Default should remain unchanged */
    TrackerPlugin* def = tracker_plugin_get_default();
    ASSERT_TRUE(def == &mock_plugin_basic);

    tracker_plugin_registry_cleanup();
}

/* ============================================================================
 * Plugin List Tests
 * ============================================================================ */

TEST(list_all_plugins) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_basic);
    tracker_plugin_register(&mock_plugin_full);

    int count = 0;
    TrackerPlugin** list = tracker_plugin_list_all(&count);

    ASSERT_NOT_NULL(list);
    ASSERT_EQ(count, 2);

    tracker_plugin_registry_cleanup();
}

TEST(list_all_plugins_empty) {
    tracker_plugin_registry_cleanup();

    int count = -1;
    TrackerPlugin** list = tracker_plugin_list_all(&count);

    ASSERT_NULL(list);
    ASSERT_EQ(count, 0);
}

TEST(list_all_plugins_null_count) {
    tracker_plugin_registry_cleanup();

    TrackerPlugin** list = tracker_plugin_list_all(NULL);
    ASSERT_NULL(list);
}

/* ============================================================================
 * Transform Lookup Tests
 * ============================================================================ */

TEST(find_transform) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_full);

    TrackerPlugin* plugin = NULL;
    TrackerTransformFn fn = tracker_plugin_find_transform("transpose", &plugin);

    ASSERT_NOT_NULL(fn);
    ASSERT_TRUE(plugin == &mock_plugin_full);

    tracker_plugin_registry_cleanup();
}

TEST(find_transform_not_found) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_full);

    TrackerPlugin* plugin = NULL;
    TrackerTransformFn fn = tracker_plugin_find_transform("nonexistent", &plugin);

    ASSERT_NULL(fn);

    tracker_plugin_registry_cleanup();
}

TEST(find_transform_null_name) {
    tracker_plugin_registry_cleanup();

    TrackerTransformFn fn = tracker_plugin_find_transform(NULL, NULL);
    ASSERT_NULL(fn);
}

TEST(find_transform_uninitialized) {
    tracker_plugin_registry_cleanup();

    TrackerTransformFn fn = tracker_plugin_find_transform("transpose", NULL);
    ASSERT_NULL(fn);
}

TEST(list_all_transforms) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    tracker_plugin_register(&mock_plugin_full);

    int count = 0;
    TrackerTransformInfo* list = tracker_plugin_list_all_transforms(&count);

    ASSERT_NOT_NULL(list);
    ASSERT_EQ(count, 2);  /* transpose and velocity */

    free(list);
    tracker_plugin_registry_cleanup();
}

TEST(list_all_transforms_empty) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();

    /* Register plugin without transforms */
    tracker_plugin_register(&mock_plugin_basic);

    int count = -1;
    TrackerTransformInfo* list = tracker_plugin_list_all_transforms(&count);

    ASSERT_NULL(list);
    ASSERT_EQ(count, 0);

    tracker_plugin_registry_cleanup();
}

/* ============================================================================
 * Plugin Capabilities Tests
 * ============================================================================ */

TEST(has_capability) {
    ASSERT_TRUE(tracker_plugin_has_cap(&mock_plugin_basic, TRACKER_CAP_EVALUATE));
    ASSERT_FALSE(tracker_plugin_has_cap(&mock_plugin_basic, TRACKER_CAP_TRANSFORMS));

    ASSERT_TRUE(tracker_plugin_has_cap(&mock_plugin_full, TRACKER_CAP_EVALUATE));
    ASSERT_TRUE(tracker_plugin_has_cap(&mock_plugin_full, TRACKER_CAP_TRANSFORMS));
    ASSERT_TRUE(tracker_plugin_has_cap(&mock_plugin_full, TRACKER_CAP_VALIDATION));
    ASSERT_TRUE(tracker_plugin_has_cap(&mock_plugin_full, TRACKER_CAP_GENERATORS));
}

/* ============================================================================
 * Context Tests
 * ============================================================================ */

TEST(context_init) {
    TrackerContext ctx;
    tracker_context_init(&ctx);

    ASSERT_EQ(ctx.bpm, TRACKER_DEFAULT_BPM);
    ASSERT_EQ(ctx.rows_per_beat, TRACKER_DEFAULT_RPB);
    ASSERT_EQ(ctx.ticks_per_row, TRACKER_DEFAULT_TPR);
    ASSERT_EQ(ctx.spillover_mode, TRACKER_SPILLOVER_LAYER);
    ASSERT_EQ(ctx.current_row, 0);
    ASSERT_EQ(ctx.current_track, 0);
    ASSERT_NULL(ctx.song);
}

TEST(context_init_null_safe) {
    tracker_context_init(NULL);
    /* Should not crash */
}

TEST(context_from_song) {
    TrackerSong* song = tracker_song_new("Test Song");
    ASSERT_NOT_NULL(song);

    song->bpm = 140;
    song->rows_per_beat = 8;

    TrackerPattern* pattern = tracker_pattern_new(32, 4, "Test");
    ASSERT_NOT_NULL(pattern);
    tracker_song_add_pattern(song, pattern);

    TrackerContext ctx;
    tracker_context_from_song(&ctx, song, 0, 8, 2);

    ASSERT_EQ(ctx.bpm, 140);
    ASSERT_EQ(ctx.rows_per_beat, 8);
    ASSERT_EQ(ctx.current_pattern, 0);
    ASSERT_EQ(ctx.current_row, 8);
    ASSERT_EQ(ctx.current_track, 2);
    ASSERT_EQ(ctx.total_rows, 32);
    ASSERT_EQ(ctx.total_tracks, 4);
    ASSERT_TRUE(ctx.song == song);
    ASSERT_NOT_NULL(ctx.lookup_phrase);

    tracker_song_free(song);
}

TEST(context_from_song_null_ctx) {
    TrackerSong* song = tracker_song_new("Test");
    tracker_context_from_song(NULL, song, 0, 0, 0);
    /* Should not crash */
    tracker_song_free(song);
}

TEST(context_from_song_null_song) {
    TrackerContext ctx;
    tracker_context_from_song(&ctx, NULL, 0, 0, 0);

    /* Should have defaults */
    ASSERT_EQ(ctx.bpm, TRACKER_DEFAULT_BPM);
    ASSERT_NULL(ctx.song);
}

/* ============================================================================
 * Random Number Tests
 * ============================================================================ */

TEST(context_random) {
    TrackerContext ctx;
    tracker_context_init(&ctx);
    tracker_context_reseed(&ctx, 12345);

    uint32_t r1 = tracker_context_random(&ctx, 100);
    uint32_t r2 = tracker_context_random(&ctx, 100);

    /* Should be in range */
    ASSERT_TRUE(r1 < 100);
    ASSERT_TRUE(r2 < 100);

    /* Should be different (with high probability) */
    ASSERT_TRUE(r1 != r2);
}

TEST(context_random_deterministic) {
    TrackerContext ctx1, ctx2;
    tracker_context_init(&ctx1);
    tracker_context_init(&ctx2);

    tracker_context_reseed(&ctx1, 12345);
    tracker_context_reseed(&ctx2, 12345);

    /* Same seed should produce same sequence */
    for (int i = 0; i < 10; i++) {
        uint32_t r1 = tracker_context_random(&ctx1, 1000);
        uint32_t r2 = tracker_context_random(&ctx2, 1000);
        ASSERT_EQ(r1, r2);
    }
}

TEST(context_random_null_safe) {
    uint32_t r = tracker_context_random(NULL, 100);
    ASSERT_EQ(r, 0);
}

TEST(context_random_zero_max) {
    TrackerContext ctx;
    tracker_context_init(&ctx);

    uint32_t r = tracker_context_random(&ctx, 0);
    ASSERT_EQ(r, 0);
}

TEST(context_random_float) {
    TrackerContext ctx;
    tracker_context_init(&ctx);
    tracker_context_reseed(&ctx, 12345);

    float f1 = tracker_context_random_float(&ctx);
    float f2 = tracker_context_random_float(&ctx);

    /* Should be in range [0, 1) */
    ASSERT_TRUE(f1 >= 0.0f && f1 < 1.0f);
    ASSERT_TRUE(f2 >= 0.0f && f2 < 1.0f);
}

TEST(context_random_float_null_safe) {
    float f = tracker_context_random_float(NULL);
    ASSERT_TRUE(fabs(f) < 0.001f);
}

TEST(context_reseed) {
    TrackerContext ctx;
    tracker_context_init(&ctx);

    tracker_context_reseed(&ctx, 99999);
    ASSERT_EQ(ctx.random_seed, 99999);
    ASSERT_EQ(ctx.random_state, 99999);

    /* Reseed with 0 should use 1 */
    tracker_context_reseed(&ctx, 0);
    ASSERT_EQ(ctx.random_state, 1);
}

TEST(context_reseed_null_safe) {
    tracker_context_reseed(NULL, 12345);
    /* Should not crash */
}

/* ============================================================================
 * Compilation Tests
 * ============================================================================ */

TEST(compile_cell_null) {
    const char* error = NULL;
    CompiledCell* compiled = tracker_compile_cell(NULL, NULL, &error);

    ASSERT_NULL(compiled);
    ASSERT_NOT_NULL(error);
}

TEST(compile_cell_empty) {
    TrackerCell cell;
    tracker_cell_init(&cell);
    cell.type = TRACKER_CELL_EMPTY;

    const char* error = NULL;
    CompiledCell* compiled = tracker_compile_cell(&cell, NULL, &error);

    ASSERT_NULL(compiled);  /* Not an error, just nothing to compile */
    ASSERT_NULL(error);

    tracker_cell_clear(&cell);
}

TEST(compile_cell_note_off) {
    TrackerCell cell;
    tracker_cell_init(&cell);
    cell.type = TRACKER_CELL_NOTE_OFF;

    const char* error = NULL;
    CompiledCell* compiled = tracker_compile_cell(&cell, NULL, &error);

    ASSERT_NOT_NULL(compiled);
    ASSERT_FALSE(compiled->is_generator);
    ASSERT_NULL(error);

    tracker_compiled_cell_free(compiled);
    tracker_cell_clear(&cell);
}

TEST(compile_cell_unknown_language) {
    TrackerCell cell;
    tracker_cell_init(&cell);
    tracker_cell_set_expression(&cell, "c4", "nonexistent_lang");

    tracker_plugin_registry_cleanup();

    const char* error = NULL;
    CompiledCell* compiled = tracker_compile_cell(&cell, NULL, &error);

    ASSERT_NULL(compiled);
    ASSERT_NOT_NULL(error);

    tracker_cell_clear(&cell);
}

TEST(compile_cell_with_plugin) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();
    tracker_plugin_register(&mock_plugin_full);

    TrackerCell cell;
    tracker_cell_init(&cell);
    tracker_cell_set_expression(&cell, "c4", "mock_full");

    const char* error = NULL;
    CompiledCell* compiled = tracker_compile_cell(&cell, NULL, &error);

    ASSERT_NOT_NULL(compiled);
    ASSERT_NULL(error);
    ASSERT_TRUE(compiled->plugin == &mock_plugin_full);
    ASSERT_FALSE(compiled->is_generator);

    tracker_compiled_cell_free(compiled);
    tracker_cell_clear(&cell);
    tracker_plugin_registry_cleanup();
}

TEST(compile_cell_generator) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();
    tracker_plugin_register(&mock_plugin_full);

    TrackerCell cell;
    tracker_cell_init(&cell);
    tracker_cell_set_expression(&cell, "gen:random", "mock_full");

    const char* error = NULL;
    CompiledCell* compiled = tracker_compile_cell(&cell, NULL, &error);

    ASSERT_NOT_NULL(compiled);
    ASSERT_TRUE(compiled->is_generator);

    tracker_compiled_cell_free(compiled);
    tracker_cell_clear(&cell);
    tracker_plugin_registry_cleanup();
}

TEST(compiled_cell_free_null_safe) {
    tracker_compiled_cell_free(NULL);
    /* Should not crash */
}

/* ============================================================================
 * FX Chain Compilation Tests
 * ============================================================================ */

TEST(compile_fx_chain_null) {
    CompiledFxChain* compiled = tracker_compile_fx_chain(NULL, NULL, NULL);
    ASSERT_NULL(compiled);
}

TEST(compile_fx_chain_empty) {
    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);

    CompiledFxChain* compiled = tracker_compile_fx_chain(&chain, NULL, NULL);
    ASSERT_NULL(compiled);  /* Not an error, just nothing to compile */

    tracker_fx_chain_clear(&chain);
}

TEST(compile_fx_chain_with_transform) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();
    tracker_plugin_register(&mock_plugin_full);

    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);
    tracker_fx_chain_append(&chain, "transpose", "12", NULL);

    const char* error = NULL;
    CompiledFxChain* compiled = tracker_compile_fx_chain(&chain, "mock_full", &error);

    ASSERT_NOT_NULL(compiled);
    ASSERT_NULL(error);
    ASSERT_EQ(compiled->count, 1);
    ASSERT_NOT_NULL(compiled->entries[0].fn);

    tracker_compiled_fx_chain_free(compiled);
    free(compiled);
    tracker_fx_chain_clear(&chain);
    tracker_plugin_registry_cleanup();
}

TEST(compile_fx_chain_unknown_transform) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();
    tracker_plugin_register(&mock_plugin_full);

    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);
    tracker_fx_chain_append(&chain, "nonexistent_fx", NULL, NULL);

    const char* error = NULL;
    CompiledFxChain* compiled = tracker_compile_fx_chain(&chain, "mock_full", &error);

    ASSERT_NULL(compiled);
    ASSERT_NOT_NULL(error);

    tracker_fx_chain_clear(&chain);
    tracker_plugin_registry_cleanup();
}

TEST(compiled_fx_chain_free_null_safe) {
    tracker_compiled_fx_chain_free(NULL);
    /* Should not crash */
}

/* ============================================================================
 * Invalidation Tests
 * ============================================================================ */

TEST(invalidate_pattern_null_safe) {
    tracker_invalidate_pattern(NULL);
    /* Should not crash */
}

TEST(invalidate_pattern) {
    TrackerPattern* pattern = tracker_pattern_new(16, 2, "Test");
    ASSERT_NOT_NULL(pattern);

    /* Set some cells as not dirty */
    TrackerCell* cell = tracker_pattern_get_cell(pattern, 0, 0);
    if (cell) {
        cell->dirty = false;
    }

    tracker_invalidate_pattern(pattern);

    /* All cells should be dirty now */
    cell = tracker_pattern_get_cell(pattern, 0, 0);
    ASSERT_TRUE(cell->dirty);

    tracker_pattern_free(pattern);
}

TEST(invalidate_song_null_safe) {
    tracker_invalidate_song(NULL);
    /* Should not crash */
}

TEST(invalidate_song) {
    TrackerSong* song = tracker_song_new("Test");
    ASSERT_NOT_NULL(song);

    TrackerPattern* pattern = tracker_pattern_new(16, 2, "Test");
    tracker_song_add_pattern(song, pattern);

    TrackerCell* cell = tracker_pattern_get_cell(pattern, 0, 0);
    if (cell) {
        cell->dirty = false;
    }

    tracker_invalidate_song(song);

    cell = tracker_pattern_get_cell(pattern, 0, 0);
    ASSERT_TRUE(cell->dirty);

    tracker_song_free(song);
}

/* ============================================================================
 * Evaluation Tests
 * ============================================================================ */

TEST(evaluate_cell_null) {
    TrackerContext ctx;
    tracker_context_init(&ctx);

    TrackerPhrase* phrase = tracker_evaluate_cell(NULL, &ctx);
    ASSERT_NULL(phrase);
}

TEST(evaluate_cell_null_ctx) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();
    tracker_plugin_register(&mock_plugin_full);

    TrackerCell cell;
    tracker_cell_init(&cell);
    tracker_cell_set_expression(&cell, "c4", "mock_full");

    CompiledCell* compiled = tracker_compile_cell(&cell, NULL, NULL);
    ASSERT_NOT_NULL(compiled);

    TrackerPhrase* phrase = tracker_evaluate_cell(compiled, NULL);
    ASSERT_NULL(phrase);

    tracker_compiled_cell_free(compiled);
    tracker_cell_clear(&cell);
    tracker_plugin_registry_cleanup();
}

TEST(evaluate_cell_basic) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();
    tracker_plugin_register(&mock_plugin_full);

    TrackerCell cell;
    tracker_cell_init(&cell);
    tracker_cell_set_expression(&cell, "c4", "mock_full");

    CompiledCell* compiled = tracker_compile_cell(&cell, NULL, NULL);
    ASSERT_NOT_NULL(compiled);

    TrackerContext ctx;
    tracker_context_init(&ctx);

    TrackerPhrase* phrase = tracker_evaluate_cell(compiled, &ctx);
    ASSERT_NOT_NULL(phrase);
    ASSERT_GT(phrase->count, 0);

    tracker_phrase_free(phrase);
    tracker_compiled_cell_free(compiled);
    tracker_cell_clear(&cell);
    tracker_plugin_registry_cleanup();
}

/* ============================================================================
 * Apply FX Chain Tests
 * ============================================================================ */

TEST(apply_fx_chain_null_phrase) {
    TrackerContext ctx;
    tracker_context_init(&ctx);

    TrackerPhrase* result = tracker_apply_fx_chain(NULL, NULL, &ctx);
    ASSERT_NULL(result);
}

TEST(apply_fx_chain_empty_chain) {
    TrackerPhrase* phrase = tracker_phrase_new(4);
    TrackerEvent event = {.type = TRACKER_EVENT_NOTE_ON, .data1 = 60};
    tracker_phrase_add_event(phrase, &event);

    TrackerContext ctx;
    tracker_context_init(&ctx);

    TrackerPhrase* result = tracker_apply_fx_chain(NULL, phrase, &ctx);
    ASSERT_TRUE(result == phrase);

    tracker_phrase_free(phrase);
}

TEST(apply_fx_chain_transform) {
    tracker_plugin_registry_cleanup();
    reset_mock_state();
    tracker_plugin_register(&mock_plugin_full);

    TrackerFxChain chain;
    tracker_fx_chain_init(&chain);
    tracker_fx_chain_append(&chain, "transpose", "12", NULL);

    CompiledFxChain* compiled = tracker_compile_fx_chain(&chain, "mock_full", NULL);
    ASSERT_NOT_NULL(compiled);

    TrackerPhrase* phrase = tracker_phrase_new(4);
    TrackerEvent event = {.type = TRACKER_EVENT_NOTE_ON, .data1 = 60, .data2 = 100};
    tracker_phrase_add_event(phrase, &event);

    TrackerContext ctx;
    tracker_context_init(&ctx);

    TrackerPhrase* result = tracker_apply_fx_chain(compiled, phrase, &ctx);
    ASSERT_NOT_NULL(result);
    ASSERT_EQ(result->count, 1);
    ASSERT_EQ(result->events[0].data1, 72);  /* 60 + 12 */

    tracker_phrase_free(result);
    tracker_compiled_fx_chain_free(compiled);
    free(compiled);
    tracker_fx_chain_clear(&chain);
    tracker_plugin_registry_cleanup();
}

/* ============================================================================
 * Test Suite
 * ============================================================================ */

BEGIN_TEST_SUITE("Tracker Plugin Tests")

    /* Registry lifecycle */
    RUN_TEST(registry_init);
    RUN_TEST(registry_cleanup_safe);

    /* Registration */
    RUN_TEST(register_plugin_basic);
    RUN_TEST(register_null_plugin);
    RUN_TEST(register_plugin_no_language_id);
    RUN_TEST(register_duplicate_plugin);
    RUN_TEST(register_plugin_init_fails);
    RUN_TEST(register_multiple_plugins);

    /* Unregistration */
    RUN_TEST(unregister_plugin);
    RUN_TEST(unregister_nonexistent_plugin);
    RUN_TEST(unregister_null_language_id);
    RUN_TEST(unregister_updates_default);

    /* Lookup */
    RUN_TEST(find_plugin_by_id);
    RUN_TEST(find_plugin_null_returns_default);
    RUN_TEST(find_plugin_not_found);
    RUN_TEST(find_plugin_uninitialized_registry);

    /* Default plugin */
    RUN_TEST(get_default_plugin);
    RUN_TEST(get_default_uninitialized);
    RUN_TEST(set_default_plugin);
    RUN_TEST(set_default_nonexistent);

    /* List */
    RUN_TEST(list_all_plugins);
    RUN_TEST(list_all_plugins_empty);
    RUN_TEST(list_all_plugins_null_count);

    /* Transforms */
    RUN_TEST(find_transform);
    RUN_TEST(find_transform_not_found);
    RUN_TEST(find_transform_null_name);
    RUN_TEST(find_transform_uninitialized);
    RUN_TEST(list_all_transforms);
    RUN_TEST(list_all_transforms_empty);

    /* Capabilities */
    RUN_TEST(has_capability);

    /* Context */
    RUN_TEST(context_init);
    RUN_TEST(context_init_null_safe);
    RUN_TEST(context_from_song);
    RUN_TEST(context_from_song_null_ctx);
    RUN_TEST(context_from_song_null_song);

    /* Random */
    RUN_TEST(context_random);
    RUN_TEST(context_random_deterministic);
    RUN_TEST(context_random_null_safe);
    RUN_TEST(context_random_zero_max);
    RUN_TEST(context_random_float);
    RUN_TEST(context_random_float_null_safe);
    RUN_TEST(context_reseed);
    RUN_TEST(context_reseed_null_safe);

    /* Compilation */
    RUN_TEST(compile_cell_null);
    RUN_TEST(compile_cell_empty);
    RUN_TEST(compile_cell_note_off);
    RUN_TEST(compile_cell_unknown_language);
    RUN_TEST(compile_cell_with_plugin);
    RUN_TEST(compile_cell_generator);
    RUN_TEST(compiled_cell_free_null_safe);

    /* FX chain compilation */
    RUN_TEST(compile_fx_chain_null);
    RUN_TEST(compile_fx_chain_empty);
    RUN_TEST(compile_fx_chain_with_transform);
    RUN_TEST(compile_fx_chain_unknown_transform);
    RUN_TEST(compiled_fx_chain_free_null_safe);

    /* Invalidation */
    RUN_TEST(invalidate_pattern_null_safe);
    RUN_TEST(invalidate_pattern);
    RUN_TEST(invalidate_song_null_safe);
    RUN_TEST(invalidate_song);

    /* Evaluation */
    RUN_TEST(evaluate_cell_null);
    RUN_TEST(evaluate_cell_null_ctx);
    RUN_TEST(evaluate_cell_basic);

    /* Apply FX chain */
    RUN_TEST(apply_fx_chain_null_phrase);
    RUN_TEST(apply_fx_chain_empty_chain);
    RUN_TEST(apply_fx_chain_transform);

END_TEST_SUITE()
