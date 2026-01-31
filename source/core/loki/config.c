/* config.c - TOML-based configuration system
 *
 * Implements declarative configuration loading from TOML files.
 * This replaces automatic Lua loading with opt-in Lua scripting.
 */

#include "config.h"
#include "psnd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <toml.h>

/* Global config instance */
static loki_config_t g_config;
static bool g_config_initialized = false;

/* Initialize config with sensible defaults */
void config_init(loki_config_t *config) {
    if (!config) return;

    memset(config, 0, sizeof(loki_config_t));

    /* Editor defaults */
    strncpy(config->theme, "nord", CONFIG_MAX_THEME_NAME - 1);
    config->line_numbers = true;
    config->tab_width = 4;
    config->lua_enabled = false;  /* Opt-in: Lua disabled by default */

    /* Audio defaults */
    strncpy(config->backend, "tsf", CONFIG_MAX_BACKEND_NAME - 1);
    config->soundfont[0] = '\0';

    /* Link defaults */
    config->link_enabled = false;
    config->tempo = 120;

    /* Default keybindings */
    config->keybinding_count = 0;

    /* Add default keybindings */
    struct {
        const char *key;
        const char *cmd;
    } defaults[] = {
        {"ctrl-s", "save"},
        {"ctrl-q", "quit"},
        {"ctrl-e", "eval_line"},
        {"ctrl-p", "play_file"},
        {"ctrl-g", "stop"},
        {"ctrl-f", "find"},
        {"ctrl-l", "lua_repl"},
        {"ctrl-t", "new_buffer"},
        {NULL, NULL}
    };

    for (int i = 0; defaults[i].key && config->keybinding_count < CONFIG_MAX_KEYBINDINGS; i++) {
        strncpy(config->keybindings[config->keybinding_count].key,
                defaults[i].key, CONFIG_MAX_KEY_NAME - 1);
        strncpy(config->keybindings[config->keybinding_count].command,
                defaults[i].cmd, CONFIG_MAX_COMMAND_NAME - 1);
        config->keybinding_count++;
    }

    config->loaded = false;
    config->loaded_from[0] = '\0';
}

/* Helper: safely copy string from TOML datum */
static void copy_toml_string(char *dest, size_t dest_size, toml_datum_t datum) {
    if (datum.ok && datum.u.s) {
        strncpy(dest, datum.u.s, dest_size - 1);
        dest[dest_size - 1] = '\0';
        free(datum.u.s);
    }
}

/* Parse [editor] section */
static void parse_editor_section(loki_config_t *config, toml_table_t *editor) {
    if (!editor) return;

    /* theme */
    toml_datum_t theme = toml_string_in(editor, "theme");
    copy_toml_string(config->theme, CONFIG_MAX_THEME_NAME, theme);

    /* line_numbers */
    toml_datum_t line_nums = toml_bool_in(editor, "line_numbers");
    if (line_nums.ok) {
        config->line_numbers = line_nums.u.b;
    }

    /* tab_width */
    toml_datum_t tab_w = toml_int_in(editor, "tab_width");
    if (tab_w.ok && tab_w.u.i >= 1 && tab_w.u.i <= 16) {
        config->tab_width = (int)tab_w.u.i;
    }

    /* [editor.lua] subsection */
    toml_table_t *lua_section = toml_table_in(editor, "lua");
    if (lua_section) {
        toml_datum_t enabled = toml_bool_in(lua_section, "enabled");
        if (enabled.ok) {
            config->lua_enabled = enabled.u.b;
        }
    }
}

/* Parse [audio] section */
static void parse_audio_section(loki_config_t *config, toml_table_t *audio) {
    if (!audio) return;

    /* backend */
    toml_datum_t backend = toml_string_in(audio, "backend");
    copy_toml_string(config->backend, CONFIG_MAX_BACKEND_NAME, backend);

    /* soundfont */
    toml_datum_t sf = toml_string_in(audio, "soundfont");
    copy_toml_string(config->soundfont, CONFIG_MAX_SOUNDFONT_PATH, sf);
}

