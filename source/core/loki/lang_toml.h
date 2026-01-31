/* lang_toml.h - TOML language definition loading
 *
 * Loads language definitions from .psnd/languages/ directory.
 * These provide syntax highlighting rules without requiring Lua.
 */

#ifndef LOKI_LANG_TOML_H
#define LOKI_LANG_TOML_H

/* Load all TOML language definitions from .psnd/languages/
 * Searches both project-local and home directory.
 * Call this once at startup after config loading.
 *
 * Returns: number of languages loaded
 */
int lang_toml_load_all(void);

/* Load a single TOML language definition
 * Returns: 1 on success, 0 on failure
 */
int lang_toml_load(const char *path);

#endif /* LOKI_LANG_TOML_H */
