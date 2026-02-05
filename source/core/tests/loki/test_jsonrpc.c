/**
 * @file test_jsonrpc.c
 * @brief Unit tests for JSON-RPC harness and ViewModel serialization.
 *
 * Tests jsonrpc_serialize_viewmodel() and JSON building utilities.
 */

#include "test_framework.h"
#include "loki/jsonrpc.h"
#include "loki/json.h"
#include "loki/session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * Helper Functions
 *============================================================================*/

/* Create a minimal EditorViewModel for testing */
static EditorViewModel *create_test_viewmodel(void) {
    EditorViewModel *vm = calloc(1, sizeof(EditorViewModel));
    if (!vm) return NULL;

    vm->rows = 24;
    vm->cols = 80;
    vm->gutter_width = 4;

    /* Cursor */
    vm->cursor.row = 5;
    vm->cursor.col = 10;
    vm->cursor.file_row = 5;
    vm->cursor.file_col = 10;
    vm->cursor.visible = 1;

    /* Status - use static strings (not owned for minimal test) */
    vm->status.mode = "normal";
    vm->status.filename = "test.alda";
    vm->status.lang = "Alda";
    vm->status.numrows = 100;
    vm->status.current_row = 5;
    vm->status.dirty = 0;
    vm->status.playing = 0;
    vm->status.link_active = 0;

    /* Message */
    vm->message = NULL;

    /* Tabs */
    vm->tabs.count = 1;
    vm->tabs.active = 0;
    vm->tabs.labels = calloc(1, sizeof(char *));
    if (vm->tabs.labels) {
        vm->tabs.labels[0] = "test.alda";
    }

    /* No REPL */
    vm->repl_active = 0;

    /* No rows initially */
    vm->row_views = NULL;
    vm->row_count = 0;

    return vm;
}

/* Free test viewmodel (minimal version - doesn't own strings) */
static void free_test_viewmodel(EditorViewModel *vm) {
    if (!vm) return;
    free(vm->tabs.labels);
    free(vm->row_views);
    free(vm);
}

/* Check if JSON string contains a substring */
static int json_contains(const char *json, const char *substr) {
    return json && substr && strstr(json, substr) != NULL;
}

/*============================================================================
 * Null Safety Tests
 *============================================================================*/

TEST(serialize_null_viewmodel_returns_null) {
    char *result = jsonrpc_serialize_viewmodel(NULL);
    ASSERT_NULL(result);
}

/*============================================================================
 * Basic Serialization Tests
 *============================================================================*/

TEST(serialize_minimal_viewmodel) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    /* Verify it's valid JSON starting with { */
    ASSERT_TRUE(json[0] == '{');

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_includes_dimensions) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"rows\":24"));
    ASSERT_TRUE(json_contains(json, "\"cols\":80"));
    ASSERT_TRUE(json_contains(json, "\"gutter_width\":4"));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_includes_cursor) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"cursor\""));
    ASSERT_TRUE(json_contains(json, "\"row\":5"));
    ASSERT_TRUE(json_contains(json, "\"col\":10"));
    ASSERT_TRUE(json_contains(json, "\"visible\":true"));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_includes_status) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"status\""));
    ASSERT_TRUE(json_contains(json, "\"mode\":\"normal\""));
    ASSERT_TRUE(json_contains(json, "\"filename\":\"test.alda\""));
    ASSERT_TRUE(json_contains(json, "\"lang\":\"Alda\""));
    ASSERT_TRUE(json_contains(json, "\"numrows\":100"));
    ASSERT_TRUE(json_contains(json, "\"dirty\":false"));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_includes_tabs) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"tabs\""));
    ASSERT_TRUE(json_contains(json, "\"count\":1"));
    ASSERT_TRUE(json_contains(json, "\"active\":0"));
    ASSERT_TRUE(json_contains(json, "\"labels\""));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_includes_repl_inactive) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"repl\""));
    ASSERT_TRUE(json_contains(json, "\"active\":false"));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_includes_rows_content) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"rows_content\""));

    free(json);
    free_test_viewmodel(vm);
}

/*============================================================================
 * Status Field Tests
 *============================================================================*/

