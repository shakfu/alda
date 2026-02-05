/**
 * @file test_lang_dispatch.c
 * @brief Unit tests for language dispatch system.
 *
 * Tests the lang_dispatch module functions for registering languages
 * and dispatching by command name or file extension.
 *
 * This test compiles lang_dispatch.c directly and provides mock language
 * entries for testing, avoiding dependency on the full language infrastructure.
 */

#include "test_framework.h"
#include "lang_dispatch.h"
#include <stdio.h>
#include <string.h>

/* Global test stats required by test framework */
test_stats_t test_stats;

/*============================================================================
 * Test Helpers
 *============================================================================*/

/* Dummy REPL main for test entries */
static int dummy_repl_main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

/* Dummy play main for test entries */
static int dummy_play_main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

/* Mock language entries for testing */
static const LangDispatchEntry mock_alda = {
    .commands = {"alda", "a"},
    .command_count = 2,
    .extensions = {".alda"},
    .extension_count = 1,
    .display_name = "Alda",
    .description = "Music composition language",
    .repl_main = dummy_repl_main,
    .play_main = dummy_play_main,
};

static const LangDispatchEntry mock_joy = {
    .commands = {"joy"},
    .command_count = 1,
    .extensions = {".joy"},
    .extension_count = 1,
    .display_name = "Joy",
    .description = "Concatenative language",
    .repl_main = dummy_repl_main,
    .play_main = dummy_play_main,
};

static const LangDispatchEntry mock_tr7 = {
    .commands = {"tr7", "scheme"},
    .command_count = 2,
    .extensions = {".scm", ".ss"},
    .extension_count = 2,
    .display_name = "TR7",
    .description = "Scheme dialect",
    .repl_main = dummy_repl_main,
    .play_main = NULL,  /* No play_main to test that case */
};

/* Flag to track if init was called */
static int init_called = 0;

/* Initialize dispatch system with mock languages */
static void ensure_init(void) {
    if (!init_called) {
        /* Register mock languages for testing */
        lang_dispatch_register(&mock_alda);
        lang_dispatch_register(&mock_joy);
        lang_dispatch_register(&mock_tr7);
        init_called = 1;
    }
}

/*============================================================================
 * Registration Tests
 *============================================================================*/

TEST(register_null_returns_error) {
    /* Registering NULL should return -1 */
    int result = lang_dispatch_register(NULL);
    ASSERT_EQ(result, -1);
}

TEST(register_valid_entry_succeeds) {
    ensure_init();

    int count = 0;
    lang_dispatch_get_all(&count);

    /* We registered 3 mock languages */
    ASSERT_EQ(count, 3);
}

/*============================================================================
 * Get All Tests
 *============================================================================*/

TEST(get_all_returns_array) {
    ensure_init();

    int count = 0;
    const LangDispatchEntry **langs = lang_dispatch_get_all(&count);

    ASSERT_NOT_NULL(langs);
    ASSERT_EQ(count, 3);
}

TEST(get_all_null_count_safe) {
    ensure_init();

    /* Should not crash with NULL count pointer */
    const LangDispatchEntry **langs = lang_dispatch_get_all(NULL);
    ASSERT_NOT_NULL(langs);
}

TEST(get_all_entries_valid) {
    ensure_init();

    int count = 0;
    const LangDispatchEntry **langs = lang_dispatch_get_all(&count);

    for (int i = 0; i < count; i++) {
        ASSERT_NOT_NULL(langs[i]);
        ASSERT_NOT_NULL(langs[i]->display_name);
        ASSERT_TRUE(langs[i]->command_count > 0);
        ASSERT_TRUE(langs[i]->extension_count > 0);
    }
}

TEST(get_all_returns_correct_languages) {
    ensure_init();

    int count = 0;
    const LangDispatchEntry **langs = lang_dispatch_get_all(&count);

    /* Verify our mock languages are present */
    int found_alda = 0, found_joy = 0, found_tr7 = 0;
    for (int i = 0; i < count; i++) {
        if (strcmp(langs[i]->display_name, "Alda") == 0) found_alda = 1;
        if (strcmp(langs[i]->display_name, "Joy") == 0) found_joy = 1;
        if (strcmp(langs[i]->display_name, "TR7") == 0) found_tr7 = 1;
    }

    ASSERT_TRUE(found_alda);
    ASSERT_TRUE(found_joy);
    ASSERT_TRUE(found_tr7);
}

