/**
 * @file test_config.c
 * @brief Tests for configuration system (config.c, keybind.c, theme_toml.c).
 *
 * Tests verify:
 * - Config initialization and defaults
 * - TOML config file parsing
 * - Keybinding lookup and registration
 * - Key code to string mapping
 * - Theme list enumeration
 * - NULL handling throughout
 */

#include "test_framework.h"
#include "loki/config.h"
#include "loki/keybind.h"
#include "loki/theme_toml.h"
#include "loki/internal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#include <process.h>
#define access _access
#define F_OK 0
#else
#include <unistd.h>
#endif

test_stats_t test_stats;

/* ============================================================================
 * Config Init Tests
 * ============================================================================ */

TEST(config_init_sets_defaults) {
    loki_config_t config;
    config_init(&config);

    /* Editor defaults */
    ASSERT_STR_EQ(config.theme, "nord");
    ASSERT_TRUE(config.line_numbers);
    ASSERT_EQ(config.tab_width, 4);
    ASSERT_FALSE(config.lua_enabled);

    /* Audio defaults */
    ASSERT_STR_EQ(config.backend, "tsf");
    ASSERT_EQ(config.soundfont[0], '\0');

    /* Link defaults */
    ASSERT_FALSE(config.link_enabled);
    ASSERT_EQ(config.tempo, 120);

    /* Default keybindings should be present */
    ASSERT_TRUE(config.keybinding_count > 0);

    /* Metadata */
    ASSERT_FALSE(config.loaded);
    ASSERT_EQ(config.loaded_from[0], '\0');
}

TEST(config_init_null_safe) {
    /* Should not crash */
    config_init(NULL);
}

TEST(config_init_default_keybindings) {
    loki_config_t config;
    config_init(&config);

    /* Check that default keybindings exist */
    const char *save_cmd = config_get_keybinding(&config, "ctrl-s");
    ASSERT_NOT_NULL(save_cmd);
    ASSERT_STR_EQ(save_cmd, "save");

    const char *quit_cmd = config_get_keybinding(&config, "ctrl-q");
    ASSERT_NOT_NULL(quit_cmd);
    ASSERT_STR_EQ(quit_cmd, "quit");

    const char *eval_cmd = config_get_keybinding(&config, "ctrl-e");
    ASSERT_NOT_NULL(eval_cmd);
    ASSERT_STR_EQ(eval_cmd, "eval_line");
}

/* ============================================================================
 * Config Keybinding Lookup Tests
 * ============================================================================ */

TEST(config_get_keybinding_found) {
    loki_config_t config;
    config_init(&config);

    const char *cmd = config_get_keybinding(&config, "ctrl-s");
    ASSERT_NOT_NULL(cmd);
    ASSERT_STR_EQ(cmd, "save");
}

TEST(config_get_keybinding_not_found) {
    loki_config_t config;
    config_init(&config);

    const char *cmd = config_get_keybinding(&config, "ctrl-z");
    ASSERT_NULL(cmd);
}

TEST(config_get_keybinding_null_config) {
    const char *cmd = config_get_keybinding(NULL, "ctrl-s");
    ASSERT_NULL(cmd);
}

TEST(config_get_keybinding_null_key) {
    loki_config_t config;
    config_init(&config);

    const char *cmd = config_get_keybinding(&config, NULL);
    ASSERT_NULL(cmd);
}

TEST(config_get_keybinding_empty_key) {
    loki_config_t config;
    config_init(&config);

    const char *cmd = config_get_keybinding(&config, "");
    ASSERT_NULL(cmd);
}

/* ============================================================================
 * Config Free Tests
 * ============================================================================ */

TEST(config_free_null_safe) {
    /* Should not crash */
    config_free(NULL);
}

TEST(config_free_after_init) {
    loki_config_t config;
    config_init(&config);
    /* Should not crash */
    config_free(&config);
}

/* ============================================================================
 * Config Load File Tests (with temp files)
 * ============================================================================ */

/* Helper: create a temp TOML file with given content */
static char *create_temp_toml(const char *content) {
    static char path[256];
#ifdef _WIN32
    char *tmp = getenv("TEMP");
    if (!tmp) tmp = ".";
    snprintf(path, sizeof(path), "%s\\test_config_%d.toml", tmp, (int)getpid());
#else
    snprintf(path, sizeof(path), "/tmp/test_config_%d.toml", (int)getpid());
#endif

    FILE *f = fopen(path, "w");
    if (!f) return NULL;
    fputs(content, f);
    fclose(f);
    return path;
}

