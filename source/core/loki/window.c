/* window.c - Binary tree tiling window manager implementation
 *
 * Implements vim-style split windows using a binary tree layout.
 * See window.h for API documentation.
 */

#include "window.h"
#include "internal.h"
#include <stdlib.h>
#include <string.h>

/* ======================= Internal Helpers ================================= */

/* Create a new leaf node */
static WindowNode *node_create_leaf(int model_id) {
    WindowNode *node = calloc(1, sizeof(WindowNode));
    if (!node) return NULL;

    node->split = SPLIT_NONE;
    node->first = NULL;
    node->second = NULL;
    node->split_ratio = 0.5f;
    node->view = NULL;
    node->model_id = model_id;
    node->x = node->y = 0;
    node->width = node->height = 0;
    node->parent = NULL;

    return node;
}

/* Free a node and all its descendants */
static void node_free_recursive(WindowNode *node) {
    if (!node) return;

    if (node->split != SPLIT_NONE) {
        node_free_recursive(node->first);
        node_free_recursive(node->second);
    }

    /* Free the view if this is a leaf node */
    if (node->view) {
        window_view_destroy(node->view);
        node->view = NULL;
    }
    free(node);
}

/* Count leaves in a subtree */
static int count_leaves(const WindowNode *node) {
    if (!node) return 0;

    if (node->split == SPLIT_NONE) {
        return 1;
    }

    return count_leaves(node->first) + count_leaves(node->second);
}

/* Find first leaf in subtree (depth-first, left-to-right) */
static WindowNode *find_first_leaf(WindowNode *node) {
    if (!node) return NULL;

    while (node->split != SPLIT_NONE) {
        node = node->first;
    }

    return node;
}

/* Find next leaf after current in depth-first order */
static WindowNode *find_next_leaf(WindowNode *current) {
    if (!current) return NULL;

    WindowNode *node = current;

    /* Go up until we find a node where we came from the first child */
    while (node->parent) {
        WindowNode *parent = node->parent;
        if (parent->first == node && parent->second) {
            /* We came from first child - go to second subtree */
            return find_first_leaf(parent->second);
        }
        node = parent;
    }

    /* We've gone up past root - wrap to first leaf */
    return find_first_leaf(node);
}

/* Check if node is in subtree */
static int is_in_subtree(const WindowNode *node, const WindowNode *subtree) {
    if (!node || !subtree) return 0;
    if (node == subtree) return 1;

    if (subtree->split != SPLIT_NONE) {
        return is_in_subtree(node, subtree->first) ||
               is_in_subtree(node, subtree->second);
    }

    return 0;
}

/* Layout a subtree recursively */
static void layout_recursive(WindowNode *node, int x, int y, int w, int h) {
    if (!node) return;

    node->x = x;
    node->y = y;
    node->width = w;
    node->height = h;

    if (node->split == SPLIT_NONE) {
        /* Leaf node - geometry is set */
        return;
    }

    if (node->split == SPLIT_HORIZONTAL) {
        /* Top/bottom split */
        int first_h = (int)(h * node->split_ratio);
        int second_h = h - first_h - 1;  /* -1 for separator */

        if (first_h < 1) first_h = 1;
        if (second_h < 1) second_h = 1;

        layout_recursive(node->first, x, y, w, first_h);
        layout_recursive(node->second, x, y + first_h + 1, w, second_h);
    } else {
        /* Left/right split */
        int first_w = (int)(w * node->split_ratio);
        int second_w = w - first_w - 1;  /* -1 for separator */

        if (first_w < 1) first_w = 1;
        if (second_w < 1) second_w = 1;

        layout_recursive(node->first, x, y, first_w, h);
        layout_recursive(node->second, x + first_w + 1, y, second_w, h);
    }
}

/* Equalize ratios in subtree */
static void equalize_recursive(WindowNode *node) {
    if (!node) return;

    if (node->split != SPLIT_NONE) {
        node->split_ratio = 0.5f;
        equalize_recursive(node->first);
        equalize_recursive(node->second);
    }
}

/* Helper: get center point of a node's geometry */
static void get_node_center(const WindowNode *node, int *cx, int *cy) {
    if (!node) {
        *cx = *cy = 0;
        return;
    }
    *cx = node->x + node->width / 2;
    *cy = node->y + node->height / 2;
}

