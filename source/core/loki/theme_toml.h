/* theme_toml.h - TOML theme loading
 *
 * Loads theme definitions from .psnd/themes directory.
 * Falls back to built-in C themes if TOML theme not found.
 */

#ifndef LOKI_THEME_TOML_H
#define LOKI_THEME_TOML_H

/* Forward declaration - matches loki/core.h */
struct editor_ctx;
typedef struct editor_ctx editor_ctx_t;

/* Load a TOML theme by name
 * Looks for .psnd/themes/<name>.toml (project-local or home directory).
 * Applies colors directly to the editor context.
 *
 * Returns: 1 on success, 0 if theme not found or parse error
 */
int theme_toml_load(editor_ctx_t *ctx, const char *name);

/* List available TOML themes
 * Returns a NULL-terminated array of theme names.
 * Caller should NOT free the returned array (static storage).
 * Call theme_toml_scan() first to populate the list.
 */
const char **theme_toml_list(void);

/* Scan for available TOML themes in .psnd/themes directories.
 * Call this once at startup to populate the theme list.
 */
void theme_toml_scan(void);

#endif /* LOKI_THEME_TOML_H */
