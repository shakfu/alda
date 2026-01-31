/* test_memcheck.h - Memory leak detection for tests
 *
 * Provides allocation tracking to detect memory leaks during test execution.
 *
 * Usage:
 *   // At start of test suite
 *   memcheck_init();
 *
 *   // In test code, use tracked allocations
 *   void *ptr = MEMCHECK_MALLOC(size);
 *   ptr = MEMCHECK_REALLOC(ptr, new_size);
 *   MEMCHECK_FREE(ptr);
 *
 *   // Or wrap existing code with begin/end
 *   memcheck_begin();
 *   run_code_that_allocates();
 *   int leaks = memcheck_end();  // Returns leak count
 *
 *   // At end of test suite
 *   memcheck_report();  // Prints any remaining leaks
 *   memcheck_cleanup();
 *
 * Configuration:
 *   Define TEST_MEMCHECK_ENABLED=1 to enable tracking (default: enabled)
 *   Define TEST_MEMCHECK_MAX_ALLOCS for max tracked allocations (default: 4096)
 */

#ifndef TEST_MEMCHECK_H
#define TEST_MEMCHECK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Configuration */
#ifndef TEST_MEMCHECK_ENABLED
#define TEST_MEMCHECK_ENABLED 1
#endif

#ifndef TEST_MEMCHECK_MAX_ALLOCS
#define TEST_MEMCHECK_MAX_ALLOCS 4096
#endif

/* =============================================================================
 * Allocation Tracking Structure
 * =============================================================================
 */

typedef struct {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    int active;  /* 1 if allocated, 0 if freed */
} memcheck_alloc_t;

typedef struct {
    memcheck_alloc_t allocs[TEST_MEMCHECK_MAX_ALLOCS];
    int count;
    int enabled;
    int total_allocs;
    int total_frees;
    size_t total_bytes;
    size_t peak_bytes;
    size_t current_bytes;
} memcheck_state_t;

/* Global state - defined as static in header for simplicity */
static memcheck_state_t g_memcheck = {0};

/* =============================================================================
 * Core Functions
 * =============================================================================
 */

/**
 * Initialize the memory checker.
 * Call once at the start of test suite.
 */
static inline void memcheck_init(void) {
    memset(&g_memcheck, 0, sizeof(g_memcheck));
    g_memcheck.enabled = TEST_MEMCHECK_ENABLED;
}

/**
 * Enable/disable memory tracking.
 */
static inline void memcheck_enable(int enabled) {
    g_memcheck.enabled = enabled;
}

/**
 * Begin a tracking session (resets leak detection).
 */
static inline void memcheck_begin(void) {
    /* Mark all existing allocations as inactive for this session */
    for (int i = 0; i < g_memcheck.count; i++) {
        g_memcheck.allocs[i].active = 0;
    }
    g_memcheck.count = 0;
    g_memcheck.current_bytes = 0;
    g_memcheck.peak_bytes = 0;
}

/**
 * Track an allocation.
 */
static inline void *memcheck_track_alloc(void *ptr, size_t size,
                                          const char *file, int line) {
    if (!g_memcheck.enabled || !ptr) {
        return ptr;
    }

    if (g_memcheck.count >= TEST_MEMCHECK_MAX_ALLOCS) {
        fprintf(stderr, "memcheck: allocation table full\n");
        return ptr;
    }

    memcheck_alloc_t *alloc = &g_memcheck.allocs[g_memcheck.count++];
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->file = file;
    alloc->line = line;
    alloc->active = 1;

    g_memcheck.total_allocs++;
    g_memcheck.total_bytes += size;
    g_memcheck.current_bytes += size;

    if (g_memcheck.current_bytes > g_memcheck.peak_bytes) {
        g_memcheck.peak_bytes = g_memcheck.current_bytes;
    }

    return ptr;
}

/**
 * Track a free.
 */
