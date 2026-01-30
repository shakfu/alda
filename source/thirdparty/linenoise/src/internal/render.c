/* src/render.c - Line rendering implementation for linenoise
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

#include "render.h"
#include "utf8.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
/* MSVC's _vsnprintf doesn't null-terminate on overflow and returns -1
 * instead of the required size. Use _vscprintf to get the required size. */
static int render_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
    int ret;
    va_list ap_copy;

    if (size == 0) {
        return _vscprintf(fmt, ap);
    }
    /* Copy va_list first since _vsnprintf consumes it. */
    va_copy(ap_copy, ap);
    ret = _vsnprintf(buf, size, fmt, ap_copy);
    va_end(ap_copy);
    if (size > 0) buf[size - 1] = '\0';
    if (ret < 0) {
        /* Buffer too small - get required size. */
        ret = _vscprintf(fmt, ap);
    }
    return ret;
}
#define vsnprintf render_vsnprintf
#endif

#define INITIAL_BUF_SIZE 256

int render_buf_init(render_buf_t *rb, size_t initial_capacity) {
    if (rb == NULL) return -1;

    if (initial_capacity == 0) {
        initial_capacity = INITIAL_BUF_SIZE;
    }

    rb->data = malloc(initial_capacity);
    if (rb->data == NULL) return -1;

    rb->len = 0;
    rb->capacity = initial_capacity;
    rb->data[0] = '\0';

    return 0;
}

int render_buf_append(render_buf_t *rb, const char *data, size_t len) {
    size_t new_capacity;
    char *new_data;

    if (rb == NULL || data == NULL) return -1;

    /* Ensure enough capacity. */
    if (rb->len + len + 1 > rb->capacity) {
        new_capacity = rb->capacity * 2;
        while (new_capacity < rb->len + len + 1) {
            new_capacity *= 2;
        }
        new_data = realloc(rb->data, new_capacity);
        if (new_data == NULL) return -1;
        rb->data = new_data;
        rb->capacity = new_capacity;
    }

    memcpy(rb->data + rb->len, data, len);
    rb->len += len;
    rb->data[rb->len] = '\0';

    return 0;
}

int render_buf_printf(render_buf_t *rb, const char *fmt, ...) {
    va_list ap;
    char buf[256];
    int len;

    if (rb == NULL || fmt == NULL) return -1;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len < 0) return -1;
    if ((size_t)len >= sizeof(buf)) {
        /* Buffer too small - allocate dynamically. */
        char *tmp = malloc(len + 1);
        if (tmp == NULL) return -1;

        va_start(ap, fmt);
        vsnprintf(tmp, len + 1, fmt, ap);
        va_end(ap);

        int result = render_buf_append(rb, tmp, len);
        free(tmp);
        return result;
    }

    return render_buf_append(rb, buf, len);
}

void render_buf_free(render_buf_t *rb) {
    if (rb == NULL) return;
    free(rb->data);
    rb->data = NULL;
    rb->len = 0;
    rb->capacity = 0;
}

void render_buf_reset(render_buf_t *rb) {
    if (rb == NULL) return;
    rb->len = 0;
    if (rb->data != NULL) {
        rb->data[0] = '\0';
    }
}

size_t render_str_width(const char *s, size_t len) {
    return utf8_str_width(s, len);
}

int render_cursor_to_col(render_buf_t *outbuf, int col) {
    if (col == 0) {
        return render_buf_append(outbuf, "\r", 1);
    }
    return render_buf_printf(outbuf, "\r\x1b[%dC", col);
}

int render_cursor_up(render_buf_t *outbuf, int n) {
    if (n <= 0) return 0;
    return render_buf_printf(outbuf, "\x1b[%dA", n);
}

int render_cursor_down(render_buf_t *outbuf, int n) {
    if (n <= 0) return 0;
    return render_buf_printf(outbuf, "\x1b[%dB", n);
}

int render_clear_eol(render_buf_t *outbuf) {
    return render_buf_append(outbuf, "\x1b[0K", 4);
}

int render_cr(render_buf_t *outbuf) {
    return render_buf_append(outbuf, "\r", 1);
}

int render_hint(const render_state_t *state, render_buf_t *outbuf, int available_width) {
    size_t hint_len, hint_width;
    char seq[64];

    if (state == NULL || outbuf == NULL) return -1;
    if (state->hint == NULL || available_width <= 0) return 0;

    hint_len = strlen(state->hint);
    hint_width = render_str_width(state->hint, hint_len);

    /* Truncate hint to fit available width. */
    if (hint_width > (size_t)available_width) {
        size_t i = 0, w = 0;
        while (i < hint_len) {
            size_t clen = utf8_next_grapheme_len(state->hint, i, hint_len);
            int cwidth = utf8_single_char_width(state->hint + i, clen);
            if (w + cwidth > (size_t)available_width) break;
            w += cwidth;
            i += clen;
        }
        hint_len = i;
    }

    /* Set color/bold if specified. */
    if (state->hint_bold == 1 && state->hint_color == -1) {
        /* Bold only. */
        if (render_buf_printf(outbuf, "\033[1;37;49m") < 0) return -1;
    } else if (state->hint_color != -1 || state->hint_bold != 0) {
        snprintf(seq, sizeof(seq), "\033[%d;%d;49m", state->hint_bold, state->hint_color);
        if (render_buf_append(outbuf, seq, strlen(seq)) < 0) return -1;
    }

    /* Append the hint text. */
    if (render_buf_append(outbuf, state->hint, hint_len) < 0) return -1;

    /* Reset colors if we set them. */
    if (state->hint_color != -1 || state->hint_bold != 0) {
        if (render_buf_append(outbuf, "\033[0m", 4) < 0) return -1;
    }

    return 0;
}