static void remove_temp_file(const char *path) {
    if (path) remove(path);
}

TEST(config_load_file_editor_section) {
    const char *toml =
        "[editor]\n"
        "theme = \"monokai\"\n"
        "line_numbers = false\n"
        "tab_width = 2\n"
        "\n"
        "[editor.lua]\n"
        "enabled = true\n";

    char *path = create_temp_toml(toml);
    ASSERT_NOT_NULL(path);

    loki_config_t config;
    config_init(&config);

    int result = config_load_file(&config, path);
    ASSERT_EQ(result, 0);

    ASSERT_STR_EQ(config.theme, "monokai");
    ASSERT_FALSE(config.line_numbers);
    ASSERT_EQ(config.tab_width, 2);
    ASSERT_TRUE(config.lua_enabled);
    ASSERT_TRUE(config.loaded);

    remove_temp_file(path);
}

TEST(config_load_file_audio_section) {
    const char *toml =
        "[audio]\n"
        "backend = \"fluid\"\n"
        "soundfont = \"/path/to/gm.sf2\"\n";

    char *path = create_temp_toml(toml);
    ASSERT_NOT_NULL(path);

    loki_config_t config;
    config_init(&config);

    int result = config_load_file(&config, path);
    ASSERT_EQ(result, 0);

    ASSERT_STR_EQ(config.backend, "fluid");
    ASSERT_STR_EQ(config.soundfont, "/path/to/gm.sf2");

    remove_temp_file(path);
}

TEST(config_load_file_link_section) {
    const char *toml =
        "[link]\n"
        "enabled = true\n"
        "tempo = 140\n";

    char *path = create_temp_toml(toml);
    ASSERT_NOT_NULL(path);

    loki_config_t config;
    config_init(&config);

    int result = config_load_file(&config, path);
    ASSERT_EQ(result, 0);

    ASSERT_TRUE(config.link_enabled);
    ASSERT_EQ(config.tempo, 140);

    remove_temp_file(path);
}

TEST(config_load_file_keybindings_section) {
    const char *toml =
        "[keybindings]\n"
        "ctrl-s = \"custom_save\"\n"
        "ctrl-r = \"reload\"\n";

    char *path = create_temp_toml(toml);
    ASSERT_NOT_NULL(path);

    loki_config_t config;
    config_init(&config);

    int result = config_load_file(&config, path);
    ASSERT_EQ(result, 0);

    /* Keybindings from file should replace defaults */
    ASSERT_EQ(config.keybinding_count, 2);

    const char *save_cmd = config_get_keybinding(&config, "ctrl-s");
    ASSERT_NOT_NULL(save_cmd);
    ASSERT_STR_EQ(save_cmd, "custom_save");

    const char *reload_cmd = config_get_keybinding(&config, "ctrl-r");
    ASSERT_NOT_NULL(reload_cmd);
    ASSERT_STR_EQ(reload_cmd, "reload");

    remove_temp_file(path);
}

TEST(config_load_file_tab_width_clamped) {
    /* Tab width should be clamped to 1-16 */
    const char *toml =
        "[editor]\n"
        "tab_width = 100\n";  /* Too large */

    char *path = create_temp_toml(toml);
    ASSERT_NOT_NULL(path);

    loki_config_t config;
    config_init(&config);

    config_load_file(&config, path);

    /* Should keep default (4) since 100 is out of range */
    ASSERT_EQ(config.tab_width, 4);

    remove_temp_file(path);
}

TEST(config_load_file_tempo_clamped) {
    /* Tempo should be clamped to 20-999 */
    const char *toml =
        "[link]\n"
        "tempo = 5\n";  /* Too low */

    char *path = create_temp_toml(toml);
    ASSERT_NOT_NULL(path);

    loki_config_t config;
    config_init(&config);

    config_load_file(&config, path);

    /* Should keep default (120) since 5 is out of range */
    ASSERT_EQ(config.tempo, 120);

    remove_temp_file(path);
}

