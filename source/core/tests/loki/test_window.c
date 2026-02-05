/* test_window.c - Unit tests for binary tree window manager
 *
 * Tests window creation, splitting, navigation, layout, and closing.
 */

#include "test_framework.h"
#include "loki/window.h"
#include "loki/internal.h"  /* For EditorView, MODE_NORMAL */
#include <stdio.h>
#include <string.h>

/* Callback for window_foreach_leaf - counts leaves */
static void leaf_count_callback(WindowNode *node, void *userdata) {
    (void)node;
    (*(int *)userdata)++;
}

/* ======================= Creation/Destruction Tests ======================= */

TEST(window_manager_create_basic) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);
    ASSERT_NOT_NULL(wm->root);
    ASSERT_NOT_NULL(wm->active);
    ASSERT_TRUE(wm->active == wm->root);
    ASSERT_EQ(window_count_panes(wm), 1);

    window_manager_destroy(wm);
}

TEST(window_manager_initial_state) {
    WindowManager *wm = window_manager_create(42);
    ASSERT_NOT_NULL(wm);

    /* Root should be a leaf with correct model_id */
    ASSERT_TRUE(window_node_is_leaf(wm->root));
    ASSERT_EQ(wm->root->model_id, 42);
    ASSERT_EQ(wm->root->split, SPLIT_NONE);
    ASSERT_NULL(wm->root->first);
    ASSERT_NULL(wm->root->second);
    ASSERT_NULL(wm->root->parent);

    window_manager_destroy(wm);
}

TEST(window_manager_destroy_null) {
    /* Should not crash */
    window_manager_destroy(NULL);
}

/* ======================= Split Tests ====================================== */

TEST(window_split_vertical) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    int result = window_split(wm, SPLIT_VERTICAL);
    ASSERT_EQ(result, 0);

    /* Root should now be a container */
    ASSERT_FALSE(window_node_is_leaf(wm->root));
    ASSERT_EQ(wm->root->split, SPLIT_VERTICAL);
    ASSERT_NOT_NULL(wm->root->first);
    ASSERT_NOT_NULL(wm->root->second);

    /* Both children should be leaves */
    ASSERT_TRUE(window_node_is_leaf(wm->root->first));
    ASSERT_TRUE(window_node_is_leaf(wm->root->second));

    /* Should have 2 panes now */
    ASSERT_EQ(window_count_panes(wm), 2);

    /* Active should be the new pane (first child) */
    ASSERT_TRUE(wm->active == wm->root->first);

    /* Both should have same model_id */
    ASSERT_EQ(wm->root->first->model_id, 1);
    ASSERT_EQ(wm->root->second->model_id, 1);

    window_manager_destroy(wm);
}

TEST(window_split_horizontal) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    int result = window_split(wm, SPLIT_HORIZONTAL);
    ASSERT_EQ(result, 0);

    ASSERT_EQ(wm->root->split, SPLIT_HORIZONTAL);
    ASSERT_EQ(window_count_panes(wm), 2);

    window_manager_destroy(wm);
}

TEST(window_split_multiple) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Split vertically: creates left and right */
    window_split(wm, SPLIT_VERTICAL);
    ASSERT_EQ(window_count_panes(wm), 2);

    /* Active is now left pane - split it horizontally */
    window_split(wm, SPLIT_HORIZONTAL);
    ASSERT_EQ(window_count_panes(wm), 3);

    /* Should have: root(V) -> first(H) -> {first(leaf), second(leaf)}, second(leaf) */
    ASSERT_FALSE(window_node_is_leaf(wm->root));
    ASSERT_FALSE(window_node_is_leaf(wm->root->first));
    ASSERT_TRUE(window_node_is_leaf(wm->root->second));

    window_manager_destroy(wm);
}

TEST(window_split_invalid) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* SPLIT_NONE is invalid */
    int result = window_split(wm, SPLIT_NONE);
    ASSERT_EQ(result, -1);
    ASSERT_EQ(window_count_panes(wm), 1);

    /* NULL manager */
    result = window_split(NULL, SPLIT_VERTICAL);
    ASSERT_EQ(result, -1);

    window_manager_destroy(wm);
}

/* ======================= Navigation Tests ================================= */

TEST(window_navigate_basic) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Split vertically to create left/right panes */
    window_split(wm, SPLIT_VERTICAL);

    /* Layout the tree so navigation has geometry to work with */
    window_layout(wm, 0, 0, 80, 24);

    /* Active is first (left) pane */
    WindowNode *left = wm->active;
    WindowNode *right = wm->root->second;

    /* Navigate right */
    int result = window_navigate(wm, 'l');
    ASSERT_EQ(result, 0);
    ASSERT_TRUE(wm->active == right);

    /* Navigate left */
    result = window_navigate(wm, 'h');
    ASSERT_EQ(result, 0);
    ASSERT_TRUE(wm->active == left);

    window_manager_destroy(wm);
}