/* Find best leaf node in a given direction from current */
static WindowNode *find_neighbor(WindowNode *root, WindowNode *current, int direction) {
    if (!root || !current) return NULL;

    int cur_cx, cur_cy;
    get_node_center(current, &cur_cx, &cur_cy);

    WindowNode *best = NULL;
    int best_dist = -1;

    /* Iterate all leaves and find the best one in the given direction */
    WindowNode *stack[64];
    int stack_size = 0;
    stack[stack_size++] = root;

    while (stack_size > 0) {
        WindowNode *node = stack[--stack_size];

        if (node->split != SPLIT_NONE) {
            if (node->second) stack[stack_size++] = node->second;
            if (node->first) stack[stack_size++] = node->first;
            continue;
        }

        if (node == current) continue;

        int node_cx, node_cy;
        get_node_center(node, &node_cx, &node_cy);

        /* Check if node is in the correct direction */
        int valid = 0;
        int dist = 0;

        switch (direction) {
            case 'h':  /* Left */
                if (node_cx < cur_cx) {
                    valid = 1;
                    dist = cur_cx - node_cx + abs(node_cy - cur_cy) / 2;
                }
                break;
            case 'l':  /* Right */
                if (node_cx > cur_cx) {
                    valid = 1;
                    dist = node_cx - cur_cx + abs(node_cy - cur_cy) / 2;
                }
                break;
            case 'k':  /* Up */
                if (node_cy < cur_cy) {
                    valid = 1;
                    dist = cur_cy - node_cy + abs(node_cx - cur_cx) / 2;
                }
                break;
            case 'j':  /* Down */
                if (node_cy > cur_cy) {
                    valid = 1;
                    dist = node_cy - cur_cy + abs(node_cx - cur_cx) / 2;
                }
                break;
        }

        if (valid && (best_dist < 0 || dist < best_dist)) {
            best = node;
            best_dist = dist;
        }
    }

    return best;
}

/* ======================= Window Manager Lifecycle ========================= */

WindowManager *window_manager_create(int model_id) {
    WindowManager *wm = calloc(1, sizeof(WindowManager));
    if (!wm) return NULL;

    WindowNode *root = node_create_leaf(model_id);
    if (!root) {
        free(wm);
        return NULL;
    }

    wm->root = root;
    wm->active = root;
    wm->border_style = 1;  /* ASCII borders by default */

    return wm;
}

void window_manager_destroy(WindowManager *wm) {
    if (!wm) return;

    node_free_recursive(wm->root);
    free(wm);
}

/* ======================= Window Operations ================================ */

int window_split(WindowManager *wm, SplitDirection direction) {
    if (!wm || !wm->active) return -1;
    if (direction == SPLIT_NONE) return -1;

    WindowNode *current = wm->active;

    /* Create two new leaf nodes */
    WindowNode *new_pane = node_create_leaf(current->model_id);
    WindowNode *old_pane = node_create_leaf(current->model_id);

    if (!new_pane || !old_pane) {
        free(new_pane);
        free(old_pane);
        return -1;
    }

    /* Transfer view from current to old_pane */
    old_pane->view = current->view;
    current->view = NULL;

    /* Create a cloned view for the new pane */
    new_pane->view = window_view_clone(old_pane->view);
    if (!new_pane->view && old_pane->view) {
        /* Clone failed but we had a source view - create empty view */
        new_pane->view = window_view_create();
    }

    /* Convert current node to a container */
    current->split = direction;
    current->first = new_pane;
    current->second = old_pane;
    current->split_ratio = 0.5f;

    /* Set parent pointers */
    new_pane->parent = current;
    old_pane->parent = current;

    /* Focus moves to the new pane */
    wm->active = new_pane;

    return 0;
}