/* Parse [link] section */
static void parse_link_section(loki_config_t *config, toml_table_t *link) {
    if (!link) return;

    /* enabled */
    toml_datum_t enabled = toml_bool_in(link, "enabled");
    if (enabled.ok) {
        config->link_enabled = enabled.u.b;
    }

    /* tempo */
    toml_datum_t tempo = toml_int_in(link, "tempo");
    if (tempo.ok && tempo.u.i >= 20 && tempo.u.i <= 999) {
        config->tempo = (int)tempo.u.i;
    }
}

/* Parse [keybindings] section */
static void parse_keybindings_section(loki_config_t *config, toml_table_t *keybindings) {
    if (!keybindings) return;

    /* Clear existing keybindings and load from config */
    config->keybinding_count = 0;

    /* Iterate over all keys in the keybindings table */
    int i = 0;
    const char *key;
    while ((key = toml_key_in(keybindings, i++)) != NULL) {
        if (config->keybinding_count >= CONFIG_MAX_KEYBINDINGS) break;

        toml_datum_t command = toml_string_in(keybindings, key);
        if (command.ok && command.u.s) {
            strncpy(config->keybindings[config->keybinding_count].key,
                    key, CONFIG_MAX_KEY_NAME - 1);
            strncpy(config->keybindings[config->keybinding_count].command,
                    command.u.s, CONFIG_MAX_COMMAND_NAME - 1);
            config->keybinding_count++;
            free(command.u.s);
        }
    }
}

/* Load configuration from a specific file */
int config_load_file(loki_config_t *config, const char *path) {
    if (!config || !path) return -1;

    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    char errbuf[256];
    toml_table_t *root = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!root) {
        fprintf(stderr, "Config parse error (%s): %s\n", path, errbuf);
        return -1;
    }

    /* Parse sections */
    toml_table_t *editor = toml_table_in(root, "editor");
    parse_editor_section(config, editor);

    toml_table_t *audio = toml_table_in(root, "audio");
    parse_audio_section(config, audio);

    toml_table_t *link = toml_table_in(root, "link");
    parse_link_section(config, link);

    toml_table_t *keybindings = toml_table_in(root, "keybindings");
    parse_keybindings_section(config, keybindings);

    toml_free(root);

    config->loaded = true;
    strncpy(config->loaded_from, path, sizeof(config->loaded_from) - 1);

    return 0;
}

/* Load configuration - tries project-local, then user home */
int config_load(loki_config_t *config, const char *project_root) {
    if (!config) return -1;

    char path[1024];

    /* Try project-local config first */
    if (project_root && project_root[0] != '\0') {
        snprintf(path, sizeof(path), "%s/" PSND_CONFIG_DIR "/config.toml", project_root);
    } else {
        snprintf(path, sizeof(path), PSND_CONFIG_DIR "/config.toml");
    }

    if (access(path, R_OK) == 0) {
        if (config_load_file(config, path) == 0) {
            return 0;
        }
    }

    /* Try user home directory */
    const char *home = getenv("HOME");
    if (home && home[0] != '\0') {
        snprintf(path, sizeof(path), "%s/" PSND_CONFIG_DIR "/config.toml", home);
        if (access(path, R_OK) == 0) {
            if (config_load_file(config, path) == 0) {
                return 0;
            }
        }
    }

    /* No config file found - defaults already set */
    return 1;  /* Not an error, just no config found */
}

/* Get keybinding command for a given key */
const char *config_get_keybinding(const loki_config_t *config, const char *key) {
    if (!config || !key) return NULL;

    for (int i = 0; i < config->keybinding_count; i++) {
        if (strcmp(config->keybindings[i].key, key) == 0) {
            return config->keybindings[i].command;
        }
    }
    return NULL;
}

/* Get global config instance */
loki_config_t *config_global(void) {
    if (!g_config_initialized) {
        config_init(&g_config);
        config_load(&g_config, NULL);
        g_config_initialized = true;
    }
    return &g_config;
}

/* Free any dynamically allocated config resources */
void config_free(loki_config_t *config) {
    if (!config) return;
    /* Currently no dynamic allocations in config struct */
    (void)config;
}
