/* window.h - Binary tree tiling window manager
 *
 * This module implements vim-style split windows using a binary tree layout.
 * Each tree node is either a split container (horizontal/vertical) or a leaf
 * pane. Panes have independent EditorView instances but can share EditorModel
 * (same document in multiple views).
 *
 * Key bindings (handled via Ctrl-W prefix in modal.c):
 *   Ctrl-W s - Split horizontally (top/bottom)
 *   Ctrl-W v - Split vertically (left/right)
 *   Ctrl-W h/j/k/l - Navigate between panes
 *   Ctrl-W c - Close current pane
 *   Ctrl-W w - Cycle to next pane
 *   Ctrl-W = - Equalize split ratios
 */

#ifndef LOKI_WINDOW_H
#define LOKI_WINDOW_H

#include <stddef.h>

/* Forward declarations */
struct editor_ctx;
struct EditorView;

/* ======================= Split Direction ================================== */

typedef enum SplitDirection {
    SPLIT_NONE = 0,       /* Leaf node (no children) */
    SPLIT_HORIZONTAL,     /* Top/bottom split (first=top, second=bottom) */
    SPLIT_VERTICAL        /* Left/right split (first=left, second=right) */
} SplitDirection;

/* ======================= Window Node ====================================== */

/**
 * WindowNode - Binary tree node for window layout.
 *
 * Interior nodes (split != SPLIT_NONE):
 *   - Have two children (first, second)
 *   - split_ratio determines division (0.5 = equal)
 *   - view and model_id are unused
 *
 * Leaf nodes (split == SPLIT_NONE):
 *   - Have no children
 *   - view points to the pane's EditorView
 *   - model_id references the buffer in buffer_manager
 */
typedef struct WindowNode {
    SplitDirection split;           /* SPLIT_NONE for leaf, H/V for container */
    struct WindowNode *first;       /* Left/top child (containers only) */
    struct WindowNode *second;      /* Right/bottom child (containers only) */
    float split_ratio;              /* 0.0-1.0, where 0.5 = equal split */

    struct EditorView *view;        /* Leaf only: view state (owned) */
    int model_id;                   /* Leaf only: buffer_manager buffer ID */

    /* Computed geometry (updated by window_layout()) */
    int x, y;                       /* Top-left corner in screen coordinates */
    int width, height;              /* Dimensions in characters */

    struct WindowNode *parent;      /* For tree navigation (NULL for root) */
} WindowNode;

/* ======================= Window Manager =================================== */

/**
 * WindowManager - Manages the window tree for an editor context.
 *
 * Owns the window tree and tracks the active (focused) pane.
 * All pane operations go through this struct.
 */
typedef struct WindowManager {
    WindowNode *root;               /* Root of window tree */
    WindowNode *active;             /* Currently focused pane (always a leaf) */
    int border_style;               /* 0=none, 1=ascii, 2=unicode */
} WindowManager;

/* ======================= Window Manager Lifecycle ========================= */

/**
 * Create and initialize a window manager.
 * Creates a single root pane with the given buffer ID.
 *
 * @param model_id  Buffer ID for the initial pane
 * @return New WindowManager, or NULL on failure
 */
WindowManager *window_manager_create(int model_id);

/**
 * Destroy a window manager and free all nodes.
 * Does NOT free the EditorViews - those are owned elsewhere.
 *
 * @param wm  WindowManager to destroy (may be NULL)
 */
void window_manager_destroy(WindowManager *wm);

/* ======================= Window Operations ================================ */

/**
 * Split the active pane.
 *
 * The active pane becomes a container node with two children:
 *   - First child: new pane (for SPLIT_VERTICAL: left, for SPLIT_HORIZONTAL: top)
 *   - Second child: original pane content
 *
 * The new pane shares the same model_id as the original (same document).
 * Focus moves to the new pane.
 *
 * @param wm         WindowManager
 * @param direction  SPLIT_HORIZONTAL or SPLIT_VERTICAL
 * @return 0 on success, -1 on failure
 */
int window_split(WindowManager *wm, SplitDirection direction);

/**
 * Close the active pane.
 *
 * If this is the only pane (root), does nothing and returns -1.
 * Otherwise, removes the pane and promotes the sibling to take its place.
 * Focus moves to the remaining sibling.
 *
 * @param wm  WindowManager
 * @return 0 on success, -1 if cannot close (only pane)
 */