TEST(serialize_status_playing_true) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    vm->status.playing = 1;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"playing\":true"));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_status_dirty_true) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    vm->status.dirty = 1;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"dirty\":true"));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_status_link_active) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    vm->status.link_active = 1;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"link_active\":true"));

    free(json);
    free_test_viewmodel(vm);
}

/*============================================================================
 * REPL Tests
 *============================================================================*/

TEST(serialize_repl_active) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    vm->repl_active = 1;
    vm->repl.prompt = "> ";
    vm->repl.input = "test input";
    vm->repl.input_len = 10;
    vm->repl.log_lines = NULL;
    vm->repl.log_count = 0;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"active\":true"));
    ASSERT_TRUE(json_contains(json, "\"prompt\":\"> \""));
    ASSERT_TRUE(json_contains(json, "\"input\":\"test input\""));
    ASSERT_TRUE(json_contains(json, "\"input_len\":10"));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_repl_with_log) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    vm->repl_active = 1;
    vm->repl.prompt = "> ";
    vm->repl.input = "";
    vm->repl.input_len = 0;

    const char *log_lines[] = {"line1", "line2", "line3"};
    vm->repl.log_lines = log_lines;
    vm->repl.log_count = 3;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"log\""));
    ASSERT_TRUE(json_contains(json, "\"line1\""));
    ASSERT_TRUE(json_contains(json, "\"line2\""));
    ASSERT_TRUE(json_contains(json, "\"line3\""));

    free(json);
    free_test_viewmodel(vm);
}

/*============================================================================
 * Row View Tests
 *============================================================================*/

TEST(serialize_with_row_views) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    /* Create one row view */
    vm->row_count = 1;
    vm->row_views = calloc(1, sizeof(EditorRowView));
    ASSERT_NOT_NULL(vm->row_views);

    vm->row_views[0].row_num = 1;
    vm->row_views[0].is_empty = 0;
    vm->row_views[0].segments = NULL;
    vm->row_views[0].segment_count = 0;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"row_num\":1"));
    ASSERT_TRUE(json_contains(json, "\"is_empty\":false"));
    ASSERT_TRUE(json_contains(json, "\"segments\""));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_empty_row) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    vm->row_count = 1;
    vm->row_views = calloc(1, sizeof(EditorRowView));
    ASSERT_NOT_NULL(vm->row_views);

    vm->row_views[0].row_num = 0;
    vm->row_views[0].is_empty = 1;
    vm->row_views[0].segments = NULL;
    vm->row_views[0].segment_count = 0;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"is_empty\":true"));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_row_with_segments) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    vm->row_count = 1;
    vm->row_views = calloc(1, sizeof(EditorRowView));
    ASSERT_NOT_NULL(vm->row_views);

    /* Create segments */
    RenderSegment *segs = calloc(2, sizeof(RenderSegment));
    ASSERT_NOT_NULL(segs);

    segs[0].text = "hello";
    segs[0].len = 5;
    segs[0].hl_type = 1;  /* Some highlight type */
    segs[0].selected = 0;

    segs[1].text = "world";
    segs[1].len = 5;
    segs[1].hl_type = 2;
    segs[1].selected = 1;

    vm->row_views[0].row_num = 1;
    vm->row_views[0].is_empty = 0;
    vm->row_views[0].segments = segs;
    vm->row_views[0].segment_count = 2;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"text\":\"hello\""));
    ASSERT_TRUE(json_contains(json, "\"text\":\"world\""));
    ASSERT_TRUE(json_contains(json, "\"hl_type\":1"));
    ASSERT_TRUE(json_contains(json, "\"hl_type\":2"));
    ASSERT_TRUE(json_contains(json, "\"selected\":false"));
    ASSERT_TRUE(json_contains(json, "\"selected\":true"));

    free(segs);
    free(json);
    free_test_viewmodel(vm);
}

/*============================================================================
 * Message Tests
 *============================================================================*/

TEST(serialize_null_message) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    vm->message = NULL;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"message\":null"));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_with_message) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    vm->message = "File saved successfully";

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"message\":\"File saved successfully\""));

    free(json);
    free_test_viewmodel(vm);
}

/*============================================================================
 * Multiple Tabs Tests
 *============================================================================*/

