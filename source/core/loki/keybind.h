/* keybind.h - Command dispatcher for configurable keybindings
 *
 * Maps key codes to commands via TOML configuration.
 * Commands are string names that map to handler functions.
 */

#ifndef LOKI_KEYBIND_H
#define LOKI_KEYBIND_H

/* Forward declaration */
struct editor_ctx;
typedef struct editor_ctx editor_ctx_t;

/* Command handler function signature
 * fd is the terminal file descriptor (0 for non-terminal sources)
 * Returns 1 if command was handled, 0 if not
 */
typedef int (*keybind_handler_t)(editor_ctx_t *ctx, int fd);

/* Convert a key code (CTRL_S, etc.) to a key string ("ctrl-s")
 * Returns static string or NULL if key code not recognized.
 */
const char *keybind_key_to_string(int keycode);

/* Look up command for a key code using config
 * Returns command name or NULL if no binding found.
 */
const char *keybind_get_command(int keycode);

/* Execute a command by name
 * Returns 1 if command was executed, 0 if command not found.
 */
int keybind_execute(editor_ctx_t *ctx, int fd, const char *command);

/* Try to handle a key via configured keybindings
 * Returns 1 if key was handled, 0 if not (use default handling).
 */
int keybind_try_handle(editor_ctx_t *ctx, int fd, int keycode);

/* Register a custom command handler
 * Returns 0 on success, -1 if registry full.
 */
int keybind_register(const char *command, keybind_handler_t handler);

/* Initialize built-in command handlers */
void keybind_init(void);

#endif /* LOKI_KEYBIND_H */