int render_single_line(const render_state_t *state, render_buf_t *outbuf, int flags) {
    const char *buf;
    size_t len;
    size_t poscol, lencol;
    size_t pwidth;

    if (state == NULL || outbuf == NULL) return -1;

    buf = state->buf;
    len = state->buf_len;
    poscol = state->cursor_width;
    lencol = state->buf_width;
    pwidth = state->prompt_width;

    /* Scroll the buffer horizontally if cursor is past the right edge.
     * We need to trim full UTF-8 characters from the left. */
    while (pwidth + poscol >= (size_t)state->cols) {
        size_t clen = utf8_next_grapheme_len(buf, 0, len);
        int cwidth = utf8_single_char_width(buf, clen);
        buf += clen;
        len -= clen;
        poscol -= cwidth;
        lencol -= cwidth;
    }

    /* Trim from the right if the line still doesn't fit. */
    while (pwidth + lencol > (size_t)state->cols) {
        size_t clen = utf8_prev_grapheme_len(buf, len);
        int cwidth = utf8_single_char_width(buf + len - clen, clen);
        len -= clen;
        lencol -= cwidth;
    }

    /* Move cursor to left edge. */
    if (render_cr(outbuf) < 0) return -1;

    if (flags & RENDER_WRITE) {
        /* Write prompt. */
        if (render_buf_append(outbuf, state->prompt, state->prompt_len) < 0) return -1;

        /* Write buffer content. */
        if (state->mask_mode) {
            /* In mask mode, output '*' for each character. */
            size_t i = 0;
            while (i < len) {
                if (render_buf_append(outbuf, "*", 1) < 0) return -1;
                i += utf8_next_grapheme_len(buf, i, len);
            }
        } else {
            if (render_buf_append(outbuf, buf, len) < 0) return -1;
        }

        /* Show hints if any. */
        if (state->hint != NULL) {
            int available = state->cols - (pwidth + lencol);
            if (available > 0) {
                render_hint(state, outbuf, available);
            }
        }
    }

    /* Erase to end of line. */
    if (render_clear_eol(outbuf) < 0) return -1;

    if (flags & RENDER_WRITE) {
        /* Move cursor to correct position. */
        if (render_cursor_to_col(outbuf, (int)(poscol + pwidth)) < 0) return -1;
    }

    return 0;
}

int render_multi_line(const render_state_t *state, render_buf_t *outbuf, int flags,
                      int old_rows, int old_cursor_row) {
    size_t pwidth, bufwidth, poswidth;
    int rows, rpos2, col;

    if (state == NULL || outbuf == NULL) return -1;

    pwidth = state->prompt_width;
    bufwidth = state->buf_width;
    poswidth = state->cursor_width;

    /* Calculate rows needed for current content. */
    rows = (pwidth + bufwidth + state->cols - 1) / state->cols;
    if (rows == 0) rows = 1;

    /* First step: clear all lines used before. */
    if (flags & RENDER_CLEAN) {
        /* Move to the last row of old content. */
        if (old_rows - old_cursor_row > 0) {
            if (render_cursor_down(outbuf, old_rows - old_cursor_row) < 0) return -1;
        }

        /* Clear each row moving upward. */
        for (int j = 0; j < old_rows - 1; j++) {
            if (render_cr(outbuf) < 0) return -1;
            if (render_clear_eol(outbuf) < 0) return -1;
            if (render_cursor_up(outbuf, 1) < 0) return -1;
        }
    }

    if (flags & RENDER_ALL) {
        /* Clean the top line. */
        if (render_cr(outbuf) < 0) return -1;
        if (render_clear_eol(outbuf) < 0) return -1;
    }

    if (flags & RENDER_WRITE) {
        /* Write prompt. */
        if (render_buf_append(outbuf, state->prompt, state->prompt_len) < 0) return -1;

        /* Write buffer content. */
        if (state->mask_mode) {
            size_t i = 0;
            while (i < state->buf_len) {
                if (render_buf_append(outbuf, "*", 1) < 0) return -1;
                i += utf8_next_grapheme_len(state->buf, i, state->buf_len);
            }
        } else {
            if (render_buf_append(outbuf, state->buf, state->buf_len) < 0) return -1;
        }

        /* Show hints if any. */
        if (state->hint != NULL) {
            int available = state->cols - ((pwidth + bufwidth) % state->cols);
            if (available > 0 && available < state->cols) {
                render_hint(state, outbuf, available);
            }
        }

        /* Handle cursor at the very end of a line. */
        if (state->cursor_pos == state->buf_len &&
            (poswidth + pwidth) % state->cols == 0) {
            if (render_buf_append(outbuf, "\n\r", 2) < 0) return -1;
            rows++;
        }

        /* Calculate cursor row position (1-based from top of content). */
        rpos2 = (pwidth + poswidth + state->cols) / state->cols;

        /* Move cursor up to correct row. */
        if (rows - rpos2 > 0) {
            if (render_cursor_up(outbuf, rows - rpos2) < 0) return -1;
        }

        /* Set column position. */
        col = (pwidth + poswidth) % state->cols;
        if (render_cursor_to_col(outbuf, col) < 0) return -1;
    }

    return rows;
}