int window_close(WindowManager *wm) {
    if (!wm || !wm->active) return -1;

    WindowNode *current = wm->active;

    /* Cannot close the only pane */
    if (current == wm->root && current->split == SPLIT_NONE) {
        return -1;
    }

    WindowNode *parent = current->parent;
    if (!parent) {
        /* Current is root but is a container - shouldn't happen if active is leaf */
        return -1;
    }

    /* Find sibling */
    WindowNode *sibling = (parent->first == current) ? parent->second : parent->first;
    if (!sibling) return -1;

    /* Update sibling's parent to grandparent */
    WindowNode *grandparent = parent->parent;
    sibling->parent = grandparent;

    if (grandparent) {
        /* Replace parent with sibling in grandparent */
        if (grandparent->first == parent) {
            grandparent->first = sibling;
        } else {
            grandparent->second = sibling;
        }
    } else {
        /* Parent was root - sibling becomes new root */
        wm->root = sibling;
    }

    /* Free the closed pane's view and the node itself */
    if (current->view) {
        window_view_destroy(current->view);
        current->view = NULL;
    }
    free(current);

    /* Parent is now orphaned - free it without recursion (children are reassigned) */
    parent->first = NULL;
    parent->second = NULL;
    free(parent);

    /* Move focus to first leaf in sibling subtree */
    wm->active = find_first_leaf(sibling);

    return 0;
}

int window_navigate(WindowManager *wm, int direction) {
    if (!wm || !wm->active) return -1;

    WindowNode *neighbor = find_neighbor(wm->root, wm->active, direction);
    if (!neighbor) return -1;

    wm->active = neighbor;
    return 0;
}

int window_cycle(WindowManager *wm) {
    if (!wm || !wm->active) return -1;

    int pane_count = window_count_panes(wm);
    if (pane_count <= 1) return -1;

    WindowNode *next = find_next_leaf(wm->active);
    if (next && next != wm->active) {
        wm->active = next;
        return 0;
    }

    return -1;
}

void window_equalize(WindowManager *wm) {
    if (!wm || !wm->root) return;
    equalize_recursive(wm->root);
}

/* ======================= Layout Computation =============================== */

void window_layout(WindowManager *wm, int x, int y, int width, int height) {
    if (!wm || !wm->root) return;
    layout_recursive(wm->root, x, y, width, height);
}

/* ======================= Query Functions ================================== */

int window_count_panes(WindowManager *wm) {
    if (!wm) return 0;
    return count_leaves(wm->root);
}

WindowNode *window_get_active(WindowManager *wm) {
    return wm ? wm->active : NULL;
}

int window_set_active(WindowManager *wm, WindowNode *node) {
    if (!wm || !node) return -1;
    if (node->split != SPLIT_NONE) return -1;  /* Must be a leaf */
    if (!is_in_subtree(node, wm->root)) return -1;

    wm->active = node;
    return 0;
}

WindowNode *window_find_by_model(WindowManager *wm, int model_id) {
    if (!wm || !wm->root) return NULL;

    /* DFS to find first leaf with matching model_id */
    WindowNode *stack[64];
    int stack_size = 0;
    stack[stack_size++] = wm->root;

    while (stack_size > 0) {
        WindowNode *node = stack[--stack_size];

        if (node->split == SPLIT_NONE) {
            if (node->model_id == model_id) {
                return node;
            }
        } else {
            if (node->second) stack[stack_size++] = node->second;
            if (node->first) stack[stack_size++] = node->first;
        }
    }

    return NULL;
}

void window_foreach_leaf(WindowManager *wm,
                         void (*callback)(WindowNode *node, void *userdata),
                         void *userdata) {
    if (!wm || !wm->root || !callback) return;

    /* DFS iteration */
    WindowNode *stack[64];
    int stack_size = 0;
    stack[stack_size++] = wm->root;

    while (stack_size > 0) {
        WindowNode *node = stack[--stack_size];

        if (node->split == SPLIT_NONE) {
            callback(node, userdata);
        } else {
            if (node->second) stack[stack_size++] = node->second;
            if (node->first) stack[stack_size++] = node->first;
        }
    }
}

/* ======================= Node Helpers ===================================== */

int window_node_is_leaf(const WindowNode *node) {
    return node && node->split == SPLIT_NONE;
}

WindowNode *window_node_sibling(WindowNode *node) {
    if (!node || !node->parent) return NULL;

    WindowNode *parent = node->parent;
    if (parent->first == node) return parent->second;
    if (parent->second == node) return parent->first;

    return NULL;
}

/* ======================= View Management =================================== */