TEST(serialize_multiple_tabs) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    /* Free original tabs */
    free(vm->tabs.labels);

    /* Create 3 tabs */
    vm->tabs.count = 3;
    vm->tabs.active = 1;
    vm->tabs.labels = calloc(3, sizeof(char *));
    ASSERT_NOT_NULL(vm->tabs.labels);
    vm->tabs.labels[0] = "file1.alda";
    vm->tabs.labels[1] = "file2.joy";
    vm->tabs.labels[2] = "file3.scm";

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"count\":3"));
    ASSERT_TRUE(json_contains(json, "\"active\":1"));
    ASSERT_TRUE(json_contains(json, "\"file1.alda\""));
    ASSERT_TRUE(json_contains(json, "\"file2.joy\""));
    ASSERT_TRUE(json_contains(json, "\"file3.scm\""));

    free(json);
    free_test_viewmodel(vm);
}

/*============================================================================
 * JSON Builder Tests
 *============================================================================*/

TEST(json_builder_empty_object) {
    JsonBuilder jb;
    json_builder_init(&jb);

    json_object_start(&jb);
    json_object_end(&jb);

    const char *result = json_builder_get(&jb);
    ASSERT_STR_EQ(result, "{}");

    json_builder_free(&jb);
}

TEST(json_builder_empty_array) {
    JsonBuilder jb;
    json_builder_init(&jb);

    json_array_start(&jb);
    json_array_end(&jb);

    const char *result = json_builder_get(&jb);
    ASSERT_STR_EQ(result, "[]");

    json_builder_free(&jb);
}

TEST(json_builder_string_value) {
    JsonBuilder jb;
    json_builder_init(&jb);

    json_object_start(&jb);
    json_kv_string(&jb, "key", "value");
    json_object_end(&jb);

    const char *result = json_builder_get(&jb);
    ASSERT_TRUE(json_contains(result, "\"key\":\"value\""));

    json_builder_free(&jb);
}

TEST(json_builder_int_value) {
    JsonBuilder jb;
    json_builder_init(&jb);

    json_object_start(&jb);
    json_kv_int(&jb, "count", 42);
    json_object_end(&jb);

    const char *result = json_builder_get(&jb);
    ASSERT_TRUE(json_contains(result, "\"count\":42"));

    json_builder_free(&jb);
}

TEST(json_builder_bool_true) {
    JsonBuilder jb;
    json_builder_init(&jb);

    json_object_start(&jb);
    json_kv_bool(&jb, "enabled", 1);
    json_object_end(&jb);

    const char *result = json_builder_get(&jb);
    ASSERT_TRUE(json_contains(result, "\"enabled\":true"));

    json_builder_free(&jb);
}

TEST(json_builder_bool_false) {
    JsonBuilder jb;
    json_builder_init(&jb);

    json_object_start(&jb);
    json_kv_bool(&jb, "enabled", 0);
    json_object_end(&jb);

    const char *result = json_builder_get(&jb);
    ASSERT_TRUE(json_contains(result, "\"enabled\":false"));

    json_builder_free(&jb);
}

TEST(json_builder_nested_object) {
    JsonBuilder jb;
    json_builder_init(&jb);

    json_object_start(&jb);
    json_key(&jb, "nested");
    json_object_start(&jb);
    json_kv_int(&jb, "inner", 123);
    json_object_end(&jb);
    json_object_end(&jb);

    const char *result = json_builder_get(&jb);
    ASSERT_TRUE(json_contains(result, "\"nested\""));
    ASSERT_TRUE(json_contains(result, "\"inner\":123"));

    json_builder_free(&jb);
}

TEST(json_builder_array_of_strings) {
    JsonBuilder jb;
    json_builder_init(&jb);

    json_array_start(&jb);
    json_string(&jb, "one");
    json_string(&jb, "two");
    json_string(&jb, "three");
    json_array_end(&jb);

    const char *result = json_builder_get(&jb);
    ASSERT_TRUE(json_contains(result, "["));
    ASSERT_TRUE(json_contains(result, "\"one\""));
    ASSERT_TRUE(json_contains(result, "\"two\""));
    ASSERT_TRUE(json_contains(result, "\"three\""));
    ASSERT_TRUE(json_contains(result, "]"));

    json_builder_free(&jb);
}

