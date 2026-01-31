/* picker.h - Modal picker interface
 *
 * Provides a full-screen selection UI for choosing items from a list.
 * Used for theme selection, preset selection, file picking, etc.
 *
 * The picker operates as a state machine integrated with the modal editor:
 * 1. picker_open() enters MODE_PICKER and stores picker state
 * 2. picker_handle_key() processes navigation keys
 * 3. On selection/cancel, callback is invoked and mode is restored
 *
 * Key bindings:
 *   j / DOWN  - Move selection down
 *   k / UP    - Move selection up
 *   ENTER     - Select current item
 *   ESC / q   - Cancel without selection
 *   CTRL_D    - Page down
 *   CTRL_U    - Page up
 */

#ifndef LOKI_PICKER_H
#define LOKI_PICKER_H

#include "loki/core.h"

/* Callback type for picker selection
 * ctx: Editor context
 * index: Selected item index (0-based), or -1 if cancelled
 * data: User-provided data pointer */
typedef void (*picker_callback_t)(editor_ctx_t *ctx, int index, void *data);

/* Open the picker with a list of items
 * ctx: Editor context
 * title: Title displayed at top of picker
 * items: Array of item strings (not copied, must remain valid)
 * count: Number of items
 * on_select: Callback invoked when item is selected (index >= 0) or cancelled (index == -1)
 * user_data: Arbitrary data passed to callback
 * Returns: 0 on success, -1 on failure */
int picker_open(editor_ctx_t *ctx, const char *title,
                const char **items, int count,
                picker_callback_t on_select,
                void *user_data);

/* Close the picker and restore previous mode
 * ctx: Editor context
 * Invokes on_cancel callback if set, otherwise on_select with index -1 */
void picker_close(editor_ctx_t *ctx);

/* Handle a keypress in picker mode
 * ctx: Editor context
 * key: Key code
 * Returns: 1 if picker is still active, 0 if picker closed */
int picker_handle_key(editor_ctx_t *ctx, int key);

/* Blocking convenience wrapper - waits for selection
 * ctx: Editor context
 * fd: File descriptor for terminal input
 * title: Picker title
 * items: Array of item strings
 * count: Number of items
 * Returns: Selected index (0-based), or -1 if cancelled */
int picker_select_blocking(editor_ctx_t *ctx, int fd,
                           const char *title, const char **items, int count);

/* Get currently selected index
 * ctx: Editor context
 * Returns: Selected index, or -1 if picker not active */
int picker_get_selected(editor_ctx_t *ctx);

/* Set selected index
 * ctx: Editor context
 * index: New selected index (clamped to valid range) */
void picker_set_selected(editor_ctx_t *ctx, int index);

#endif /* LOKI_PICKER_H */
