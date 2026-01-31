/**
 * @file repl_helpers.h
 * @brief Shared REPL helper utilities for language modules.
 *
 * Provides common utility functions used across language REPLs to reduce
 * code duplication. These are low-level helpers that don't require
 * restructuring existing REPL implementations.
 */

#ifndef LOKI_REPL_HELPERS_H
#define LOKI_REPL_HELPERS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * String Utilities
 * ============================================================================ */

/**
 * @brief Check if string starts with prefix.
 *
 * @param str The string to check
 * @param prefix The prefix to look for
 * @return 1 if str starts with prefix, 0 otherwise
 */
int repl_starts_with(const char *str, const char *prefix);

/**
 * @brief Strip trailing newline/carriage return characters in place.
 *
 * @param line String to modify (must be writable)
 * @return New length of string after stripping
 */
size_t repl_strip_newlines(char *line);

/**
 * @brief Skip leading whitespace in a string.
 *
 * @param str String to scan
 * @return Pointer to first non-whitespace character, or end of string
 */
const char *repl_skip_whitespace(const char *str);

/**
 * @brief Trim trailing whitespace from a string in place.
 *
 * @param str String to modify (must be writable)
 * @return The input string (for chaining)
 */
char *repl_trim_trailing(char *str);

/* ============================================================================
 * History File Management
 * ============================================================================ */

/**
 * @brief Get the history file path for a language.
 *
 * Determines the appropriate history file location:
 * 1. If local .psnd/ directory exists, use .psnd/<lang>_history
 * 2. Else if ~/.psnd/ exists, use ~/.psnd/<lang>_history
 * 3. Otherwise, buf is set to empty string (no history)
 *
 * @param lang_name Language name (e.g., "joy", "tr7", "alda", "bog")
 * @param buf Output buffer for path
 * @param buf_size Size of output buffer
 * @return 1 if a valid path was found, 0 if no history location available
 */
int repl_get_history_path(const char *lang_name, char *buf, size_t buf_size);

/* ============================================================================
 * REPL Loop Helpers
 * ============================================================================ */

/**
 * @brief Read lines from piped input and process them.
 *
 * Common pattern for non-interactive (piped) REPL input handling.
 * Reads lines from stdin, strips newlines, and calls the process callback.
 *
 * @param process_fn Callback to process each line. Returns:
 *                   0 = continue (command handled)
 *                   1 = quit (exit loop)
 *                   2 = evaluate (line should be evaluated as code)
 * @param eval_fn Callback to evaluate code (called when process_fn returns 2)
 * @param ctx User context passed to callbacks
 */
void repl_pipe_loop(int (*process_fn)(void *ctx, const char *line),
                    void (*eval_fn)(void *ctx, const char *line),
                    void *ctx);

/* ============================================================================
 * Interactive REPL Loop Skeleton
 * ============================================================================ */

/**
 * @brief Language identifier for syntax highlighting in interactive REPLs.
 */
typedef enum {
    REPL_SKEL_LANG_NONE = 0,
    REPL_SKEL_LANG_ALDA,
    REPL_SKEL_LANG_JOY,
    REPL_SKEL_LANG_BOG,
    REPL_SKEL_LANG_TR7,
    REPL_SKEL_LANG_LUA
} ReplSkelLanguage;

/**
 * @brief Completion callback type for interactive REPL.
 *
 * @param prefix Current word prefix to complete
 * @param count Output: number of completions returned
 * @param user_data User-provided context
 * @return Array of completion strings (caller frees), or NULL
 */
typedef char **(*ReplSkelCompletionFn)(const char *prefix, int *count, void *user_data);

/**
 * @brief Configuration for the interactive REPL skeleton.
 */
typedef struct {
    /* Required callbacks */
    int (*process_command)(void *ctx, const char *input);  /**< Returns 0=handled, 1=quit, 2=eval */
    void (*eval_line)(void *ctx, const char *input);       /**< Evaluate code */

    /* Optional callbacks */
    void (*print_banner)(void);                            /**< Print startup banner (NULL = skip) */
    void (*on_iteration)(void);                            /**< Called each loop iteration (e.g., Link polling) */

    /* Completion support */
    ReplSkelCompletionFn completion_fn;                    /**< Tab completion callback (NULL = disabled) */
    void *completion_user_data;                            /**< User data for completion callback */

    /* Configuration */
    const char *prompt;                                    /**< REPL prompt (e.g., "joy> ") */
    const char *lang_name;                                 /**< Language name for history file (e.g., "joy") */
    ReplSkelLanguage syntax_lang;                          /**< Syntax highlighting language */

    /* Context */
    void *lang_ctx;                                        /**< Language-specific context passed to callbacks */
} ReplSkeletonConfig;

/**
 * @brief Run an interactive REPL loop using the provided configuration.
 *
 * Handles the common REPL pattern:
 * 1. Detects interactive vs piped input (falls back to repl_pipe_loop for pipes)
 * 2. Initializes line editor with syntax highlighting
 * 3. Sets up tab completion if provided
 * 4. Loads history from language-specific history file
 * 5. Enables raw mode for input
 * 6. Runs main loop: readline -> process command -> eval
 * 7. Saves history and cleans up on exit
 *
 * @param config Configuration for the REPL loop
 * @param syntax_ctx Editor context for syntax highlighting (can be NULL)
 */
void repl_skeleton_run(const ReplSkeletonConfig *config, void *syntax_ctx);

#ifdef __cplusplus
}
#endif

#endif /* LOKI_REPL_HELPERS_H */
