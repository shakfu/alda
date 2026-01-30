/* internal/completion.h - Tab completion for linenoise
 *
 * This module provides tab completion functionality, managing completion
 * candidates and cycling through them during user input.
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

#ifndef LINENOISE_COMPLETION_H
#define LINENOISE_COMPLETION_H

#include <stddef.h>

/* Completion list structure.
 * This is exposed (not opaque) because user callbacks need to populate it. */
typedef struct completions {
    size_t len;         /* Number of completion candidates. */
    char **cvec;        /* Array of completion strings. */
} completions_t;

/* Initialize a completions structure.
 * Must be called before using the structure. */
void completions_init(completions_t *lc);

/* Add a completion candidate to the list.
 * The string is copied internally. */
void completions_add(completions_t *lc, const char *str);

/* Get a completion candidate by index.
 * Returns NULL if index is out of bounds. */
const char *completions_get(const completions_t *lc, size_t index);

/* Get the number of completion candidates. */
size_t completions_len(const completions_t *lc);

/* Free all memory associated with the completions list.
 * The completions_t structure itself is not freed. */
void completions_free(completions_t *lc);

/* Clear all completions without freeing the structure. */
void completions_clear(completions_t *lc);

#endif /* LINENOISE_COMPLETION_H */