static inline void memcheck_track_free(void *ptr, const char *file, int line) {
    if (!g_memcheck.enabled || !ptr) {
        return;
    }

    /* Find the allocation */
    for (int i = g_memcheck.count - 1; i >= 0; i--) {
        if (g_memcheck.allocs[i].ptr == ptr && g_memcheck.allocs[i].active) {
            g_memcheck.allocs[i].active = 0;
            g_memcheck.current_bytes -= g_memcheck.allocs[i].size;
            g_memcheck.total_frees++;
            return;
        }
    }

    /* Double-free or freeing untracked pointer */
    fprintf(stderr, "memcheck: free of untracked pointer %p at %s:%d\n",
            ptr, file, line);
}

/**
 * Track a realloc.
 */
static inline void *memcheck_track_realloc(void *old_ptr, void *new_ptr,
                                            size_t new_size,
                                            const char *file, int line) {
    if (!g_memcheck.enabled) {
        return new_ptr;
    }

    if (!old_ptr) {
        /* realloc(NULL, size) is equivalent to malloc(size) */
        return memcheck_track_alloc(new_ptr, new_size, file, line);
    }

    if (!new_ptr) {
        /* realloc failed, old pointer still valid */
        return NULL;
    }

    /* Find and update the old allocation */
    for (int i = g_memcheck.count - 1; i >= 0; i--) {
        if (g_memcheck.allocs[i].ptr == old_ptr && g_memcheck.allocs[i].active) {
            size_t old_size = g_memcheck.allocs[i].size;
            g_memcheck.allocs[i].ptr = new_ptr;
            g_memcheck.allocs[i].size = new_size;
            g_memcheck.allocs[i].file = file;
            g_memcheck.allocs[i].line = line;

            g_memcheck.current_bytes = g_memcheck.current_bytes - old_size + new_size;
            g_memcheck.total_bytes += new_size;

            if (g_memcheck.current_bytes > g_memcheck.peak_bytes) {
                g_memcheck.peak_bytes = g_memcheck.current_bytes;
            }

            return new_ptr;
        }
    }

    /* Old pointer wasn't tracked - track the new one */
    return memcheck_track_alloc(new_ptr, new_size, file, line);
}

/**
 * End a tracking session and return the number of leaks.
 * Note: Returns leak count even if tracking is currently disabled.
 */
static inline int memcheck_end(void) {
    int leaks = 0;
    for (int i = 0; i < g_memcheck.count; i++) {
        if (g_memcheck.allocs[i].active) {
            leaks++;
        }
    }
    return leaks;
}

/**
 * Get current leak count without ending session.
 */
static inline int memcheck_leak_count(void) {
    return memcheck_end();
}

/**
 * Get total bytes currently allocated.
 */
static inline size_t memcheck_current_bytes(void) {
    return g_memcheck.current_bytes;
}

/**
 * Get peak bytes allocated.
 */
static inline size_t memcheck_peak_bytes(void) {
    return g_memcheck.peak_bytes;
}

/**
 * Print a report of any memory leaks.
 */
static inline void memcheck_report(void) {
    if (!g_memcheck.enabled) {
        return;
    }

    int leaks = 0;
    size_t leaked_bytes = 0;

    for (int i = 0; i < g_memcheck.count; i++) {
        if (g_memcheck.allocs[i].active) {
            if (leaks == 0) {
                fprintf(stderr, "\n\x1b[31mMemory leaks detected:\x1b[0m\n");
            }
            memcheck_alloc_t *a = &g_memcheck.allocs[i];
            fprintf(stderr, "  %p: %zu bytes allocated at %s:%d\n",
                    a->ptr, a->size, a->file, a->line);
            leaks++;
            leaked_bytes += a->size;
        }
    }

    if (leaks > 0) {
        fprintf(stderr, "\x1b[31mTotal: %d leaks, %zu bytes\x1b[0m\n\n",
                leaks, leaked_bytes);
    }
}

/**
 * Print memory usage statistics.
 */
static inline void memcheck_stats(void) {
    printf("\nMemory statistics:\n");
    printf("  Total allocations: %d\n", g_memcheck.total_allocs);
    printf("  Total frees: %d\n", g_memcheck.total_frees);
    printf("  Total bytes allocated: %zu\n", g_memcheck.total_bytes);
    printf("  Peak bytes in use: %zu\n", g_memcheck.peak_bytes);
    printf("  Current bytes in use: %zu\n", g_memcheck.current_bytes);
}

