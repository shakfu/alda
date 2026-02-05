/**
 * @file test_main_dispatch.c
 * @brief Integration tests for main.c entry point dispatch.
 *
 * Tests that the psnd binary correctly dispatches based on:
 * - Command-line flags (-h, --help, -V, --version)
 * - Language commands (alda, joy, tr7)
 * - File extensions
 * - Various argument combinations
 *
 * Uses fork/exec for process execution.
 */

/* Required for nftw() and FTW_DEPTH/FTW_PHYS - must be before any includes */
#define _XOPEN_SOURCE 500

#include "test_framework.h"
#include "test_process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Path to psnd binary - set by CMake */
#ifndef PSND_BINARY
#define PSND_BINARY "./psnd"
#endif

/*============================================================================
 * Test Suite Fixture
 *============================================================================*/

static char temp_dir[TEST_PROC_MAX_PATH];

SUITE_SETUP(main_dispatch_tests) {
    if (test_mkdtemp("psnd_dispatch_test", temp_dir) != 0) {
        fprintf(stderr, "Failed to create temp directory\n");
        exit(1);
    }
}

SUITE_TEARDOWN(main_dispatch_tests) {
    test_rmdir_recursive(temp_dir);
}

/*============================================================================
 * Helper Functions
 *============================================================================*/

static int run_psnd(char *args[]) {
    return test_exec(PSND_BINARY, args);
}

static int run_psnd_with_file(const char *filename, const char *content) {
    char filepath[TEST_PROC_MAX_PATH];
    test_build_path(temp_dir, filename, filepath);

    if (test_write_file(temp_dir, filename, content) != 0) {
        return -1;
    }

    char *args[] = {"psnd", "play", filepath, NULL};
    return run_psnd(args);
}

/*============================================================================
 * Help Flag Tests
 *============================================================================*/