TEST(config_load_file_nonexistent) {
    loki_config_t config;
    config_init(&config);

    int result = config_load_file(&config, "/nonexistent/path/config.toml");
    ASSERT_EQ(result, -1);
    ASSERT_FALSE(config.loaded);
}

TEST(config_load_file_null_config) {
    int result = config_load_file(NULL, "/some/path.toml");
    ASSERT_EQ(result, -1);
}

TEST(config_load_file_null_path) {
    loki_config_t config;
    config_init(&config);

    int result = config_load_file(&config, NULL);
    ASSERT_EQ(result, -1);
}

TEST(config_load_file_empty_file) {
    char *path = create_temp_toml("");
    ASSERT_NOT_NULL(path);

    loki_config_t config;
    config_init(&config);

    /* Empty file should parse successfully (no sections) */
    int result = config_load_file(&config, path);
    ASSERT_EQ(result, 0);
    ASSERT_TRUE(config.loaded);

    /* Defaults should remain */
    ASSERT_STR_EQ(config.theme, "nord");

    remove_temp_file(path);
}

TEST(config_load_file_partial_sections) {
    /* Config with only some sections should preserve defaults for others */
    const char *toml =
        "[editor]\n"
        "theme = \"dracula\"\n";

    char *path = create_temp_toml(toml);
    ASSERT_NOT_NULL(path);

    loki_config_t config;
    config_init(&config);

    int result = config_load_file(&config, path);
    ASSERT_EQ(result, 0);

    /* Editor section was parsed */
    ASSERT_STR_EQ(config.theme, "dracula");

    /* Other sections should keep defaults */
    ASSERT_STR_EQ(config.backend, "tsf");
    ASSERT_EQ(config.tempo, 120);

    remove_temp_file(path);
}

/* ============================================================================
 * Keybind Key-to-String Tests
 * ============================================================================ */

TEST(keybind_key_to_string_ctrl_s) {
    const char *str = keybind_key_to_string(CTRL_S);
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "ctrl-s");
}

TEST(keybind_key_to_string_ctrl_q) {
    const char *str = keybind_key_to_string(CTRL_Q);
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "ctrl-q");
}

TEST(keybind_key_to_string_ctrl_e) {
    const char *str = keybind_key_to_string(CTRL_E);
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "ctrl-e");
}

TEST(keybind_key_to_string_ctrl_p) {
    const char *str = keybind_key_to_string(CTRL_P);
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "ctrl-p");
}

TEST(keybind_key_to_string_ctrl_g) {
    const char *str = keybind_key_to_string(CTRL_G);
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "ctrl-g");
}

TEST(keybind_key_to_string_ctrl_f) {
    const char *str = keybind_key_to_string(CTRL_F);
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "ctrl-f");
}

TEST(keybind_key_to_string_ctrl_l) {
    const char *str = keybind_key_to_string(CTRL_L);
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "ctrl-l");
}

TEST(keybind_key_to_string_ctrl_t) {
    const char *str = keybind_key_to_string(CTRL_T);
    ASSERT_NOT_NULL(str);
    ASSERT_STR_EQ(str, "ctrl-t");
}

TEST(keybind_key_to_string_unknown) {
    /* Unknown key code should return NULL */
    const char *str = keybind_key_to_string(999);
    ASSERT_NULL(str);
}

TEST(keybind_key_to_string_zero) {
    const char *str = keybind_key_to_string(0);
    ASSERT_NULL(str);
}

/* ============================================================================
 * Keybind Register Tests
 * ============================================================================ */

/* Dummy handler for registration tests */
static int dummy_handler(editor_ctx_t *ctx, int fd) {
    (void)ctx;
    (void)fd;
    return 1;
}

TEST(keybind_register_new_command) {
    /* Note: keybind_init() is called automatically by keybind_execute() */
    int result = keybind_register("test_command", dummy_handler);
    ASSERT_EQ(result, 0);
}

TEST(keybind_register_update_existing) {
    /* Registering same command name should update handler */
    int result = keybind_register("test_command2", dummy_handler);
    ASSERT_EQ(result, 0);

    result = keybind_register("test_command2", dummy_handler);
    ASSERT_EQ(result, 0);  /* Should succeed (update) */
}

/* ============================================================================
 * Keybind Execute Tests
 * ============================================================================ */

