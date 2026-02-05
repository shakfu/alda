/**
 * @file test_context.c
 * @brief Unit tests for MHS context and state management.
 *
 * Tests the MHS state management functions including null safety,
 * lifecycle management, and error handling.
 */

#include "test_framework.h"
#include "mhs_context.h"
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * Null Safety Tests
 *============================================================================*/

TEST(init_null_context) {
    int result = loki_mhs_init(NULL);
    ASSERT_EQ(result, -1);
}

TEST(cleanup_null_context_safe) {
    /* Should not crash */
    loki_mhs_cleanup(NULL);
    ASSERT_TRUE(1);
}

TEST(is_initialized_null_context) {
    int result = loki_mhs_is_initialized(NULL);
    ASSERT_EQ(result, 0);
}

TEST(eval_null_context) {
    int result = loki_mhs_eval(NULL, "test code");
    ASSERT_EQ(result, -1);
}

TEST(eval_file_null_context) {
    int result = loki_mhs_eval_file(NULL, "test.hs");
    ASSERT_EQ(result, -1);
}

TEST(stop_null_context_safe) {
    /* Should not crash */
    loki_mhs_stop(NULL);
    ASSERT_TRUE(1);
}

TEST(is_playing_null_context) {
    int result = loki_mhs_is_playing(NULL);
    ASSERT_EQ(result, 0);
}

TEST(get_error_null_context) {
    const char *err = loki_mhs_get_error(NULL);
    ASSERT_NULL(err);
}

/*============================================================================
 * MIDI Port API Tests (don't require context)
 *============================================================================*/

TEST(list_ports_returns_count) {
    /* Should return a count >= 0 */
    int count = loki_mhs_list_ports();
    ASSERT_TRUE(count >= 0);
}

TEST(port_name_invalid_index) {
    /* Invalid index should return empty string */
    const char *name = loki_mhs_port_name(-1);
    ASSERT_NOT_NULL(name);
    ASSERT_EQ(strlen(name), (size_t)0);
}

TEST(port_name_large_index) {
    const char *name = loki_mhs_port_name(9999);
    ASSERT_NOT_NULL(name);
    ASSERT_EQ(strlen(name), (size_t)0);
}

TEST(open_port_invalid_index) {
    int result = loki_mhs_open_port(-1);
    ASSERT_EQ(result, -1);
}

TEST(open_port_large_index) {
    int result = loki_mhs_open_port(9999);
    ASSERT_EQ(result, -1);
}

/*============================================================================
 * State Structure Tests
 *============================================================================*/

TEST(loki_mhs_state_size) {
    /* LokiMhsState should have reasonable size */
    ASSERT_TRUE(sizeof(LokiMhsState) > 0);
    ASSERT_TRUE(sizeof(LokiMhsState) < 4096);  /* Sanity check */
}

TEST(loki_mhs_state_error_buffer_size) {
    /* Error buffer should be at least 256 bytes */
    ASSERT_TRUE(MHS_ERROR_BUFSIZE >= 256);
}

/*============================================================================
 * Main Test Runner
 *============================================================================*/

BEGIN_TEST_SUITE("MHS Context Tests")
    /* Null safety */
    RUN_TEST(init_null_context);
    RUN_TEST(cleanup_null_context_safe);
    RUN_TEST(is_initialized_null_context);
    RUN_TEST(eval_null_context);
    RUN_TEST(eval_file_null_context);
    RUN_TEST(stop_null_context_safe);
    RUN_TEST(is_playing_null_context);
    RUN_TEST(get_error_null_context);

    /* MIDI port API */
    RUN_TEST(list_ports_returns_count);
    RUN_TEST(port_name_invalid_index);
    RUN_TEST(port_name_large_index);
    RUN_TEST(open_port_invalid_index);
    RUN_TEST(open_port_large_index);

    /* State structure */
    RUN_TEST(loki_mhs_state_size);
    RUN_TEST(loki_mhs_state_error_buffer_size);
END_TEST_SUITE()