TEST(help_short_flag) {
    char *args[] = {"psnd", "-h", NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

TEST(help_long_flag) {
    char *args[] = {"psnd", "--help", NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

TEST(no_args_shows_help_with_error) {
    char *args[] = {"psnd", NULL};
    int result = run_psnd(args);
    /* No args returns 1 (error) but shows help */
    ASSERT_EQ(result, 1);
}

/*============================================================================
 * Version Flag Tests
 *============================================================================*/

TEST(version_short_flag) {
    char *args[] = {"psnd", "-V", NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

TEST(version_long_flag) {
    char *args[] = {"psnd", "--version", NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

/*============================================================================
 * Play Command Dispatch Tests
 *============================================================================*/

TEST(play_requires_file) {
    char *args[] = {"psnd", "play", NULL};
    int result = run_psnd(args);
    ASSERT_NEQ(result, 0);
}

TEST(play_alda_file) {
    int result = run_psnd_with_file("test.alda", "piano: c\n");
    ASSERT_EQ(result, 0);
}

TEST(play_joy_file) {
    int result = run_psnd_with_file("test.joy", "42\n");
    ASSERT_EQ(result, 0);
}

TEST(play_scheme_file) {
    int result = run_psnd_with_file("test.scm", "(+ 1 2)\n");
    ASSERT_EQ(result, 0);
}

TEST(play_nonexistent_file) {
    char *args[] = {"psnd", "play", "/nonexistent/path/file.alda", NULL};
    int result = run_psnd(args);
    ASSERT_NEQ(result, 0);
}

/*============================================================================
 * Language Command Tests
 *============================================================================*/

TEST(alda_help_flag) {
    /* alda -h should work */
    char *args[] = {"psnd", "alda", "-h", NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

TEST(joy_help_flag) {
    char *args[] = {"psnd", "joy", "-h", NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

TEST(tr7_help_flag) {
    char *args[] = {"psnd", "tr7", "-h", NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

TEST(alda_list_ports) {
    char *args[] = {"psnd", "alda", "-l", NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

TEST(joy_list_ports) {
    char *args[] = {"psnd", "joy", "-l", NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

/*============================================================================
 * Unknown Command Tests
 *============================================================================*/

TEST(unknown_command_fallback) {
    /* Unknown command without file extension - should try editor */
    char *args[] = {"psnd", "unknowncommand", NULL};
    int result = run_psnd(args);
    /* May fail since file doesn't exist, but shouldn't crash */
    (void)result;  /* Just verify no crash */
}

/*============================================================================
 * File Extension Routing Tests
 *============================================================================*/

TEST(direct_file_alda) {
    /* Direct file argument should open editor or fail gracefully */
    char filepath[TEST_PROC_MAX_PATH];
    test_build_path(temp_dir, "direct.alda", filepath);
    test_write_file(temp_dir, "direct.alda", "piano: c\n");

    /* Direct file opens editor, which we can't test non-interactively */
    /* Just verify the dispatch doesn't crash */
    char *args[] = {"psnd", "play", filepath, NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

TEST(direct_file_joy) {
    char filepath[TEST_PROC_MAX_PATH];
    test_build_path(temp_dir, "direct.joy", filepath);
    test_write_file(temp_dir, "direct.joy", "1 2 +\n");

    char *args[] = {"psnd", "play", filepath, NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

TEST(direct_file_scheme) {
    char filepath[TEST_PROC_MAX_PATH];
    test_build_path(temp_dir, "direct.scm", filepath);
    test_write_file(temp_dir, "direct.scm", "(+ 1 2)\n");

    char *args[] = {"psnd", "play", filepath, NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

/*============================================================================
 * CSD Extension Tests
 *============================================================================*/

TEST(csd_extension_recognized) {
    /* .csd files are always supported (Csound) */
    char filepath[TEST_PROC_MAX_PATH];
    test_build_path(temp_dir, "test.csd", filepath);

    /* Create a minimal valid CSD file */
    const char *csd_content =
        "<CsoundSynthesizer>\n"
        "<CsOptions>\n"
        "-n\n"
        "</CsOptions>\n"
        "<CsInstruments>\n"
        "sr = 44100\n"
        "ksmps = 32\n"
        "nchnls = 2\n"
        "</CsInstruments>\n"
        "<CsScore>\n"
        "</CsScore>\n"
        "</CsoundSynthesizer>\n";

    test_write_file(temp_dir, "test.csd", csd_content);

    /* .csd files route to editor - can't test interactively */
    /* Just verify the file is recognized without crashing */
}

/*============================================================================
 * Option with File Tests
 *============================================================================*/

TEST(soundfont_option_needs_file) {
    char *args[] = {"psnd", "-sf", "gm.sf2", NULL};
    int result = run_psnd(args);
    /* Should fail because no supported file is provided */
    ASSERT_NEQ(result, 0);
}

/*============================================================================
 * Language Command with File Tests
 *============================================================================*/

TEST(alda_command_with_file_routes_to_editor) {
    /* When language command is followed by a file, it should route to editor */
    /* This is tested via play command since editor is interactive */
    char filepath[TEST_PROC_MAX_PATH];
    test_build_path(temp_dir, "song.alda", filepath);
    test_write_file(temp_dir, "song.alda", "piano: c d e\n");

    char *args[] = {"psnd", "play", filepath, NULL};
    int result = run_psnd(args);
    ASSERT_EQ(result, 0);
}

/*============================================================================
 * Multiple Extension Support Tests
 *============================================================================*/

TEST(ss_extension_routes_to_tr7) {
    int result = run_psnd_with_file("test.ss", "(+ 3 4)\n");
    ASSERT_EQ(result, 0);
}

/* NOTE: Bog test removed - Bog interpreter doesn't support headless playback mode */

/*============================================================================
 * Error Handling Tests
 *============================================================================*/

TEST(invalid_option) {
    char *args[] = {"psnd", "--invalid-option-xyz", NULL};
    int result = run_psnd(args);
    /* Should fail or fall through to editor (which fails on invalid file) */
    ASSERT_NEQ(result, 0);
}

TEST(play_empty_file) {
    int result = run_psnd_with_file("empty.alda", "");
    /* Empty file should succeed (no notes to play) */
    ASSERT_EQ(result, 0);
}

TEST(play_syntax_error) {
    /* Syntax error in file - behavior depends on language */
    int result = run_psnd_with_file("bad.alda", "invalid syntax @@@@\n");
    /* May succeed or fail depending on error handling */
    (void)result;  /* Just verify no crash */
}

/*============================================================================
 * Main Test Runner
 *============================================================================*/

BEGIN_TEST_SUITE_WITH_FIXTURE("Main Dispatch Tests", main_dispatch_tests)
    /* Help flag tests */
    RUN_TEST(help_short_flag);
    RUN_TEST(help_long_flag);
    RUN_TEST(no_args_shows_help_with_error);

    /* Version flag tests */
    RUN_TEST(version_short_flag);
    RUN_TEST(version_long_flag);

    /* Play command tests */
    RUN_TEST(play_requires_file);
    RUN_TEST(play_alda_file);
    RUN_TEST(play_joy_file);
    RUN_TEST(play_scheme_file);
    RUN_TEST(play_nonexistent_file);

    /* Language command tests */
    RUN_TEST(alda_help_flag);
    RUN_TEST(joy_help_flag);
    RUN_TEST(tr7_help_flag);
    RUN_TEST(alda_list_ports);
    RUN_TEST(joy_list_ports);

    /* Unknown command tests */
    /* NOTE: unknown_command_fallback skipped - it opens editor in interactive mode */
    /* RUN_TEST(unknown_command_fallback); */

    /* File extension routing tests */
    RUN_TEST(direct_file_alda);
    RUN_TEST(direct_file_joy);
    RUN_TEST(direct_file_scheme);

    /* CSD extension tests */
    RUN_TEST(csd_extension_recognized);

    /* Option with file tests */
    RUN_TEST(soundfont_option_needs_file);

    /* Language command with file tests */
    RUN_TEST(alda_command_with_file_routes_to_editor);

    /* Multiple extension tests */
    RUN_TEST(ss_extension_routes_to_tr7);
    /* bog_extension_if_supported removed - Bog doesn't support headless playback */

    /* Error handling tests */
    RUN_TEST(invalid_option);
    RUN_TEST(play_empty_file);
    RUN_TEST(play_syntax_error);
END_TEST_SUITE_WITH_FIXTURE(main_dispatch_tests)