TEST(window_navigate_vertical) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Split horizontally to create top/bottom panes */
    window_split(wm, SPLIT_HORIZONTAL);
    window_layout(wm, 0, 0, 80, 24);

    WindowNode *top = wm->active;
    WindowNode *bottom = wm->root->second;

    /* Navigate down */
    int result = window_navigate(wm, 'j');
    ASSERT_EQ(result, 0);
    ASSERT_TRUE(wm->active == bottom);

    /* Navigate up */
    result = window_navigate(wm, 'k');
    ASSERT_EQ(result, 0);
    ASSERT_TRUE(wm->active == top);

    window_manager_destroy(wm);
}

TEST(window_navigate_no_neighbor) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Single pane - navigation should fail */
    int result = window_navigate(wm, 'l');
    ASSERT_EQ(result, -1);

    /* Split vertically */
    window_split(wm, SPLIT_VERTICAL);
    window_layout(wm, 0, 0, 80, 24);

    /* From left pane, no neighbor further left */
    result = window_navigate(wm, 'h');
    ASSERT_EQ(result, -1);

    window_manager_destroy(wm);
}

TEST(window_cycle_basic) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Single pane - cycle should fail */
    int result = window_cycle(wm);
    ASSERT_EQ(result, -1);

    /* Split to create two panes */
    window_split(wm, SPLIT_VERTICAL);
    WindowNode *first = wm->active;

    /* Cycle to next */
    result = window_cycle(wm);
    ASSERT_EQ(result, 0);
    ASSERT_TRUE(wm->active != first);

    /* Cycle again - should wrap to first */
    result = window_cycle(wm);
    ASSERT_EQ(result, 0);
    ASSERT_TRUE(wm->active == first);

    window_manager_destroy(wm);
}

/* ======================= Close Tests ====================================== */

TEST(window_close_only_pane) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Cannot close the only pane */
    int result = window_close(wm);
    ASSERT_EQ(result, -1);
    ASSERT_EQ(window_count_panes(wm), 1);

    window_manager_destroy(wm);
}

TEST(window_close_one_of_two) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    window_split(wm, SPLIT_VERTICAL);
    ASSERT_EQ(window_count_panes(wm), 2);

    /* Close active pane */
    int result = window_close(wm);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(window_count_panes(wm), 1);

    /* Root should be a leaf again */
    ASSERT_TRUE(window_node_is_leaf(wm->root));

    window_manager_destroy(wm);
}

TEST(window_close_with_nested_splits) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Create 3 panes: root(V) -> first(H) -> {A, B}, C */
    window_split(wm, SPLIT_VERTICAL);   /* Left, Right */
    window_split(wm, SPLIT_HORIZONTAL); /* Left becomes Top-Left, Bottom-Left */
    ASSERT_EQ(window_count_panes(wm), 3);

    /* Active is top-left (A) */
    /* Close it - should result in 2 panes */
    int result = window_close(wm);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(window_count_panes(wm), 2);

    window_manager_destroy(wm);
}

/* ======================= Layout Tests ===================================== */

TEST(window_layout_single) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    window_layout(wm, 0, 0, 80, 24);

    ASSERT_EQ(wm->root->x, 0);
    ASSERT_EQ(wm->root->y, 0);
    ASSERT_EQ(wm->root->width, 80);
    ASSERT_EQ(wm->root->height, 24);

    window_manager_destroy(wm);
}

TEST(window_layout_vertical_split) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    window_split(wm, SPLIT_VERTICAL);
    window_layout(wm, 0, 0, 81, 24);  /* 81 to account for separator */

    /* First (left) should be left half */
    WindowNode *left = wm->root->first;
    WindowNode *right = wm->root->second;

    ASSERT_EQ(left->x, 0);
    ASSERT_EQ(left->y, 0);
    ASSERT_EQ(left->width, 40);  /* Half of 81 = 40, separator = 1, other = 40 */
    ASSERT_EQ(left->height, 24);

    ASSERT_EQ(right->x, 41);  /* 40 + 1 separator */
    ASSERT_EQ(right->y, 0);
    ASSERT_EQ(right->width, 40);
    ASSERT_EQ(right->height, 24);

    window_manager_destroy(wm);
}

