/* src/completion.c - Tab completion implementation for linenoise
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

#include "completion.h"

#include <stdlib.h>
#include <string.h>

void completions_init(completions_t *lc) {
    if (lc == NULL) return;
    lc->len = 0;
    lc->cvec = NULL;
}

void completions_add(completions_t *lc, const char *str) {
    size_t len;
    char *copy;
    char **cvec;

    if (lc == NULL || str == NULL) return;

    len = strlen(str);
    copy = malloc(len + 1);
    if (copy == NULL) return;
    memcpy(copy, str, len + 1);

    cvec = realloc(lc->cvec, sizeof(char *) * (lc->len + 1));
    if (cvec == NULL) {
        free(copy);
        return;
    }

    lc->cvec = cvec;
    lc->cvec[lc->len++] = copy;
}

const char *completions_get(const completions_t *lc, size_t index) {
    if (lc == NULL) return NULL;
    if (index >= lc->len) return NULL;
    return lc->cvec[index];
}

size_t completions_len(const completions_t *lc) {
    if (lc == NULL) return 0;
    return lc->len;
}

void completions_free(completions_t *lc) {
    if (lc == NULL) return;
    completions_clear(lc);
}

void completions_clear(completions_t *lc) {
    size_t i;

    if (lc == NULL) return;

    for (i = 0; i < lc->len; i++) {
        free(lc->cvec[i]);
    }
    if (lc->cvec != NULL) {
        free(lc->cvec);
        lc->cvec = NULL;
    }
    lc->len = 0;
}
