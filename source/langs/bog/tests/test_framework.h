/*
 * Minimal test framework for bog
 */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;
static const char* g_current_test = NULL;

#define TEST(name)                                                             \
    static void test_##name(void);                                             \
    static void run_test_##name(void)                                          \
    {                                                                          \
        g_current_test = #name;                                                \
        g_tests_run++;                                                         \
        test_##name();                                                         \
    }                                                                          \
    static void test_##name(void)

#define RUN_TEST(name)                                                         \
    do {                                                                       \
        run_test_##name();                                                     \
    } while (0)

#define ASSERT(cond)                                                           \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "  FAIL: %s\n    %s:%d: %s\n", g_current_test,     \
                    __FILE__, __LINE__, #cond);                                \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_EQ(a, b)                                                        \
    do {                                                                       \
        if ((a) != (b)) {                                                      \
            fprintf(stderr, "  FAIL: %s\n    %s:%d: %s != %s\n",               \
                    g_current_test, __FILE__, __LINE__, #a, #b);               \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_STREQ(a, b)                                                     \
    do {                                                                       \
        if (strcmp((a), (b)) != 0) {                                           \
            fprintf(stderr, "  FAIL: %s\n    %s:%d: \"%s\" != \"%s\"\n",       \
                    g_current_test, __FILE__, __LINE__, (a), (b));             \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_NEAR(a, b, epsilon)                                             \
    do {                                                                       \
        if (fabs((a) - (b)) > (epsilon)) {                                     \
            fprintf(stderr, "  FAIL: %s\n    %s:%d: %g != %g (eps=%g)\n",      \
                    g_current_test, __FILE__, __LINE__, (double)(a),           \
                    (double)(b), (double)(epsilon));                           \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_NULL(ptr)                                                       \
    do {                                                                       \
        if ((ptr) != NULL) {                                                   \
            fprintf(stderr, "  FAIL: %s\n    %s:%d: %s is not NULL\n",         \
                    g_current_test, __FILE__, __LINE__, #ptr);                 \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_NOT_NULL(ptr)                                                   \
    do {                                                                       \
        if ((ptr) == NULL) {                                                   \
            fprintf(stderr, "  FAIL: %s\n    %s:%d: %s is NULL\n",             \
                    g_current_test, __FILE__, __LINE__, #ptr);                 \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_GT(a, b)                                                        \
    do {                                                                       \
        if (!((a) > (b))) {                                                    \
            fprintf(stderr, "  FAIL: %s\n    %s:%d: %d > %d failed\n",         \
                    g_current_test, __FILE__, __LINE__, (int)(a), (int)(b));   \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_LT(a, b)                                                        \
    do {                                                                       \
        if (!((a) < (b))) {                                                    \
            fprintf(stderr, "  FAIL: %s\n    %s:%d: %d < %d failed\n",         \
                    g_current_test, __FILE__, __LINE__, (int)(a), (int)(b));   \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_GTE(a, b)                                                       \
    do {                                                                       \
        if (!((a) >= (b))) {                                                   \
            fprintf(stderr, "  FAIL: %s\n    %s:%d: %d >= %d failed\n",        \
                    g_current_test, __FILE__, __LINE__, (int)(a), (int)(b));   \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define ASSERT_LTE(a, b)                                                       \
    do {                                                                       \
        if (!((a) <= (b))) {                                                   \
            fprintf(stderr, "  FAIL: %s\n    %s:%d: %d <= %d failed\n",        \
                    g_current_test, __FILE__, __LINE__, (int)(a), (int)(b));   \
            g_tests_failed++;                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

#define TEST_PASS()                                                            \
    do {                                                                       \
        g_tests_passed++;                                                      \
        printf("  PASS: %s\n", g_current_test);                                \
    } while (0)

#define TEST_SUMMARY()                                                         \
    do {                                                                       \
        printf("\n%d tests, %d passed, %d failed\n", g_tests_run,              \
               g_tests_passed, g_tests_failed);                                \
    } while (0)

#define TEST_EXIT_CODE() (g_tests_failed > 0 ? 1 : 0)

/* =============================================================================
 * Test Fixtures
 * =============================================================================
 */

/* Define a fixture with its state structure */
#define FIXTURE(name, state_struct)                                            \
    typedef struct name##_fixture_state state_struct name##_fixture_state_t;   \
    static name##_fixture_state_t name##_fixture_instance;                     \
    static void name##_setup(name##_fixture_state_t *fixture);                 \
    static void name##_teardown(name##_fixture_state_t *fixture)

/* Define the setup function for a fixture */
#define FIXTURE_SETUP(name)                                                    \
    static void name##_setup(name##_fixture_state_t *fixture)

/* Define the teardown function for a fixture */
#define FIXTURE_TEARDOWN(name)                                                 \
    static void name##_teardown(name##_fixture_state_t *fixture)

/* Define a test that uses a fixture */
#define TEST_F(fixture_name, test_name)                                        \
    static void test_##fixture_name##_##test_name(                             \
        fixture_name##_fixture_state_t *fixture);                              \
    static void run_test_##fixture_name##_##test_name(void)                    \
    {                                                                          \
        g_current_test = #fixture_name "_" #test_name;                         \
        g_tests_run++;                                                         \
        fixture_name##_setup(&fixture_name##_fixture_instance);                \
        test_##fixture_name##_##test_name(&fixture_name##_fixture_instance);   \
        fixture_name##_teardown(&fixture_name##_fixture_instance);             \
    }                                                                          \
    static void test_##fixture_name##_##test_name(                             \
        fixture_name##_fixture_state_t *fixture)

/* Run a fixture-based test */
#define RUN_TEST_F(fixture_name, test_name)                                    \
    do {                                                                       \
        run_test_##fixture_name##_##test_name();                               \
    } while (0)

/* Suite-level setup/teardown */
#define SUITE_SETUP(name) static void name##_suite_setup(void)

#define SUITE_TEARDOWN(name) static void name##_suite_teardown(void)

#endif /* TEST_FRAMEWORK_H */