TEST(window_layout_horizontal_split) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    window_split(wm, SPLIT_HORIZONTAL);
    window_layout(wm, 0, 0, 80, 25);  /* 25 to account for separator */

    WindowNode *top = wm->root->first;
    WindowNode *bottom = wm->root->second;

    ASSERT_EQ(top->x, 0);
    ASSERT_EQ(top->y, 0);
    ASSERT_EQ(top->width, 80);
    ASSERT_EQ(top->height, 12);  /* Half of 25 = 12, separator = 1, other = 12 */

    ASSERT_EQ(bottom->x, 0);
    ASSERT_EQ(bottom->y, 13);  /* 12 + 1 separator */
    ASSERT_EQ(bottom->width, 80);
    ASSERT_EQ(bottom->height, 12);

    window_manager_destroy(wm);
}

TEST(window_layout_with_offset) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Layout with non-zero offset (e.g., after tab bar) */
    window_layout(wm, 0, 1, 80, 23);

    ASSERT_EQ(wm->root->x, 0);
    ASSERT_EQ(wm->root->y, 1);
    ASSERT_EQ(wm->root->width, 80);
    ASSERT_EQ(wm->root->height, 23);

    window_manager_destroy(wm);
}

/* ======================= Equalize Tests =================================== */

TEST(window_equalize_basic) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    window_split(wm, SPLIT_VERTICAL);

    /* Manually change ratio */
    wm->root->split_ratio = 0.7f;

    window_equalize(wm);

    /* Ratio should be reset to 0.5 */
    ASSERT_NEAR(wm->root->split_ratio, 0.5f, 0.001f);

    window_manager_destroy(wm);
}

/* ======================= Query Tests ====================================== */

TEST(window_find_by_model) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Single pane with model_id = 1 */
    WindowNode *found = window_find_by_model(wm, 1);
    ASSERT_NOT_NULL(found);
    ASSERT_TRUE(found == wm->root);

    /* Not found */
    found = window_find_by_model(wm, 999);
    ASSERT_NULL(found);

    window_manager_destroy(wm);
}

TEST(window_foreach_leaf) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    window_split(wm, SPLIT_VERTICAL);
    window_split(wm, SPLIT_HORIZONTAL);

    /* Count leaves via callback */
    int count = 0;
    window_foreach_leaf(wm, leaf_count_callback, &count);
    ASSERT_EQ(count, 3);

    window_manager_destroy(wm);
}

TEST(window_set_active) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    window_split(wm, SPLIT_VERTICAL);

    WindowNode *first = wm->root->first;
    WindowNode *second = wm->root->second;

    /* Set active to second */
    int result = window_set_active(wm, second);
    ASSERT_EQ(result, 0);
    ASSERT_TRUE(wm->active == second);

    /* Set active to first */
    result = window_set_active(wm, first);
    ASSERT_EQ(result, 0);
    ASSERT_TRUE(wm->active == first);

    /* Cannot set container as active */
    result = window_set_active(wm, wm->root);
    ASSERT_EQ(result, -1);

    window_manager_destroy(wm);
}

/* ======================= Node Helper Tests ================================ */

TEST(window_node_sibling) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    window_split(wm, SPLIT_VERTICAL);

    WindowNode *first = wm->root->first;
    WindowNode *second = wm->root->second;

    ASSERT_TRUE(window_node_sibling(first) == second);
    ASSERT_TRUE(window_node_sibling(second) == first);

    /* Root has no sibling */
    ASSERT_NULL(window_node_sibling(wm->root));

    window_manager_destroy(wm);
}

TEST(window_node_is_leaf_check) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Initial root is a leaf */
    ASSERT_TRUE(window_node_is_leaf(wm->root));

    window_split(wm, SPLIT_VERTICAL);

    /* After split, root is no longer a leaf */
    ASSERT_FALSE(window_node_is_leaf(wm->root));
    ASSERT_TRUE(window_node_is_leaf(wm->root->first));
    ASSERT_TRUE(window_node_is_leaf(wm->root->second));

    window_manager_destroy(wm);
}

/* ======================= View Management Tests ============================= */

TEST(window_view_create_basic) {
    EditorView *view = window_view_create();
    ASSERT_NOT_NULL(view);

    /* Check defaults */
    ASSERT_EQ(view->cx, 0);
    ASSERT_EQ(view->cy, 0);
    ASSERT_EQ(view->rowoff, 0);
    ASSERT_EQ(view->coloff, 0);
    ASSERT_EQ(view->mode, MODE_NORMAL);
    ASSERT_TRUE(view->line_numbers);  /* Default enabled */

    window_view_destroy(view);
}