TEST(keybind_execute_null_command) {
    int result = keybind_execute(NULL, 0, NULL);
    ASSERT_EQ(result, 0);
}

TEST(keybind_execute_unknown_command) {
    int result = keybind_execute(NULL, 0, "nonexistent_command_xyz");
    ASSERT_EQ(result, 0);
}

/* ============================================================================
 * Theme TOML Tests
 * ============================================================================ */

TEST(theme_toml_list_returns_array) {
    /* Should return non-NULL array (may be empty) */
    const char **themes = theme_toml_list();
    ASSERT_NOT_NULL(themes);
    /* themes[0] may be NULL if no themes found, that's OK */
}

TEST(theme_toml_scan_no_crash) {
    /* Should not crash even if no themes directory exists */
    theme_toml_scan();
}

TEST(theme_toml_load_null_ctx) {
    int result = theme_toml_load(NULL, "nord");
    ASSERT_EQ(result, 0);
}

TEST(theme_toml_load_null_name) {
    /* Create a minimal context for testing */
    editor_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    int result = theme_toml_load(&ctx, NULL);
    ASSERT_EQ(result, 0);
}

TEST(theme_toml_load_empty_name) {
    editor_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    int result = theme_toml_load(&ctx, "");
    ASSERT_EQ(result, 0);
}

TEST(theme_toml_load_nonexistent) {
    editor_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Should return 0 (not found) for nonexistent theme */
    int result = theme_toml_load(&ctx, "nonexistent_theme_xyz_123");
    ASSERT_EQ(result, 0);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

BEGIN_TEST_SUITE("Config System Tests")

    /* Config init */
    RUN_TEST(config_init_sets_defaults);
    RUN_TEST(config_init_null_safe);
    RUN_TEST(config_init_default_keybindings);

    /* Config keybinding lookup */
    RUN_TEST(config_get_keybinding_found);
    RUN_TEST(config_get_keybinding_not_found);
    RUN_TEST(config_get_keybinding_null_config);
    RUN_TEST(config_get_keybinding_null_key);
    RUN_TEST(config_get_keybinding_empty_key);

    /* Config free */
    RUN_TEST(config_free_null_safe);
    RUN_TEST(config_free_after_init);

    /* Config load file */
    RUN_TEST(config_load_file_editor_section);
    RUN_TEST(config_load_file_audio_section);
    RUN_TEST(config_load_file_link_section);
    RUN_TEST(config_load_file_keybindings_section);
    RUN_TEST(config_load_file_tab_width_clamped);
    RUN_TEST(config_load_file_tempo_clamped);
    RUN_TEST(config_load_file_nonexistent);
    RUN_TEST(config_load_file_null_config);
    RUN_TEST(config_load_file_null_path);
    RUN_TEST(config_load_file_empty_file);
    RUN_TEST(config_load_file_partial_sections);

    /* Keybind key-to-string */
    RUN_TEST(keybind_key_to_string_ctrl_s);
    RUN_TEST(keybind_key_to_string_ctrl_q);
    RUN_TEST(keybind_key_to_string_ctrl_e);
    RUN_TEST(keybind_key_to_string_ctrl_p);
    RUN_TEST(keybind_key_to_string_ctrl_g);
    RUN_TEST(keybind_key_to_string_ctrl_f);
    RUN_TEST(keybind_key_to_string_ctrl_l);
    RUN_TEST(keybind_key_to_string_ctrl_t);
    RUN_TEST(keybind_key_to_string_unknown);
    RUN_TEST(keybind_key_to_string_zero);

    /* Keybind register */
    RUN_TEST(keybind_register_new_command);
    RUN_TEST(keybind_register_update_existing);

    /* Keybind execute */
    RUN_TEST(keybind_execute_null_command);
    RUN_TEST(keybind_execute_unknown_command);

    /* Theme TOML */
    RUN_TEST(theme_toml_list_returns_array);
    RUN_TEST(theme_toml_scan_no_crash);
    RUN_TEST(theme_toml_load_null_ctx);
    RUN_TEST(theme_toml_load_null_name);
    RUN_TEST(theme_toml_load_empty_name);
    RUN_TEST(theme_toml_load_nonexistent);

END_TEST_SUITE()
