/* internal/history.h - History management for linenoise
 *
 * This module provides a reusable history buffer that stores command history
 * entries. It supports adding entries, navigating through history, and
 * persisting history to/from files.
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

#ifndef LINENOISE_HISTORY_H
#define LINENOISE_HISTORY_H

#include <stddef.h>

/* Default maximum history length. */
#define HISTORY_DEFAULT_MAX_LEN 100

/* Opaque history structure. */
typedef struct history history_t;

/* Create a new history buffer with the specified maximum length.
 * Returns NULL on allocation failure. */
history_t *history_create(int max_len);

/* Destroy a history buffer and free all associated memory. */
void history_destroy(history_t *h);

/* Add a line to the history.
 * Duplicate consecutive entries are ignored.
 * When the history is full, the oldest entry is removed.
 * Returns 1 on success, 0 on failure or if the line was a duplicate. */
int history_add(history_t *h, const char *line);

/* Get a history entry by index.
 * Index 0 is the oldest entry, index (len-1) is the newest.
 * Returns NULL if the index is out of bounds or history is NULL. */
const char *history_get(const history_t *h, int index);

/* Get the current number of history entries. */
int history_len(const history_t *h);

/* Get the maximum history length. */
int history_max_len(const history_t *h);

/* Set a new maximum history length.
 * If the new length is smaller than the current number of entries,
 * the oldest entries are removed.
 * Returns 1 on success, 0 on failure. */
int history_set_max_len(history_t *h, int len);

/* Save history to a file.
 * The file is created with permissions 0600 (user read/write only).
 * Returns 0 on success, -1 on failure. */
int history_save(const history_t *h, const char *filename);

/* Load history from a file.
 * Each line in the file becomes a history entry.
 * Returns 0 on success, -1 on failure. */
int history_load(history_t *h, const char *filename, size_t max_line_len);

/* Clear all history entries. */
void history_clear(history_t *h);

/* Duplicate the entry at the given index.
 * Caller must free the returned string.
 * Returns NULL on error. */
char *history_dup(const history_t *h, int index);

/* Replace the entry at the given index with a new string.
 * Returns 1 on success, 0 on failure. */
int history_set(history_t *h, int index, const char *line);

#endif /* LINENOISE_HISTORY_H */