int window_close(WindowManager *wm);

/**
 * Navigate to an adjacent pane.
 *
 * @param wm         WindowManager
 * @param direction  'h' (left), 'j' (down), 'k' (up), 'l' (right)
 * @return 0 on success, -1 if no pane in that direction
 */
int window_navigate(WindowManager *wm, int direction);

/**
 * Cycle to the next pane (depth-first order).
 * Wraps around to the first pane after the last.
 *
 * @param wm  WindowManager
 * @return 0 on success, -1 if only one pane
 */
int window_cycle(WindowManager *wm);

/**
 * Equalize all split ratios to 0.5.
 *
 * @param wm  WindowManager
 */
void window_equalize(WindowManager *wm);

/* ======================= Layout Computation =============================== */

/**
 * Recompute geometry for all nodes in the tree.
 * Must be called after splits, closes, or window resizes.
 *
 * @param wm      WindowManager
 * @param x       Left edge of available area
 * @param y       Top edge of available area
 * @param width   Width of available area
 * @param height  Height of available area
 */
void window_layout(WindowManager *wm, int x, int y, int width, int height);

/* ======================= Query Functions ================================== */

/**
 * Count the number of leaf panes.
 *
 * @param wm  WindowManager
 * @return Number of panes (>= 1)
 */
int window_count_panes(WindowManager *wm);

/**
 * Get the active (focused) pane.
 *
 * @param wm  WindowManager
 * @return Active WindowNode (always a leaf), or NULL if no manager
 */
WindowNode *window_get_active(WindowManager *wm);

/**
 * Set the active pane by node pointer.
 * The node must be a leaf in this tree.
 *
 * @param wm    WindowManager
 * @param node  Leaf node to make active
 * @return 0 on success, -1 if node is not a leaf or not in tree
 */
int window_set_active(WindowManager *wm, WindowNode *node);

/**
 * Find a leaf node by its buffer ID.
 * Returns the first leaf with matching model_id.
 *
 * @param wm        WindowManager
 * @param model_id  Buffer ID to find
 * @return WindowNode if found, NULL otherwise
 */
WindowNode *window_find_by_model(WindowManager *wm, int model_id);

/**
 * Iterate over all leaf nodes in depth-first order.
 * Calls callback for each leaf.
 *
 * @param wm        WindowManager
 * @param callback  Function to call for each leaf
 * @param userdata  Passed to callback
 */
void window_foreach_leaf(WindowManager *wm,
                         void (*callback)(WindowNode *node, void *userdata),
                         void *userdata);

/* ======================= Node Helpers ===================================== */

/**
 * Check if a node is a leaf (has no children).
 *
 * @param node  WindowNode to check
 * @return 1 if leaf, 0 if container
 */
int window_node_is_leaf(const WindowNode *node);

/**
 * Get the sibling of a node (other child of parent).
 *
 * @param node  WindowNode
 * @return Sibling node, or NULL if no parent or no sibling
 */
WindowNode *window_node_sibling(WindowNode *node);

/* ======================= View Management =================================== */

/**
 * Create a new EditorView with default values.
 * The view is allocated on the heap and must be freed with window_view_destroy().
 *
 * @return New EditorView, or NULL on allocation failure
 */
struct EditorView *window_view_create(void);

/**
 * Clone an EditorView, copying cursor, scroll, selection, and display settings.
 * Modal state (mode, command buffer) is NOT copied - new view starts in NORMAL mode.
 *
 * @param src  Source view to clone from
 * @return New EditorView with copied state, or NULL on failure
 */
struct EditorView *window_view_clone(const struct EditorView *src);

/**
 * Destroy an EditorView and free its memory.
 *
 * @param view  View to destroy (may be NULL)
 */
void window_view_destroy(struct EditorView *view);

/**
 * Sync view state from a pane's view to the main context view.
 * Call this when switching to a pane to make its state active.
 *
 * @param ctx   Editor context (destination)
 * @param pane  Window pane with source view
 */
void window_sync_view_to_ctx(struct editor_ctx *ctx, WindowNode *pane);

/**
 * Sync view state from main context view to a pane's view.
 * Call this before switching away from a pane to save its state.
 *
 * @param ctx   Editor context (source)
 * @param pane  Window pane with destination view
 */
void window_sync_view_from_ctx(struct editor_ctx *ctx, WindowNode *pane);

#endif /* LOKI_WINDOW_H */