TEST(json_builder_array_of_ints) {
    JsonBuilder jb;
    json_builder_init(&jb);

    json_array_start(&jb);
    json_int(&jb, 1);
    json_int(&jb, 2);
    json_int(&jb, 3);
    json_array_end(&jb);

    const char *result = json_builder_get(&jb);
    ASSERT_TRUE(json_contains(result, "1"));
    ASSERT_TRUE(json_contains(result, "2"));
    ASSERT_TRUE(json_contains(result, "3"));

    json_builder_free(&jb);
}

TEST(json_builder_null_value) {
    JsonBuilder jb;
    json_builder_init(&jb);

    json_object_start(&jb);
    json_key(&jb, "value");
    json_null(&jb);
    json_object_end(&jb);

    const char *result = json_builder_get(&jb);
    ASSERT_TRUE(json_contains(result, "\"value\":null"));

    json_builder_free(&jb);
}

TEST(json_builder_string_with_length) {
    JsonBuilder jb;
    json_builder_init(&jb);

    /* String with embedded content to test length-based handling */
    const char *text = "hello world";

    json_object_start(&jb);
    json_key(&jb, "partial");
    json_string_len(&jb, text, 5);  /* Just "hello" */
    json_object_end(&jb);

    const char *result = json_builder_get(&jb);
    ASSERT_TRUE(json_contains(result, "\"partial\":\"hello\""));

    json_builder_free(&jb);
}

TEST(json_builder_reset) {
    JsonBuilder jb;
    json_builder_init(&jb);

    json_object_start(&jb);
    json_kv_int(&jb, "first", 1);
    json_object_end(&jb);

    const char *result1 = json_builder_get(&jb);
    ASSERT_TRUE(json_contains(result1, "\"first\":1"));

    json_builder_reset(&jb);

    json_object_start(&jb);
    json_kv_int(&jb, "second", 2);
    json_object_end(&jb);

    const char *result2 = json_builder_get(&jb);
    ASSERT_TRUE(json_contains(result2, "\"second\":2"));
    ASSERT_FALSE(json_contains(result2, "\"first\""));

    json_builder_free(&jb);
}

/*============================================================================
 * JSON Parser Tests
 *============================================================================*/

TEST(json_parse_empty_object) {
    JsonValue val = json_parse("{}");
    ASSERT_EQ(val.type, JSON_OBJECT);
    ASSERT_EQ(val.data.object_val.count, 0);
    json_value_free(&val);
}

TEST(json_parse_simple_object) {
    JsonValue val = json_parse("{\"cmd\":\"load\"}");
    ASSERT_EQ(val.type, JSON_OBJECT);

    const char *cmd = json_object_get_string(&val, "cmd");
    ASSERT_NOT_NULL(cmd);
    ASSERT_STR_EQ(cmd, "load");

    json_value_free(&val);
}

TEST(json_parse_object_with_int) {
    JsonValue val = json_parse("{\"code\":65}");
    ASSERT_EQ(val.type, JSON_OBJECT);

    int code = json_object_get_int(&val, "code", -1);
    ASSERT_EQ(code, 65);

    json_value_free(&val);
}

TEST(json_parse_object_with_bool) {
    JsonValue val = json_parse("{\"enabled\":true}");
    ASSERT_EQ(val.type, JSON_OBJECT);

    int enabled = json_object_get_bool(&val, "enabled", 0);
    ASSERT_EQ(enabled, 1);

    json_value_free(&val);
}

TEST(json_parse_missing_key_returns_default) {
    JsonValue val = json_parse("{\"other\":123}");
    ASSERT_EQ(val.type, JSON_OBJECT);

    int missing = json_object_get_int(&val, "missing", 42);
    ASSERT_EQ(missing, 42);

    const char *str = json_object_get_string(&val, "missing");
    ASSERT_NULL(str);

    json_value_free(&val);
}

TEST(json_parse_invalid_returns_error) {
    JsonValue val = json_parse("not json");
    ASSERT_EQ(val.type, JSON_ERROR);
}

TEST(json_parse_empty_string_returns_error) {
    JsonValue val = json_parse("");
    ASSERT_EQ(val.type, JSON_ERROR);
}

