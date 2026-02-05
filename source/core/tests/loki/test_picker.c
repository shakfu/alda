/* test_picker.c - Unit tests for modal picker
 *
 * Tests picker state machine, key handling, and selection logic.
 */

#include "test_framework.h"
#include "loki/core.h"
#include "loki/internal.h"
#include "loki/picker.h"
#include <stdio.h>
#include <string.h>

/* Helper: Initialize a minimal editor context for testing */
static void init_test_context(editor_ctx_t *ctx) {
    memset(ctx, 0, sizeof(editor_ctx_t));
    ctx->view.cx = 0;
    ctx->view.cy = 0;
    ctx->view.rowoff = 0;
    ctx->view.coloff = 0;
    ctx->model.numrows = 0;
    ctx->model.row = NULL;
    ctx->model.dirty = 0;
    ctx->model.filename = NULL;
    ctx->view.syntax = NULL;
    ctx->view.mode = MODE_NORMAL;
    ctx->view.screencols = 80;
    ctx->view.screenrows = 24;
}

/* Test items for picker */
static const char *test_items[] = {
    "Item 1",
    "Item 2",
    "Item 3",
    "Item 4",
    "Item 5"
};

/* Callback state for testing */
static struct {
    int called;
    int selected_index;
    void *user_data;
} callback_state;

static void test_callback(editor_ctx_t *ctx, int index, void *data) {
    (void)ctx;
    callback_state.called = 1;
    callback_state.selected_index = index;
    callback_state.user_data = data;
}

/* Test: Picker open */
TEST(picker_open) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    int result = picker_open(&ctx, "Test Picker", test_items, 5, test_callback, (void*)0x1234);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(ctx.view.mode, MODE_PICKER);
    ASSERT_EQ(ctx.view.picker.item_count, 5);
    ASSERT_EQ(ctx.view.picker.selected_index, 0);
    ASSERT_STR_EQ(ctx.view.picker.title, "Test Picker");
    ASSERT_EQ(ctx.view.picker.prev_mode, MODE_NORMAL);
}

/* Test: Picker close */
TEST(picker_close) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    picker_open(&ctx, "Test", test_items, 5, test_callback, NULL);
    picker_close(&ctx);

    ASSERT_EQ(ctx.view.mode, MODE_NORMAL);
    ASSERT_NULL(ctx.view.picker.items);
    ASSERT_EQ(ctx.view.picker.item_count, 0);
}

/* Test: Navigation with j/k keys */
TEST(picker_navigation) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    picker_open(&ctx, "Test", test_items, 5, test_callback, NULL);

    /* Move down with j */
    picker_handle_key(&ctx, 'j');
    ASSERT_EQ(ctx.view.picker.selected_index, 1);

    picker_handle_key(&ctx, 'j');
    ASSERT_EQ(ctx.view.picker.selected_index, 2);

    /* Move up with k */
    picker_handle_key(&ctx, 'k');
    ASSERT_EQ(ctx.view.picker.selected_index, 1);

    /* Clamp at top */
    picker_handle_key(&ctx, 'k');
    picker_handle_key(&ctx, 'k');
    picker_handle_key(&ctx, 'k');
    ASSERT_EQ(ctx.view.picker.selected_index, 0);

    picker_close(&ctx);
}

/* Test: Navigation with arrow keys */
TEST(picker_arrow_keys) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    picker_open(&ctx, "Test", test_items, 5, test_callback, NULL);

    /* Move down with arrow */
    picker_handle_key(&ctx, ARROW_DOWN);
    ASSERT_EQ(ctx.view.picker.selected_index, 1);

    /* Move up with arrow */
    picker_handle_key(&ctx, ARROW_UP);
    ASSERT_EQ(ctx.view.picker.selected_index, 0);

    picker_close(&ctx);
}

/* Test: Selection with ENTER */
TEST(picker_select) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    callback_state.called = 0;
    callback_state.selected_index = -99;
    callback_state.user_data = NULL;

    picker_open(&ctx, "Test", test_items, 5, test_callback, (void*)0xABCD);

    /* Move to item 2 */
    picker_handle_key(&ctx, 'j');
    picker_handle_key(&ctx, 'j');
    ASSERT_EQ(ctx.view.picker.selected_index, 2);

    /* Select with ENTER */
    picker_handle_key(&ctx, ENTER);

    /* Verify callback was called with correct index */
    ASSERT_TRUE(callback_state.called);
    ASSERT_EQ(callback_state.selected_index, 2);
    ASSERT_PTR_EQ(callback_state.user_data, (void*)0xABCD);

    /* Verify picker closed */
    ASSERT_EQ(ctx.view.mode, MODE_NORMAL);
}