/*============================================================================
 * Find By Command Tests
 *============================================================================*/

TEST(find_by_command_null_safe) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_command(NULL);
    ASSERT_NULL(entry);
}

TEST(find_by_command_empty_string) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_command("");
    ASSERT_NULL(entry);
}

TEST(find_by_command_unknown) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_command("nonexistent_language_xyz");
    ASSERT_NULL(entry);
}

TEST(find_by_command_alda) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_command("alda");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->commands[0], "alda");
    ASSERT_STR_EQ(entry->display_name, "Alda");
}

TEST(find_by_command_alda_alias) {
    ensure_init();

    /* "a" is an alias for alda */
    const LangDispatchEntry *entry = lang_dispatch_find_by_command("a");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->display_name, "Alda");
}

TEST(find_by_command_joy) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_command("joy");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->commands[0], "joy");
    ASSERT_STR_EQ(entry->display_name, "Joy");
}

TEST(find_by_command_tr7) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_command("tr7");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->commands[0], "tr7");
    ASSERT_STR_EQ(entry->display_name, "TR7");
}

TEST(find_by_command_scheme_alias) {
    ensure_init();

    /* "scheme" is an alias for tr7 */
    const LangDispatchEntry *entry = lang_dispatch_find_by_command("scheme");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->display_name, "TR7");
}

TEST(find_by_command_case_sensitive) {
    ensure_init();

    /* Commands should be case-sensitive */
    const LangDispatchEntry *lower = lang_dispatch_find_by_command("alda");
    const LangDispatchEntry *upper = lang_dispatch_find_by_command("ALDA");

    ASSERT_NOT_NULL(lower);
    ASSERT_NULL(upper);
}

TEST(find_by_command_returns_correct_entry) {
    ensure_init();

    int count = 0;
    const LangDispatchEntry **langs = lang_dispatch_get_all(&count);

    for (int i = 0; i < count; i++) {
        const char *cmd = langs[i]->commands[0];
        const LangDispatchEntry *found = lang_dispatch_find_by_command(cmd);
        ASSERT_NOT_NULL(found);
        ASSERT_STR_EQ(found->display_name, langs[i]->display_name);
    }
}

/*============================================================================
 * Find By Extension Tests
 *============================================================================*/

TEST(find_by_extension_null_safe) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_extension(NULL);
    ASSERT_NULL(entry);
}

TEST(find_by_extension_empty_string) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_extension("");
    ASSERT_NULL(entry);
}

TEST(find_by_extension_unknown) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_extension("file.xyz123");
    ASSERT_NULL(entry);
}

TEST(find_by_extension_alda) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_extension("song.alda");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->commands[0], "alda");
}

TEST(find_by_extension_joy) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_extension("program.joy");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->commands[0], "joy");
}

TEST(find_by_extension_scm) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_extension("code.scm");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->commands[0], "tr7");
}

TEST(find_by_extension_ss) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_extension("code.ss");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->commands[0], "tr7");
}

TEST(find_by_extension_full_path) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_extension("/path/to/file.alda");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->commands[0], "alda");
}

TEST(find_by_extension_relative_path) {
    ensure_init();

    const LangDispatchEntry *entry = lang_dispatch_find_by_extension("./dir/song.joy");
    ASSERT_NOT_NULL(entry);
    ASSERT_STR_EQ(entry->commands[0], "joy");
}

TEST(find_by_extension_case_sensitive) {
    ensure_init();

    /* Extensions should be case-sensitive */
    const LangDispatchEntry *lower = lang_dispatch_find_by_extension("file.alda");
    const LangDispatchEntry *upper = lang_dispatch_find_by_extension("file.ALDA");

    ASSERT_NOT_NULL(lower);
    ASSERT_NULL(upper);
}

TEST(find_by_extension_not_in_middle) {
    ensure_init();

    /* Extension must be at end, not in middle of filename */
    const LangDispatchEntry *entry = lang_dispatch_find_by_extension("alda.txt");
    /* Should not find Alda language because extension is .txt not .alda */
    ASSERT_NULL(entry);
}

TEST(find_by_extension_matches_all_registered) {
    ensure_init();

    int count = 0;
    const LangDispatchEntry **langs = lang_dispatch_get_all(&count);

    for (int i = 0; i < count; i++) {
        for (int j = 0; j < langs[i]->extension_count; j++) {
            char path[64];
            snprintf(path, sizeof(path), "file%s", langs[i]->extensions[j]);
            const LangDispatchEntry *found = lang_dispatch_find_by_extension(path);
            ASSERT_NOT_NULL(found);
            ASSERT_STR_EQ(found->display_name, langs[i]->display_name);
        }
    }
}

