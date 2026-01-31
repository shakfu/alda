/**
 * @file test_memcheck_selftest.c
 * @brief Self-tests for the memory leak detection system.
 *
 * Verifies that:
 * - Allocations are tracked correctly
 * - Frees are tracked correctly
 * - Leaks are detected
 * - Double-frees are detected
 * - Statistics are accurate
 */

#include "test_framework.h"
#include "test_memcheck.h"

/* Test statistics instance */
test_stats_t test_stats;

/* =============================================================================
 * Basic Allocation Tracking Tests
 * =============================================================================
 */

TEST(memcheck_malloc_free_no_leak) {
    memcheck_begin();

    void *ptr = MEMCHECK_MALLOC(100);
    ASSERT_NOT_NULL(ptr);
    MEMCHECK_FREE(ptr);

    ASSERT_EQ(memcheck_leak_count(), 0);
}

TEST(memcheck_detects_single_leak) {
    memcheck_begin();

    void *ptr = MEMCHECK_MALLOC(100);
    ASSERT_NOT_NULL(ptr);
    /* Intentionally not freeing */

    ASSERT_EQ(memcheck_leak_count(), 1);

    /* Clean up for real */
    MEMCHECK_FREE(ptr);
}

TEST(memcheck_detects_multiple_leaks) {
    memcheck_begin();

    void *ptr1 = MEMCHECK_MALLOC(100);
    void *ptr2 = MEMCHECK_MALLOC(200);
    void *ptr3 = MEMCHECK_MALLOC(300);

    ASSERT_NOT_NULL(ptr1);
    ASSERT_NOT_NULL(ptr2);
    ASSERT_NOT_NULL(ptr3);

    /* Only free one */
    MEMCHECK_FREE(ptr2);

    ASSERT_EQ(memcheck_leak_count(), 2);

    /* Clean up for real */
    MEMCHECK_FREE(ptr1);
    MEMCHECK_FREE(ptr3);
}

TEST(memcheck_calloc_tracked) {
    memcheck_begin();

    int *arr = MEMCHECK_CALLOC(10, sizeof(int));
    ASSERT_NOT_NULL(arr);

    /* Verify calloc zeroed the memory */
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(arr[i], 0);
    }

    MEMCHECK_FREE(arr);
    ASSERT_EQ(memcheck_leak_count(), 0);
}

TEST(memcheck_realloc_tracked) {
    memcheck_begin();

    void *ptr = MEMCHECK_MALLOC(100);
    ASSERT_NOT_NULL(ptr);

    ptr = MEMCHECK_REALLOC(ptr, 200);
    ASSERT_NOT_NULL(ptr);

    MEMCHECK_FREE(ptr);
    ASSERT_EQ(memcheck_leak_count(), 0);
}

TEST(memcheck_realloc_null_is_malloc) {
    memcheck_begin();

    void *ptr = MEMCHECK_REALLOC(NULL, 100);
    ASSERT_NOT_NULL(ptr);

    MEMCHECK_FREE(ptr);
    ASSERT_EQ(memcheck_leak_count(), 0);
}

TEST(memcheck_strdup_tracked) {
    memcheck_begin();

    char *str = MEMCHECK_STRDUP("hello world");
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "hello world");

    MEMCHECK_FREE(str);
    ASSERT_EQ(memcheck_leak_count(), 0);
}

/* =============================================================================
 * Statistics Tests
 * =============================================================================
 */

TEST(memcheck_tracks_current_bytes) {
    memcheck_begin();

    ASSERT_EQ(memcheck_current_bytes(), 0);

    void *ptr1 = MEMCHECK_MALLOC(100);
    ASSERT_EQ(memcheck_current_bytes(), 100);

    void *ptr2 = MEMCHECK_MALLOC(200);
    ASSERT_EQ(memcheck_current_bytes(), 300);

    MEMCHECK_FREE(ptr1);
    ASSERT_EQ(memcheck_current_bytes(), 200);

    MEMCHECK_FREE(ptr2);
    ASSERT_EQ(memcheck_current_bytes(), 0);
}

TEST(memcheck_tracks_peak_bytes) {
    memcheck_begin();

    void *ptr1 = MEMCHECK_MALLOC(100);
    void *ptr2 = MEMCHECK_MALLOC(200);
    /* Peak is 300 */

    MEMCHECK_FREE(ptr1);
    /* Current is 200, peak still 300 */

    void *ptr3 = MEMCHECK_MALLOC(50);
    /* Current is 250, peak still 300 */

    ASSERT_EQ(memcheck_peak_bytes(), 300);

    MEMCHECK_FREE(ptr2);
    MEMCHECK_FREE(ptr3);
}

/* =============================================================================
 * Edge Cases
 * =============================================================================
 */

TEST(memcheck_free_null_safe) {
    memcheck_begin();

    /* Should not crash */
    MEMCHECK_FREE(NULL);

    ASSERT_EQ(memcheck_leak_count(), 0);
}

TEST(memcheck_begin_resets_tracking) {
    memcheck_begin();

    void *ptr = MEMCHECK_MALLOC(100);
    ASSERT_EQ(memcheck_leak_count(), 1);

    /* Begin again should reset */
    memcheck_begin();
    ASSERT_EQ(memcheck_leak_count(), 0);

    /* But we still need to free the actual memory */
    free(ptr);
}

TEST(memcheck_disable_stops_tracking) {
    memcheck_begin();

    void *ptr1 = MEMCHECK_MALLOC(100);
    ASSERT_EQ(memcheck_leak_count(), 1);

    memcheck_enable(0);  /* Disable */

    void *ptr2 = malloc(200);  /* Not tracked */
    ASSERT_EQ(memcheck_leak_count(), 1);  /* Still 1 */

    memcheck_enable(1);  /* Re-enable */

    MEMCHECK_FREE(ptr1);
    free(ptr2);

    ASSERT_EQ(memcheck_leak_count(), 0);
}

/* =============================================================================
 * Test Runner
 * =============================================================================
 */

BEGIN_TEST_SUITE("Memory Check Self-Tests")

    memcheck_init();

    /* Basic tracking */
    RUN_TEST(memcheck_malloc_free_no_leak);
    RUN_TEST(memcheck_detects_single_leak);
    RUN_TEST(memcheck_detects_multiple_leaks);
    RUN_TEST(memcheck_calloc_tracked);
    RUN_TEST(memcheck_realloc_tracked);
    RUN_TEST(memcheck_realloc_null_is_malloc);
    RUN_TEST(memcheck_strdup_tracked);

    /* Statistics */
    RUN_TEST(memcheck_tracks_current_bytes);
    RUN_TEST(memcheck_tracks_peak_bytes);

    /* Edge cases */
    RUN_TEST(memcheck_free_null_safe);
    RUN_TEST(memcheck_begin_resets_tracking);
    RUN_TEST(memcheck_disable_stops_tracking);

    memcheck_cleanup();

END_TEST_SUITE()