TEST(json_parse_multiple_keys) {
    JsonValue val = json_parse("{\"cmd\":\"event\",\"type\":\"key\",\"code\":27}");
    ASSERT_EQ(val.type, JSON_OBJECT);

    const char *cmd = json_object_get_string(&val, "cmd");
    ASSERT_STR_EQ(cmd, "event");

    const char *type = json_object_get_string(&val, "type");
    ASSERT_STR_EQ(type, "key");

    int code = json_object_get_int(&val, "code", -1);
    ASSERT_EQ(code, 27);

    json_value_free(&val);
}

/*============================================================================
 * Edge Cases
 *============================================================================*/

TEST(serialize_null_filename_in_status) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    vm->status.filename = NULL;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"filename\":null"));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_empty_tabs) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    free(vm->tabs.labels);
    vm->tabs.labels = NULL;
    vm->tabs.count = 0;
    vm->tabs.active = 0;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"count\":0"));
    ASSERT_TRUE(json_contains(json, "\"labels\":[]"));

    free(json);
    free_test_viewmodel(vm);
}

TEST(serialize_cursor_invisible) {
    EditorViewModel *vm = create_test_viewmodel();
    ASSERT_NOT_NULL(vm);

    vm->cursor.visible = 0;

    char *json = jsonrpc_serialize_viewmodel(vm);
    ASSERT_NOT_NULL(json);

    ASSERT_TRUE(json_contains(json, "\"visible\":false"));

    free(json);
    free_test_viewmodel(vm);
}

/*============================================================================
 * Main Test Runner
 *============================================================================*/

BEGIN_TEST_SUITE("JSON-RPC Tests")
    /* Null safety */
    RUN_TEST(serialize_null_viewmodel_returns_null);

    /* Basic serialization */
    RUN_TEST(serialize_minimal_viewmodel);
    RUN_TEST(serialize_includes_dimensions);
    RUN_TEST(serialize_includes_cursor);
    RUN_TEST(serialize_includes_status);
    RUN_TEST(serialize_includes_tabs);
    RUN_TEST(serialize_includes_repl_inactive);
    RUN_TEST(serialize_includes_rows_content);

    /* Status fields */
    RUN_TEST(serialize_status_playing_true);
    RUN_TEST(serialize_status_dirty_true);
    RUN_TEST(serialize_status_link_active);

    /* REPL */
    RUN_TEST(serialize_repl_active);
    RUN_TEST(serialize_repl_with_log);

    /* Row views */
    RUN_TEST(serialize_with_row_views);
    RUN_TEST(serialize_empty_row);
    RUN_TEST(serialize_row_with_segments);

    /* Messages */
    RUN_TEST(serialize_null_message);
    RUN_TEST(serialize_with_message);

    /* Multiple tabs */
    RUN_TEST(serialize_multiple_tabs);

    /* JSON Builder */
    RUN_TEST(json_builder_empty_object);
    RUN_TEST(json_builder_empty_array);
    RUN_TEST(json_builder_string_value);
    RUN_TEST(json_builder_int_value);
    RUN_TEST(json_builder_bool_true);
    RUN_TEST(json_builder_bool_false);
    RUN_TEST(json_builder_nested_object);
    RUN_TEST(json_builder_array_of_strings);
    RUN_TEST(json_builder_array_of_ints);
    RUN_TEST(json_builder_null_value);
    RUN_TEST(json_builder_string_with_length);
    RUN_TEST(json_builder_reset);

    /* JSON Parser */
    RUN_TEST(json_parse_empty_object);
    RUN_TEST(json_parse_simple_object);
    RUN_TEST(json_parse_object_with_int);
    RUN_TEST(json_parse_object_with_bool);
    RUN_TEST(json_parse_missing_key_returns_default);
    RUN_TEST(json_parse_invalid_returns_error);
    RUN_TEST(json_parse_empty_string_returns_error);
    RUN_TEST(json_parse_multiple_keys);

    /* Edge cases */
    RUN_TEST(serialize_null_filename_in_status);
    RUN_TEST(serialize_empty_tabs);
    RUN_TEST(serialize_cursor_invisible);
END_TEST_SUITE()
