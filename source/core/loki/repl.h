/**
 * @file repl.h
 * @brief Common REPL infrastructure - types and functions shared by all language REPLs.
 *
 * When LOKI_USE_LINENOISE is defined, provides linenoise-based line editing
 * with tree-sitter syntax highlighting. Otherwise, provides a fallback
 * implementation using the editor's terminal handling.
 */

#ifndef LOKI_REPL_H
#define LOKI_REPL_H

#include "internal.h"

#ifdef LOKI_USE_LINENOISE
#include <linenoise.h>
#include "repl_linenoise.h"
#endif

#define REPL_MAX_INPUT_LENGTH 1024
#define REPL_HISTORY_MAX 64

/* Control key definitions */
#ifndef CTRL_A
#define CTRL_A 1
#endif
#ifndef CTRL_K
#define CTRL_K 11
#endif

#define REPL_COMPLETIONS_MAX 256

/**
 * Completion callback type.
 *
 * @param prefix  The word prefix to complete (may be empty string)
 * @param count   Output: number of completions returned
 * @param user_data  User-provided context (e.g., JoyContext*)
 * @return Array of completion strings (caller must free array and strings),
 *         or NULL if no completions. Strings must be strdup'd.
 */
typedef char **(*ReplCompletionCallback)(const char *prefix, int *count, void *user_data);

/* Completion state for TAB cycling */
typedef struct {
    char **completions;     /* Current completion candidates */
    int count;              /* Number of candidates */
    int index;              /* Current index (-1 = none selected) */
    int word_start;         /* Start position of word being completed */
    int word_len;           /* Original length of word being completed */
} ReplCompletionState;

/* Line editor state for syntax-highlighted REPL input */
typedef struct {
    char buf[REPL_MAX_INPUT_LENGTH];      /* Input buffer */
    int len;                         /* Current length */
    int pos;                         /* Cursor position */
    char *history[REPL_HISTORY_MAX]; /* History entries */
    int history_len;                 /* Number of history entries */
    int history_idx;                 /* Current history index (-1 = current input) */
    char saved_buf[REPL_MAX_INPUT_LENGTH];/* Saved current input when browsing history */
    int saved_len;                   /* Saved length */
    unsigned char hl[REPL_MAX_INPUT_LENGTH]; /* Highlight types per character */

    /* Completion support - word list (standard mechanism) */
    const char **completion_words;         /* NULL-terminated array of completion words */
    int completion_word_count;             /* Number of words in array */

    /* Completion support - callback (advanced, optional) */
    ReplCompletionCallback completion_cb;  /* Custom callback (overrides word list) */
    void *completion_user_data;            /* User data for callback */

    /* Completion state */
    ReplCompletionState completion;        /* Current completion state */

#ifdef LOKI_USE_LINENOISE
    /* Linenoise context for this editor */
    linenoise_context_t *ln_ctx;
    ReplLanguage ln_lang;            /* Language for syntax highlighting */
#endif
} ReplLineEditor;

/* Initialize line editor state (defaults to REPL_LANG_LUA) */
void repl_editor_init(ReplLineEditor *ed);

/* Initialize line editor with specific language for syntax highlighting */
#ifdef LOKI_USE_LINENOISE
void repl_editor_init_with_language(ReplLineEditor *ed, ReplLanguage lang);
#endif

/* Cleanup line editor (free history) */
void repl_editor_cleanup(ReplLineEditor *ed);

/* Add a line to history */
void repl_add_history(ReplLineEditor *ed, const char *line);

/* Load history from file (one entry per line) */
int repl_history_load(ReplLineEditor *ed, const char *filepath);

/* Save history to file (one entry per line) */
int repl_history_save(ReplLineEditor *ed, const char *filepath);

/* Enable/disable terminal raw mode for REPL */
int repl_enable_raw_mode(void);
void repl_disable_raw_mode(void);

/* Read a line with syntax highlighting */
char *repl_readline(editor_ctx_t *syntax_ctx, ReplLineEditor *ed, const char *prompt);

/* Highlight the current line buffer */
void repl_highlight_line(editor_ctx_t *syntax_ctx, ReplLineEditor *ed);

/* Render the current line with highlighting */
void repl_render_line(editor_ctx_t *syntax_ctx, ReplLineEditor *ed, const char *prompt);

/* Set completion words for TAB completion (standard mechanism) */
void repl_set_completion_words(ReplLineEditor *ed, const char **words, int count);

/* Set completion callback for TAB completion (advanced, overrides word list) */
void repl_set_completion(ReplLineEditor *ed, ReplCompletionCallback cb, void *user_data);

/* Clear completion state (call when input changes non-TAB) */
void repl_completion_clear(ReplLineEditor *ed);

#endif /* LOKI_REPL_H */