/* Test: Cancel with ESC */
TEST(picker_cancel_esc) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    callback_state.called = 0;
    callback_state.selected_index = -99;

    picker_open(&ctx, "Test", test_items, 5, test_callback, NULL);

    /* Move to item 3 */
    picker_handle_key(&ctx, 'j');
    picker_handle_key(&ctx, 'j');
    picker_handle_key(&ctx, 'j');

    /* Cancel with ESC */
    picker_handle_key(&ctx, ESC);

    /* Verify callback was called with -1 (cancel) */
    ASSERT_TRUE(callback_state.called);
    ASSERT_EQ(callback_state.selected_index, -1);

    /* Verify picker closed */
    ASSERT_EQ(ctx.view.mode, MODE_NORMAL);
}

/* Test: Cancel with q */
TEST(picker_cancel_q) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    callback_state.called = 0;
    callback_state.selected_index = -99;

    picker_open(&ctx, "Test", test_items, 5, test_callback, NULL);

    /* Cancel with q */
    picker_handle_key(&ctx, 'q');

    /* Verify callback was called with -1 (cancel) */
    ASSERT_TRUE(callback_state.called);
    ASSERT_EQ(callback_state.selected_index, -1);

    /* Verify picker closed */
    ASSERT_EQ(ctx.view.mode, MODE_NORMAL);
}

/* Test: Go to top/bottom with g/G */
TEST(picker_goto) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    picker_open(&ctx, "Test", test_items, 5, test_callback, NULL);

    /* Go to bottom with G */
    picker_handle_key(&ctx, 'G');
    ASSERT_EQ(ctx.view.picker.selected_index, 4);

    /* Go to top with g */
    picker_handle_key(&ctx, 'g');
    ASSERT_EQ(ctx.view.picker.selected_index, 0);

    picker_close(&ctx);
}

/* Test: Clamp at bottom */
TEST(picker_clamp_bottom) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    picker_open(&ctx, "Test", test_items, 5, test_callback, NULL);

    /* Move beyond last item */
    for (int i = 0; i < 10; i++) {
        picker_handle_key(&ctx, 'j');
    }

    /* Should clamp at last item */
    ASSERT_EQ(ctx.view.picker.selected_index, 4);

    picker_close(&ctx);
}

/* Test: Get/set selected index */
TEST(picker_get_set_selected) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    /* Not in picker mode */
    ASSERT_EQ(picker_get_selected(&ctx), -1);

    picker_open(&ctx, "Test", test_items, 5, test_callback, NULL);

    ASSERT_EQ(picker_get_selected(&ctx), 0);

    picker_set_selected(&ctx, 3);
    ASSERT_EQ(picker_get_selected(&ctx), 3);

    /* Clamp out-of-range values */
    picker_set_selected(&ctx, 100);
    ASSERT_EQ(picker_get_selected(&ctx), 4);

    picker_set_selected(&ctx, -5);
    ASSERT_EQ(picker_get_selected(&ctx), 0);

    picker_close(&ctx);
}

/* Test: Picker returns handle status */
TEST(picker_handle_returns) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    picker_open(&ctx, "Test", test_items, 5, test_callback, NULL);

    /* Navigation returns 1 (still active) */
    ASSERT_EQ(picker_handle_key(&ctx, 'j'), 1);
    ASSERT_EQ(picker_handle_key(&ctx, 'k'), 1);

    /* Selection returns 0 (closed) */
    ASSERT_EQ(picker_handle_key(&ctx, ENTER), 0);
}

/* Test: Empty items rejected */
TEST(picker_empty_items) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    int result = picker_open(&ctx, "Test", test_items, 0, test_callback, NULL);
    ASSERT_EQ(result, -1);
    ASSERT_EQ(ctx.view.mode, MODE_NORMAL);

    result = picker_open(&ctx, "Test", NULL, 5, test_callback, NULL);
    ASSERT_EQ(result, -1);
}

/* Test: Restores previous mode */
TEST(picker_restores_mode) {
    editor_ctx_t ctx;
    init_test_context(&ctx);

    ctx.view.mode = MODE_INSERT;
    picker_open(&ctx, "Test", test_items, 5, test_callback, NULL);
    ASSERT_EQ(ctx.view.mode, MODE_PICKER);

    picker_close(&ctx);
    ASSERT_EQ(ctx.view.mode, MODE_INSERT);
}

/* Test suite */
BEGIN_TEST_SUITE("Picker")
    RUN_TEST(picker_open);
    RUN_TEST(picker_close);
    RUN_TEST(picker_navigation);
    RUN_TEST(picker_arrow_keys);
    RUN_TEST(picker_select);
    RUN_TEST(picker_cancel_esc);
    RUN_TEST(picker_cancel_q);
    RUN_TEST(picker_goto);
    RUN_TEST(picker_clamp_bottom);
    RUN_TEST(picker_get_set_selected);
    RUN_TEST(picker_handle_returns);
    RUN_TEST(picker_empty_items);
    RUN_TEST(picker_restores_mode);
END_TEST_SUITE()