TEST(window_view_clone_basic) {
    EditorView *src = window_view_create();
    ASSERT_NOT_NULL(src);

    /* Set some state */
    src->cx = 10;
    src->cy = 20;
    src->rowoff = 100;
    src->coloff = 5;
    src->sel_active = 1;
    src->sel_start_x = 1;
    src->sel_start_y = 2;

    EditorView *clone = window_view_clone(src);
    ASSERT_NOT_NULL(clone);

    /* Check cloned state */
    ASSERT_EQ(clone->cx, 10);
    ASSERT_EQ(clone->cy, 20);
    ASSERT_EQ(clone->rowoff, 100);
    ASSERT_EQ(clone->coloff, 5);
    ASSERT_EQ(clone->sel_active, 1);
    ASSERT_EQ(clone->sel_start_x, 1);
    ASSERT_EQ(clone->sel_start_y, 2);

    /* Clone should be independent */
    clone->cx = 50;
    ASSERT_EQ(src->cx, 10);  /* Source unchanged */

    window_view_destroy(src);
    window_view_destroy(clone);
}

TEST(window_view_destroy_null) {
    /* Should not crash */
    window_view_destroy(NULL);
}

TEST(window_split_creates_views) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Create initial view for the root */
    wm->active->view = window_view_create();
    wm->active->view->cx = 5;
    wm->active->view->cy = 10;
    wm->active->view->rowoff = 50;

    /* Split should create view for new pane */
    int result = window_split(wm, SPLIT_VERTICAL);
    ASSERT_EQ(result, 0);

    WindowNode *new_pane = wm->active;  /* Active moves to new pane */
    WindowNode *old_pane = wm->root->second;

    /* Both panes should have views */
    ASSERT_NOT_NULL(old_pane->view);
    ASSERT_NOT_NULL(new_pane->view);

    /* New pane should have cloned state */
    ASSERT_EQ(new_pane->view->cx, 5);
    ASSERT_EQ(new_pane->view->cy, 10);
    ASSERT_EQ(new_pane->view->rowoff, 50);

    /* Old pane should retain original view */
    ASSERT_EQ(old_pane->view->cx, 5);
    ASSERT_EQ(old_pane->view->cy, 10);
    ASSERT_EQ(old_pane->view->rowoff, 50);

    /* Views should be independent */
    new_pane->view->cx = 100;
    ASSERT_EQ(old_pane->view->cx, 5);

    window_manager_destroy(wm);
}

TEST(window_close_frees_view) {
    WindowManager *wm = window_manager_create(1);
    ASSERT_NOT_NULL(wm);

    /* Create initial view */
    wm->active->view = window_view_create();

    /* Split creates another view */
    window_split(wm, SPLIT_VERTICAL);
    ASSERT_EQ(window_count_panes(wm), 2);

    /* Close current pane (its view should be freed - no crash) */
    int result = window_close(wm);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(window_count_panes(wm), 1);

    /* Remaining pane should still have its view */
    ASSERT_NOT_NULL(wm->active->view);

    window_manager_destroy(wm);
}

/* ======================= Test Suite ======================================= */

BEGIN_TEST_SUITE("Window Manager")
    /* Creation/Destruction */
    RUN_TEST(window_manager_create_basic);
    RUN_TEST(window_manager_initial_state);
    RUN_TEST(window_manager_destroy_null);

    /* Splitting */
    RUN_TEST(window_split_vertical);
    RUN_TEST(window_split_horizontal);
    RUN_TEST(window_split_multiple);
    RUN_TEST(window_split_invalid);

    /* Navigation */
    RUN_TEST(window_navigate_basic);
    RUN_TEST(window_navigate_vertical);
    RUN_TEST(window_navigate_no_neighbor);
    RUN_TEST(window_cycle_basic);

    /* Closing */
    RUN_TEST(window_close_only_pane);
    RUN_TEST(window_close_one_of_two);
    RUN_TEST(window_close_with_nested_splits);

    /* Layout */
    RUN_TEST(window_layout_single);
    RUN_TEST(window_layout_vertical_split);
    RUN_TEST(window_layout_horizontal_split);
    RUN_TEST(window_layout_with_offset);

    /* Equalize */
    RUN_TEST(window_equalize_basic);

    /* Query */
    RUN_TEST(window_find_by_model);
    RUN_TEST(window_foreach_leaf);
    RUN_TEST(window_set_active);

    /* Node helpers */
    RUN_TEST(window_node_sibling);
    RUN_TEST(window_node_is_leaf_check);

    /* View management */
    RUN_TEST(window_view_create_basic);
    RUN_TEST(window_view_clone_basic);
    RUN_TEST(window_view_destroy_null);
    RUN_TEST(window_split_creates_views);
    RUN_TEST(window_close_frees_view);
END_TEST_SUITE()