/**
 * Clean up the memory checker.
 */
static inline void memcheck_cleanup(void) {
    memset(&g_memcheck, 0, sizeof(g_memcheck));
}

/**
 * Assert no leaks - fails the current test if leaks exist.
 */
#define ASSERT_NO_LEAKS() do { \
    int _leaks = memcheck_leak_count(); \
    if (_leaks > 0) { \
        memcheck_report(); \
        printf("\x1b[31m  X \x1b[0m%s:%d: %d memory leak(s) detected\n", \
               __FILE__, __LINE__, _leaks); \
        test_stats.current_test_failed = 1; \
        test_stats.failed_tests++; \
        return; \
    } \
} while(0)

/* =============================================================================
 * Allocation Macros
 * =============================================================================
 */

/**
 * Tracked malloc - use instead of malloc() in tests.
 */
#define MEMCHECK_MALLOC(size) \
    memcheck_track_alloc(malloc(size), (size), __FILE__, __LINE__)

/**
 * Tracked calloc - use instead of calloc() in tests.
 */
#define MEMCHECK_CALLOC(count, size) \
    memcheck_track_alloc(calloc((count), (size)), (count) * (size), __FILE__, __LINE__)

/**
 * Tracked realloc - use instead of realloc() in tests.
 */
#define MEMCHECK_REALLOC(ptr, size) \
    memcheck_track_realloc((ptr), realloc((ptr), (size)), (size), __FILE__, __LINE__)

/**
 * Tracked free - use instead of free() in tests.
 */
#define MEMCHECK_FREE(ptr) do { \
    memcheck_track_free((ptr), __FILE__, __LINE__); \
    free(ptr); \
} while(0)

/**
 * Tracked strdup - use instead of strdup() in tests.
 */
#define MEMCHECK_STRDUP(str) \
    memcheck_track_alloc(strdup(str), strlen(str) + 1, __FILE__, __LINE__)

/* =============================================================================
 * Test Framework Integration
 * =============================================================================
 */

/**
 * Begin test suite with memory tracking.
 * Use instead of BEGIN_TEST_SUITE when you want leak detection.
 */
#define BEGIN_TEST_SUITE_MEMCHECK(name) \
    int main(void) { \
        printf("\n\x1b[33mRunning test suite: \x1b[0m%s\n\n", name); \
        test_stats.total_tests = 0; \
        test_stats.passed_tests = 0; \
        test_stats.failed_tests = 0; \
        memcheck_init();

/**
 * End test suite with memory tracking and leak report.
 * Use instead of END_TEST_SUITE when you want leak detection.
 */
#define END_TEST_SUITE_MEMCHECK() \
        memcheck_report(); \
        memcheck_stats(); \
        memcheck_cleanup(); \
        printf("\n\x1b[33mResults: \x1b[0m"); \
        if (test_stats.failed_tests == 0) { \
            printf("\x1b[32m%d/%d tests passed\n\x1b[0m", \
                   test_stats.passed_tests, test_stats.total_tests); \
            return 0; \
        } else { \
            printf("\x1b[31m%d/%d tests passed, %d failed\n\x1b[0m", \
                   test_stats.passed_tests, test_stats.total_tests, \
                   test_stats.failed_tests); \
            return 1; \
        } \
    }

/**
 * Run a test with per-test leak checking.
 * Fails the test if any memory allocated during the test is not freed.
 */
#define RUN_TEST_MEMCHECK(name) do { \
    memcheck_begin(); \
    test_stats.total_tests++; \
    test_##name##_wrapper(); \
    int _leaks = memcheck_end(); \
    if (_leaks > 0 && !test_stats.current_test_failed) { \
        printf("\x1b[31m  X \x1b[0m%s: %d memory leak(s)\n", #name, _leaks); \
        memcheck_report(); \
        test_stats.current_test_failed = 1; \
        test_stats.failed_tests++; \
        test_stats.passed_tests--; \
    } \
} while(0)

#endif /* TEST_MEMCHECK_H */