/*============================================================================
 * Has Supported Extension Tests
 *============================================================================*/

TEST(has_supported_extension_null_safe) {
    ensure_init();

    int result = lang_dispatch_has_supported_extension(NULL);
    ASSERT_FALSE(result);
}

TEST(has_supported_extension_empty_string) {
    ensure_init();

    int result = lang_dispatch_has_supported_extension("");
    ASSERT_FALSE(result);
}

TEST(has_supported_extension_unknown) {
    ensure_init();

    int result = lang_dispatch_has_supported_extension("file.unknown123");
    ASSERT_FALSE(result);
}

TEST(has_supported_extension_alda) {
    ensure_init();

    int result = lang_dispatch_has_supported_extension("song.alda");
    ASSERT_TRUE(result);
}

TEST(has_supported_extension_joy) {
    ensure_init();

    int result = lang_dispatch_has_supported_extension("program.joy");
    ASSERT_TRUE(result);
}

TEST(has_supported_extension_scm) {
    ensure_init();

    int result = lang_dispatch_has_supported_extension("code.scm");
    ASSERT_TRUE(result);
}

TEST(has_supported_extension_ss) {
    ensure_init();

    int result = lang_dispatch_has_supported_extension("code.ss");
    ASSERT_TRUE(result);
}

TEST(has_supported_extension_returns_boolean) {
    ensure_init();

    int supported = lang_dispatch_has_supported_extension("file.alda");
    int unsupported = lang_dispatch_has_supported_extension("file.xyz");

    ASSERT_TRUE(supported == 0 || supported == 1);
    ASSERT_TRUE(unsupported == 0 || unsupported == 1);
}

/*============================================================================
 * Entry Structure Tests
 *============================================================================*/

TEST(entry_has_repl_main) {
    ensure_init();

    int count = 0;
    const LangDispatchEntry **langs = lang_dispatch_get_all(&count);

    for (int i = 0; i < count; i++) {
        /* All languages should have a REPL entry point */
        ASSERT_NOT_NULL(langs[i]->repl_main);
    }
}

TEST(entry_play_main_optional) {
    ensure_init();

    /* TR7 mock entry has no play_main - verify it's NULL */
    const LangDispatchEntry *tr7 = lang_dispatch_find_by_command("tr7");
    ASSERT_NOT_NULL(tr7);
    ASSERT_NULL(tr7->play_main);

    /* Alda mock entry has play_main - verify it's set */
    const LangDispatchEntry *alda = lang_dispatch_find_by_command("alda");
    ASSERT_NOT_NULL(alda);
    ASSERT_NOT_NULL(alda->play_main);
}

TEST(entry_commands_valid) {
    ensure_init();

    int count = 0;
    const LangDispatchEntry **langs = lang_dispatch_get_all(&count);

    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(langs[i]->command_count > 0);
        ASSERT_TRUE(langs[i]->command_count <= LANG_DISPATCH_MAX_COMMANDS);

        for (int j = 0; j < langs[i]->command_count; j++) {
            ASSERT_NOT_NULL(langs[i]->commands[j]);
            ASSERT_TRUE(strlen(langs[i]->commands[j]) > 0);
        }
    }
}

TEST(entry_extensions_valid) {
    ensure_init();

    int count = 0;
    const LangDispatchEntry **langs = lang_dispatch_get_all(&count);

    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(langs[i]->extension_count > 0);
        ASSERT_TRUE(langs[i]->extension_count <= LANG_DISPATCH_MAX_EXTENSIONS);

        for (int j = 0; j < langs[i]->extension_count; j++) {
            ASSERT_NOT_NULL(langs[i]->extensions[j]);
            /* Extensions should start with . */
            ASSERT_EQ(langs[i]->extensions[j][0], '.');
        }
    }
}

TEST(entry_display_name_valid) {
    ensure_init();

    int count = 0;
    const LangDispatchEntry **langs = lang_dispatch_get_all(&count);

    for (int i = 0; i < count; i++) {
        ASSERT_NOT_NULL(langs[i]->display_name);
        ASSERT_TRUE(strlen(langs[i]->display_name) > 0);
    }
}