EditorView *window_view_create(void) {
    EditorView *view = calloc(1, sizeof(EditorView));
    if (!view) return NULL;

    /* Initialize with defaults */
    view->cx = 0;
    view->cy = 0;
    view->rowoff = 0;
    view->coloff = 0;
    view->screenrows = 0;
    view->screencols = 0;
    view->screenrows_total = 0;
    view->sel_active = 0;
    view->syntax = NULL;
    view->line_numbers = 1;  /* Default to showing line numbers */
    view->word_wrap = 0;
    view->mode = MODE_NORMAL;
    view->cmd_length = 0;
    view->cmd_cursor_pos = 0;
    view->pending_prefix = 0;
    view->statusmsg[0] = '\0';
    view->statusmsg_time = 0;
    view->playing_line = 0;

    return view;
}

EditorView *window_view_clone(const EditorView *src) {
    if (!src) return window_view_create();

    EditorView *view = calloc(1, sizeof(EditorView));
    if (!view) return NULL;

    /* Copy cursor and viewport state */
    view->cx = src->cx;
    view->cy = src->cy;
    view->rowoff = src->rowoff;
    view->coloff = src->coloff;

    /* Copy display dimensions (will be updated by layout) */
    view->screenrows = src->screenrows;
    view->screencols = src->screencols;
    view->screenrows_total = src->screenrows_total;

    /* Copy selection state */
    view->sel_active = src->sel_active;
    view->sel_start_x = src->sel_start_x;
    view->sel_start_y = src->sel_start_y;
    view->sel_end_x = src->sel_end_x;
    view->sel_end_y = src->sel_end_y;

    /* Copy display settings */
    view->syntax = src->syntax;
    memcpy(view->colors, src->colors, sizeof(view->colors));
    view->line_numbers = src->line_numbers;
    view->word_wrap = src->word_wrap;

    /* New view starts in NORMAL mode (don't inherit modal state) */
    view->mode = MODE_NORMAL;
    view->cmd_length = 0;
    view->cmd_cursor_pos = 0;
    view->cmd_history_index = -1;
    view->pending_prefix = 0;

    /* Copy status (will be overwritten quickly) */
    memcpy(view->statusmsg, src->statusmsg, sizeof(view->statusmsg));
    view->statusmsg_time = src->statusmsg_time;

    /* Don't copy picker state - it's modal */
    memset(&view->picker, 0, sizeof(view->picker));

    /* Copy playback state */
    view->playing_line = src->playing_line;

    return view;
}

void window_view_destroy(EditorView *view) {
    if (!view) return;
    /* EditorView doesn't own any allocated memory that needs freeing */
    free(view);
}

void window_sync_view_to_ctx(editor_ctx_t *ctx, WindowNode *pane) {
    if (!ctx || !pane || !pane->view) return;

    EditorView *src = pane->view;
    EditorView *dst = &ctx->view;

    /* Sync cursor and viewport */
    dst->cx = src->cx;
    dst->cy = src->cy;
    dst->rowoff = src->rowoff;
    dst->coloff = src->coloff;

    /* Sync selection state */
    dst->sel_active = src->sel_active;
    dst->sel_start_x = src->sel_start_x;
    dst->sel_start_y = src->sel_start_y;
    dst->sel_end_x = src->sel_end_x;
    dst->sel_end_y = src->sel_end_y;

    /* Sync display settings */
    dst->syntax = src->syntax;
    memcpy(dst->colors, src->colors, sizeof(dst->colors));
    dst->line_numbers = src->line_numbers;
    dst->word_wrap = src->word_wrap;

    /* Sync mode */
    dst->mode = src->mode;

    /* Note: Don't sync screenrows/screencols - those are global */
    /* Note: Don't sync picker - it's modal and global */
}

void window_sync_view_from_ctx(editor_ctx_t *ctx, WindowNode *pane) {
    if (!ctx || !pane || !pane->view) return;

    EditorView *src = &ctx->view;
    EditorView *dst = pane->view;

    /* Sync cursor and viewport */
    dst->cx = src->cx;
    dst->cy = src->cy;
    dst->rowoff = src->rowoff;
    dst->coloff = src->coloff;

    /* Sync selection state */
    dst->sel_active = src->sel_active;
    dst->sel_start_x = src->sel_start_x;
    dst->sel_start_y = src->sel_start_y;
    dst->sel_end_x = src->sel_end_x;
    dst->sel_end_y = src->sel_end_y;

    /* Sync display settings */
    dst->syntax = src->syntax;
    memcpy(dst->colors, src->colors, sizeof(dst->colors));
    dst->line_numbers = src->line_numbers;
    dst->word_wrap = src->word_wrap;

    /* Sync mode */
    dst->mode = src->mode;
}
