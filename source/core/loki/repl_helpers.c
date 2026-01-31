/**
 * @file repl_helpers.c
 * @brief Implementation of shared REPL helper utilities.
 */

#include "repl_helpers.h"
#include "repl.h"
#include "internal.h"
#include "shared/repl_commands.h"
#include "psnd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>

/* Maximum input line length for piped input */
#ifndef MAX_INPUT_LENGTH
#define MAX_INPUT_LENGTH 4096
#endif

/* ============================================================================
 * String Utilities
 * ============================================================================ */

int repl_starts_with(const char *str, const char *prefix) {
    if (!str || !prefix) return 0;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

size_t repl_strip_newlines(char *line) {
    if (!line) return 0;

    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = '\0';
    }
    return len;
}

const char *repl_skip_whitespace(const char *str) {
    if (!str) return str;
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    return str;
}

char *repl_trim_trailing(char *str) {
    if (!str) return str;

    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
    return str;
}

/* ============================================================================
 * History File Management
 * ============================================================================ */

int repl_get_history_path(const char *lang_name, char *buf, size_t buf_size) {
    if (!lang_name || !buf || buf_size == 0) {
        if (buf && buf_size > 0) buf[0] = '\0';
        return 0;
    }

    struct stat st;

    /* Check for local .psnd/ directory first */
    if (stat(PSND_CONFIG_DIR, &st) == 0 && S_ISDIR(st.st_mode)) {
        snprintf(buf, buf_size, "%s/%s_history", PSND_CONFIG_DIR, lang_name);
        return 1;
    }

    /* Fall back to ~/.psnd/ if it exists */
    const char *home = getenv("HOME");
    if (home) {
        char global_psnd[512];
        snprintf(global_psnd, sizeof(global_psnd), "%s/%s", home, PSND_CONFIG_DIR);
        if (stat(global_psnd, &st) == 0 && S_ISDIR(st.st_mode)) {
            snprintf(buf, buf_size, "%s/%s_history", global_psnd, lang_name);
            return 1;
        }
    }

    /* No history location available */
    buf[0] = '\0';
    return 0;
}

/* ============================================================================
 * REPL Loop Helpers
 * ============================================================================ */

void repl_pipe_loop(int (*process_fn)(void *ctx, const char *line),
                    void (*eval_fn)(void *ctx, const char *line),
                    void *ctx) {
    if (!process_fn) return;

    char line[MAX_INPUT_LENGTH];

    while (fgets(line, sizeof(line), stdin) != NULL) {
        /* Strip trailing newline */
        size_t len = repl_strip_newlines(line);

        /* Skip empty lines */
        if (len == 0) continue;

        /* Process command */
        int result = process_fn(ctx, line);

        if (result == 1) {
            /* Quit */
            break;
        }

        if (result == 0) {
            /* Command handled */
            continue;
        }

        /* result == 2: evaluate as code */
        if (eval_fn) {
            eval_fn(ctx, line);
        }

        fflush(stdout);
    }
}

/* ============================================================================
 * Interactive REPL Loop Skeleton
 * ============================================================================ */

/* Map ReplSkelLanguage to ReplLanguage for linenoise */
#ifdef LOKI_USE_LINENOISE
static ReplLanguage skel_lang_to_repl_lang(ReplSkelLanguage lang) {
    switch (lang) {
        case REPL_SKEL_LANG_ALDA: return REPL_LANG_ALDA;
        case REPL_SKEL_LANG_JOY:  return REPL_LANG_JOY;
        case REPL_SKEL_LANG_TR7:  return REPL_LANG_SCHEME;
        case REPL_SKEL_LANG_LUA:  return REPL_LANG_LUA;
        case REPL_SKEL_LANG_BOG:
        case REPL_SKEL_LANG_NONE:
        default:
            return REPL_LANG_NONE;
    }
}
#endif

/* Adapter for pipe loop: wraps config->process_command */
typedef struct {
    const ReplSkeletonConfig *config;
} PipeLoopAdapter;

static int pipe_process_adapter(void *ctx, const char *line) {
    PipeLoopAdapter *adapter = (PipeLoopAdapter *)ctx;
    return adapter->config->process_command(adapter->config->lang_ctx, line);
}

static void pipe_eval_adapter(void *ctx, const char *line) {
    PipeLoopAdapter *adapter = (PipeLoopAdapter *)ctx;
    if (adapter->config->eval_line) {
        adapter->config->eval_line(adapter->config->lang_ctx, line);
    }
}

void repl_skeleton_run(const ReplSkeletonConfig *config, void *syntax_ctx) {
    if (!config || !config->process_command) {
        return;
    }

    /* Use non-interactive mode for piped input */
    if (!isatty(STDIN_FILENO)) {
        PipeLoopAdapter adapter = { .config = config };
        repl_pipe_loop(pipe_process_adapter, pipe_eval_adapter, &adapter);
        return;
    }

    /* Interactive mode */
    ReplLineEditor ed;
    char *input;
    char history_path[512] = {0};

    /* Initialize line editor with language-specific syntax highlighting */
#ifdef LOKI_USE_LINENOISE
    if (config->syntax_lang != REPL_SKEL_LANG_NONE) {
        repl_editor_init_with_language(&ed, skel_lang_to_repl_lang(config->syntax_lang));
    } else {
        repl_editor_init(&ed);
    }
#else
    repl_editor_init(&ed);
#endif

    /* Set up tab completion if provided */
    if (config->completion_fn) {
        repl_set_completion(&ed, config->completion_fn, config->completion_user_data);
    }

    /* Load history */
    if (config->lang_name) {
        if (repl_get_history_path(config->lang_name, history_path, sizeof(history_path))) {
            repl_history_load(&ed, history_path);
        }
    }

    /* Print startup banner */
    if (config->print_banner) {
        config->print_banner();
    }

    /* Enable raw mode for syntax-highlighted input */
    repl_enable_raw_mode();

    /* Main REPL loop */
    const char *prompt = config->prompt ? config->prompt : "> ";

    while (1) {
        input = repl_readline((editor_ctx_t *)syntax_ctx, &ed, prompt);

        if (input == NULL) {
            /* EOF - exit cleanly */
            break;
        }

        if (input[0] == '\0') {
            /* Empty input - poll callbacks and continue */
            if (config->on_iteration) {
                config->on_iteration();
            }
            continue;
        }

        repl_add_history(&ed, input);

        /* Process command */
        int result = config->process_command(config->lang_ctx, input);

        if (result == 1) {
            /* Quit */
            break;
        }

        if (result == 0) {
            /* Command handled - poll callbacks and continue */
            if (config->on_iteration) {
                config->on_iteration();
            }
            continue;
        }

        /* result == 2: evaluate as code */
        if (config->eval_line) {
            config->eval_line(config->lang_ctx, input);
        }

        /* Poll callbacks after evaluation */
        if (config->on_iteration) {
            config->on_iteration();
        }
    }

    /* Disable raw mode before exit */
    repl_disable_raw_mode();

    /* Save history */
    if (history_path[0]) {
        repl_history_save(&ed, history_path);
    }

    repl_editor_cleanup(&ed);
}