TEST(entry_description_valid) {
    ensure_init();

    int count = 0;
    const LangDispatchEntry **langs = lang_dispatch_get_all(&count);

    for (int i = 0; i < count; i++) {
        /* Description should be set */
        ASSERT_NOT_NULL(langs[i]->description);
        ASSERT_TRUE(strlen(langs[i]->description) > 0);
    }
}

/*============================================================================
 * Print Help Test
 *============================================================================*/

TEST(print_help_does_not_crash) {
    ensure_init();

    /* Just verify it doesn't crash - output goes to stdout */
    lang_dispatch_print_help();
}

/*============================================================================
 * Cross-Validation Tests
 *============================================================================*/

TEST(command_and_extension_match_same_language) {
    ensure_init();

    int count = 0;
    const LangDispatchEntry **langs = lang_dispatch_get_all(&count);

    for (int i = 0; i < count; i++) {
        /* For each language, verify that its command and extensions resolve to it */
        const char *cmd = langs[i]->commands[0];
        const LangDispatchEntry *by_cmd = lang_dispatch_find_by_command(cmd);
        ASSERT_NOT_NULL(by_cmd);

        for (int j = 0; j < langs[i]->extension_count; j++) {
            char path[64];
            snprintf(path, sizeof(path), "file%s", langs[i]->extensions[j]);
            const LangDispatchEntry *by_ext = lang_dispatch_find_by_extension(path);
            ASSERT_NOT_NULL(by_ext);

            /* Both should resolve to the same language */
            ASSERT_STR_EQ(by_cmd->display_name, by_ext->display_name);
        }
    }
}

/*============================================================================
 * Main Test Runner
 *============================================================================*/

BEGIN_TEST_SUITE("Language Dispatch Tests")
    /* Registration tests */
    RUN_TEST(register_null_returns_error);
    RUN_TEST(register_valid_entry_succeeds);

    /* Get all tests */
    RUN_TEST(get_all_returns_array);
    RUN_TEST(get_all_null_count_safe);
    RUN_TEST(get_all_entries_valid);
    RUN_TEST(get_all_returns_correct_languages);

    /* Find by command tests */
    RUN_TEST(find_by_command_null_safe);
    RUN_TEST(find_by_command_empty_string);
    RUN_TEST(find_by_command_unknown);
    RUN_TEST(find_by_command_alda);
    RUN_TEST(find_by_command_alda_alias);
    RUN_TEST(find_by_command_joy);
    RUN_TEST(find_by_command_tr7);
    RUN_TEST(find_by_command_scheme_alias);
    RUN_TEST(find_by_command_case_sensitive);
    RUN_TEST(find_by_command_returns_correct_entry);

    /* Find by extension tests */
    RUN_TEST(find_by_extension_null_safe);
    RUN_TEST(find_by_extension_empty_string);
    RUN_TEST(find_by_extension_unknown);
    RUN_TEST(find_by_extension_alda);
    RUN_TEST(find_by_extension_joy);
    RUN_TEST(find_by_extension_scm);
    RUN_TEST(find_by_extension_ss);
    RUN_TEST(find_by_extension_full_path);
    RUN_TEST(find_by_extension_relative_path);
    RUN_TEST(find_by_extension_case_sensitive);
    RUN_TEST(find_by_extension_not_in_middle);
    RUN_TEST(find_by_extension_matches_all_registered);

    /* Has supported extension tests */
    RUN_TEST(has_supported_extension_null_safe);
    RUN_TEST(has_supported_extension_empty_string);
    RUN_TEST(has_supported_extension_unknown);
    RUN_TEST(has_supported_extension_alda);
    RUN_TEST(has_supported_extension_joy);
    RUN_TEST(has_supported_extension_scm);
    RUN_TEST(has_supported_extension_ss);
    RUN_TEST(has_supported_extension_returns_boolean);

    /* Entry structure tests */
    RUN_TEST(entry_has_repl_main);
    RUN_TEST(entry_play_main_optional);
    RUN_TEST(entry_commands_valid);
    RUN_TEST(entry_extensions_valid);
    RUN_TEST(entry_display_name_valid);
    RUN_TEST(entry_description_valid);

    /* Print help test */
    RUN_TEST(print_help_does_not_crash);

    /* Cross-validation tests */
    RUN_TEST(command_and_extension_match_same_language);
END_TEST_SUITE()
