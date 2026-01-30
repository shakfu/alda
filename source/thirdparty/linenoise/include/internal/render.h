/* internal/render.h - Line rendering for linenoise
 *
 * This module provides pure rendering functions that generate escape
 * sequences for displaying the editing line, without performing any I/O.
 * This separation enables unit testing of rendering logic.
 *
 * Copyright (c) 2010-2023, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LINENOISE_RENDER_H
#define LINENOISE_RENDER_H

#include <stddef.h>

/* Render state containing all information needed to render a line. */
typedef struct render_state {
    const char *prompt;     /* Prompt string to display. */
    size_t prompt_len;      /* Length of prompt in bytes. */
    size_t prompt_width;    /* Display width of prompt. */

    const char *buf;        /* Edit buffer content. */
    size_t buf_len;         /* Length of buffer in bytes. */
    size_t buf_width;       /* Display width of buffer. */
    size_t cursor_pos;      /* Cursor byte position in buffer. */
    size_t cursor_width;    /* Display width up to cursor. */

    int cols;               /* Terminal width in columns. */
    int rows;               /* For multiline: rows currently used. */
    int cursor_row;         /* For multiline: cursor row position. */

    int mask_mode;          /* If non-zero, show '*' instead of characters. */
    int multiline;          /* If non-zero, use multiline mode. */

    /* Hint display (optional). */
    const char *hint;       /* Hint text to display after input. */
    int hint_color;         /* Color code for hint (-1 for default). */
    int hint_bold;          /* If non-zero, display hint in bold. */
} render_state_t;

/* Append buffer for building output. */
typedef struct render_buf {
    char *data;             /* Buffer data. */
    size_t len;             /* Current length. */
    size_t capacity;        /* Allocated capacity. */
} render_buf_t;

/* Initialize a render buffer.
 * Returns 0 on success, -1 on allocation failure. */
int render_buf_init(render_buf_t *rb, size_t initial_capacity);

/* Append data to a render buffer.
 * Returns 0 on success, -1 on allocation failure. */
int render_buf_append(render_buf_t *rb, const char *data, size_t len);

/* Append a formatted string to a render buffer.
 * Returns 0 on success, -1 on failure. */
int render_buf_printf(render_buf_t *rb, const char *fmt, ...);

/* Free a render buffer's data. */
void render_buf_free(render_buf_t *rb);

/* Reset a render buffer to empty (keeps allocated memory). */
void render_buf_reset(render_buf_t *rb);

/* Render flags. */
#define RENDER_CLEAN  (1 << 0)  /* Clear previous content. */
#define RENDER_WRITE  (1 << 1)  /* Write new content. */
#define RENDER_ALL    (RENDER_CLEAN | RENDER_WRITE)

/* Render a single-line editing display.
 * Generates escape sequences to display the line with prompt and cursor.
 *
 * Parameters:
 *   state - render state containing all display information
 *   outbuf - render buffer to append output to
 *   flags - RENDER_* flags controlling what to render
 *
 * Returns:
 *   0 on success, -1 on error.
 */
int render_single_line(const render_state_t *state, render_buf_t *outbuf, int flags);

/* Render a multi-line editing display.
 * Generates escape sequences to display the line with word wrap.
 *
 * Parameters:
 *   state - render state containing all display information
 *   outbuf - render buffer to append output to
 *   flags - RENDER_* flags controlling what to render
 *   old_rows - number of rows used in previous render (for clearing)
 *   old_cursor_row - cursor row in previous render
 *
 * Returns:
 *   Number of rows used, or -1 on error.
 */
int render_multi_line(const render_state_t *state, render_buf_t *outbuf, int flags,
                      int old_rows, int old_cursor_row);

/* Render a hint after the current input.
 * Appends the hint with appropriate colors/styling.
 *
 * Parameters:
 *   state - render state with hint information
 *   outbuf - render buffer to append output to
 *   available_width - columns available for hint display
 *
 * Returns:
 *   0 on success, -1 on error.
 */
int render_hint(const render_state_t *state, render_buf_t *outbuf, int available_width);

/* Calculate display width of a UTF-8 string.
 * This is a convenience wrapper around the UTF-8 module. */
size_t render_str_width(const char *s, size_t len);

/* Generate escape sequence to move cursor to column.
 * Column is 0-based. */
int render_cursor_to_col(render_buf_t *outbuf, int col);

/* Generate escape sequence to move cursor up n rows. */
int render_cursor_up(render_buf_t *outbuf, int n);

/* Generate escape sequence to move cursor down n rows. */
int render_cursor_down(render_buf_t *outbuf, int n);

/* Generate escape sequence to clear to end of line. */
int render_clear_eol(render_buf_t *outbuf);

/* Generate escape sequence for carriage return. */
int render_cr(render_buf_t *outbuf);

#endif /* LINENOISE_RENDER_H */
