/* config.h - TOML-based configuration system
 *
 * This module provides declarative configuration via TOML files.
 * Lua scripting becomes opt-in through editor.lua.enabled setting.
 *
 * Configuration loading order:
 * 1. .psnd/config.toml (project-local)
 * 2. ~/.psnd/config.toml (user default)
 * 3. Built-in defaults
 */

#ifndef LOKI_CONFIG_H
#define LOKI_CONFIG_H

#include <stdbool.h>

/* Maximum sizes */
#define CONFIG_MAX_THEME_NAME     64
#define CONFIG_MAX_BACKEND_NAME   32
#define CONFIG_MAX_SOUNDFONT_PATH 512
#define CONFIG_MAX_KEYBINDINGS    64
#define CONFIG_MAX_KEY_NAME       32
#define CONFIG_MAX_COMMAND_NAME   64

/* Keybinding entry */
typedef struct {
    char key[CONFIG_MAX_KEY_NAME];        /* e.g., "ctrl-s", "ctrl-e" */
    char command[CONFIG_MAX_COMMAND_NAME]; /* e.g., "save", "eval_line" */
} config_keybinding_t;

/* Configuration structure */
typedef struct {
    /* [editor] section */
    char theme[CONFIG_MAX_THEME_NAME];
    bool line_numbers;
    int tab_width;
    bool lua_enabled;      /* editor.lua.enabled - opt-in Lua scripting */

    /* [audio] section */
    char backend[CONFIG_MAX_BACKEND_NAME];  /* "tsf", "fluid", "csound", "midi" */
    char soundfont[CONFIG_MAX_SOUNDFONT_PATH];

    /* [link] section */
    bool link_enabled;
    int tempo;

    /* [keybindings] section */
    config_keybinding_t keybindings[CONFIG_MAX_KEYBINDINGS];
    int keybinding_count;

    /* Metadata */
    bool loaded;           /* true if a config file was loaded */
    char loaded_from[512]; /* Path to the config file that was loaded */
} loki_config_t;

/* Initialize config with defaults */
void config_init(loki_config_t *config);

/* Load configuration from TOML file
 * Tries project-local first, then user home directory.
 * Returns 0 on success, -1 on error (config still has defaults).
 */
int config_load(loki_config_t *config, const char *project_root);

/* Load configuration from a specific file
 * Returns 0 on success, -1 on error.
 */
int config_load_file(loki_config_t *config, const char *path);

/* Get keybinding command for a given key
 * Returns command string or NULL if no binding found.
 */
const char *config_get_keybinding(const loki_config_t *config, const char *key);

/* Global config instance (initialized on first use) */
loki_config_t *config_global(void);

/* Free any dynamically allocated config resources */
void config_free(loki_config_t *config);

#endif /* LOKI_CONFIG_H */
