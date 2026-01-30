/* linenoise.h -- VERSION 2.0
 *
 * Guerrilla line editing library against the idea that a line editing lib
 * needs to be 20,000 lines of C code.
 *
 * See linenoise.c for more information.
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2023, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LINENOISE_H
#define LINENOISE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* ===== Constants ===== */

/* Maximum line length for input buffer. */
#define LINENOISE_MAX_LINE 4096

/* Default maximum history length. */
#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 100

/* Internal escape sequence buffer size. */
#define LINENOISE_SEQ_SIZE 64

/* ===== Error Handling ===== */

/* Error codes returned by linenoise functions. */
typedef enum {
    LINENOISE_OK = 0,           /* Success */
    LINENOISE_ERR_ERRNO,        /* Error in errno (use strerror) */
    LINENOISE_ERR_NOT_TTY,      /* Not a terminal */
    LINENOISE_ERR_NOT_SUPPORTED,/* Terminal not supported */
    LINENOISE_ERR_READ,         /* Read error */
    LINENOISE_ERR_WRITE,        /* Write error */
    LINENOISE_ERR_MEMORY,       /* Memory allocation failed */
    LINENOISE_ERR_INVALID,      /* Invalid argument */
    LINENOISE_ERR_EOF,          /* End of file (Ctrl+D) */
    LINENOISE_ERR_INTERRUPTED   /* Interrupted (Ctrl+C) */
} linenoise_error_t;

/* Get the last error code for the current thread. */
linenoise_error_t linenoise_get_error(void);

/* Get a human-readable error message for an error code. */
const char *linenoise_error_string(linenoise_error_t err);

/* ===== Custom Allocator ===== */

/* Custom memory allocation function types. */
typedef void *(*linenoise_malloc_fn)(size_t size);
typedef void (*linenoise_free_fn)(void *ptr);
typedef void *(*linenoise_realloc_fn)(void *ptr, size_t size);

/* Set custom memory allocator functions.
 * Pass NULL for any function to use the default (malloc/free/realloc).
 * Must be called before any other linenoise functions. */
void linenoise_set_allocator(linenoise_malloc_fn malloc_fn,
                             linenoise_free_fn free_fn,
                             linenoise_realloc_fn realloc_fn);

/* Sentinel value returned by linenoise_edit_feed() when more input is needed. */
extern char *linenoise_edit_more;

/* Completions structure for tab-completion callback. */
typedef struct linenoise_completions {
    size_t len;
    char **cvec;
} linenoise_completions_t;

/* Callback types. */
typedef void (linenoise_completion_cb_t)(const char *buf, linenoise_completions_t *completions);
typedef char *(linenoise_hints_cb_t)(const char *buf, int *color, int *bold);
typedef void (linenoise_free_hints_cb_t)(void *hint);

/* Syntax highlighting callback.
 * Called with the current buffer contents. The callback should fill the
 * colors array with color codes for each byte position:
 *   0 = default (no color)
 *   1 = red, 2 = green, 3 = yellow, 4 = blue,
 *   5 = magenta, 6 = cyan, 7 = white
 * Add 8 to make the color bold (e.g., 9 = bold red).
 * The colors array is pre-zeroed and has the same length as buf. */
typedef void (linenoise_highlight_cb_t)(const char *buf, char *colors, size_t len);

/* Opaque context structure. Each context has independent history, callbacks,
 * and settings. Thread-safe when using separate contexts per thread. */
typedef struct linenoise_context linenoise_context_t;

/* Editing state for non-blocking API. */
typedef struct linenoise_state {
    int in_completion;
    size_t completion_idx;
    int ifd;
    int ofd;
    char *buf;
    size_t buflen;
    int buf_dynamic;    /* If true, buffer auto-grows and is owned by state */
    const char *prompt;
    size_t plen;
    size_t pos;
    size_t oldpos;
    size_t len;
    size_t cols;
    size_t oldrows;
    int oldrpos;
    int history_index;
} linenoise_state_t;

/* ===== Context Management ===== */

linenoise_context_t *linenoise_context_create(void);
void linenoise_context_destroy(linenoise_context_t *ctx);

/* ===== Configuration ===== */

void linenoise_set_multiline(linenoise_context_t *ctx, int enable);
void linenoise_set_mask_mode(linenoise_context_t *ctx, int enable);
void linenoise_set_mouse_mode(linenoise_context_t *ctx, int enable);
void linenoise_set_completion_callback(linenoise_context_t *ctx, linenoise_completion_cb_t *fn);
void linenoise_set_hints_callback(linenoise_context_t *ctx, linenoise_hints_cb_t *fn);
void linenoise_set_free_hints_callback(linenoise_context_t *ctx, linenoise_free_hints_cb_t *fn);
void linenoise_set_highlight_callback(linenoise_context_t *ctx, linenoise_highlight_cb_t *fn);

/* ===== Blocking API ===== */

char *linenoise_read(linenoise_context_t *ctx, const char *prompt);

/* ===== Non-blocking API ===== */

int linenoise_edit_start(linenoise_context_t *ctx, linenoise_state_t *state,
                         int stdin_fd, int stdout_fd,
                         char *buf, size_t buflen, const char *prompt);
int linenoise_edit_start_dynamic(linenoise_context_t *ctx, linenoise_state_t *state,
                                 int stdin_fd, int stdout_fd,
                                 size_t initial_size, const char *prompt);
char *linenoise_edit_feed(linenoise_state_t *state);
void linenoise_edit_stop(linenoise_state_t *state);
void linenoise_hide(linenoise_state_t *state);
void linenoise_show(linenoise_state_t *state);

/* ===== Word Movement ===== */

void linenoise_edit_move_word_left(linenoise_state_t *state);
void linenoise_edit_move_word_right(linenoise_state_t *state);
void linenoise_edit_delete_word_right(linenoise_state_t *state);

/* ===== Undo/Redo ===== */

void linenoise_edit_undo(linenoise_state_t *state);
void linenoise_edit_redo(linenoise_state_t *state);

/* ===== History ===== */

int linenoise_history_add(linenoise_context_t *ctx, const char *line);
int linenoise_history_set_max_len(linenoise_context_t *ctx, int len);
int linenoise_history_save(linenoise_context_t *ctx, const char *filename);
int linenoise_history_load(linenoise_context_t *ctx, const char *filename);

/* ===== Utilities ===== */

void linenoise_free(void *ptr);
void linenoise_clear_screen(linenoise_context_t *ctx);
void linenoise_add_completion(linenoise_completions_t *completions, const char *text);
void linenoise_print_key_codes(void);

#ifdef __cplusplus
}
#endif

#endif /* LINENOISE_H */
