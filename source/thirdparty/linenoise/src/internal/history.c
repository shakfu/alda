/* src/history.c - History management implementation for linenoise
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

#include "history.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#define open _open
#define close _close
#define fdopen _fdopen
#define strdup _strdup
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

/* History structure definition. */
struct history {
    char **entries;     /* Array of history entry strings. */
    int len;            /* Current number of entries. */
    int max_len;        /* Maximum number of entries. */
};

history_t *history_create(int max_len) {
    history_t *h;

    if (max_len < 1) return NULL;

    h = malloc(sizeof(history_t));
    if (h == NULL) return NULL;

    h->entries = malloc(sizeof(char *) * max_len);
    if (h->entries == NULL) {
        free(h);
        return NULL;
    }

    memset(h->entries, 0, sizeof(char *) * max_len);
    h->len = 0;
    h->max_len = max_len;

    return h;
}

void history_destroy(history_t *h) {
    if (h == NULL) return;

    history_clear(h);
    free(h->entries);
    free(h);
}

int history_add(history_t *h, const char *line) {
    char *linecopy;

    if (h == NULL || line == NULL) return 0;
    if (h->max_len == 0) return 0;

    /* Don't add duplicated lines. */
    if (h->len > 0 && strcmp(h->entries[h->len - 1], line) == 0) {
        return 0;
    }

    /* Add a heap allocated copy of the line. */
    linecopy = strdup(line);
    if (linecopy == NULL) return 0;

    /* If we reached the max length, remove the oldest entry. */
    if (h->len == h->max_len) {
        free(h->entries[0]);
        memmove(h->entries, h->entries + 1, sizeof(char *) * (h->max_len - 1));
        h->len--;
    }

    h->entries[h->len] = linecopy;
    h->len++;
    return 1;
}

const char *history_get(const history_t *h, int index) {
    if (h == NULL) return NULL;
    if (index < 0 || index >= h->len) return NULL;
    return h->entries[index];
}

int history_len(const history_t *h) {
    if (h == NULL) return 0;
    return h->len;
}

int history_max_len(const history_t *h) {
    if (h == NULL) return 0;
    return h->max_len;
}

int history_set_max_len(history_t *h, int len) {
    char **new_entries;
    int tocopy;

    if (h == NULL) return 0;
    if (len < 1) return 0;

    new_entries = malloc(sizeof(char *) * len);
    if (new_entries == NULL) return 0;

    tocopy = h->len;

    /* If shrinking, free entries that won't fit. */
    if (len < tocopy) {
        for (int j = 0; j < tocopy - len; j++) {
            free(h->entries[j]);
        }
        tocopy = len;
    }

    memset(new_entries, 0, sizeof(char *) * len);
    /* Copy the most recent entries. */
    memcpy(new_entries, h->entries + (h->len - tocopy), sizeof(char *) * tocopy);

    free(h->entries);
    h->entries = new_entries;
    h->max_len = len;
    if (h->len > len) {
        h->len = len;
    }

    return 1;
}

int history_save(const history_t *h, const char *filename) {
    int fd;
    FILE *fp;

    if (h == NULL || filename == NULL) return -1;

    /* Open with explicit permissions to avoid race conditions. */
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) return -1;

    fp = fdopen(fd, "w");
    if (fp == NULL) {
        close(fd);
        return -1;
    }

    for (int j = 0; j < h->len; j++) {
        fprintf(fp, "%s\n", h->entries[j]);
    }

    fclose(fp);
    return 0;
}

int history_load(history_t *h, const char *filename, size_t max_line_len) {
    FILE *fp;
    char *buf;
    char *p;

    if (h == NULL || filename == NULL) return -1;
    if (max_line_len == 0) max_line_len = 4096;

    buf = malloc(max_line_len);
    if (buf == NULL) return -1;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        free(buf);
        return -1;
    }

    while (fgets(buf, (int)max_line_len, fp) != NULL) {
        /* Strip trailing newline characters. */
        p = strchr(buf, '\r');
        if (p == NULL) p = strchr(buf, '\n');
        if (p != NULL) *p = '\0';
        history_add(h, buf);
    }

    fclose(fp);
    free(buf);
    return 0;
}

void history_clear(history_t *h) {
    if (h == NULL) return;

    for (int j = 0; j < h->len; j++) {
        free(h->entries[j]);
        h->entries[j] = NULL;
    }
    h->len = 0;
}

char *history_dup(const history_t *h, int index) {
    const char *entry;

    entry = history_get(h, index);
    if (entry == NULL) return NULL;
    return strdup(entry);
}

int history_set(history_t *h, int index, const char *line) {
    char *linecopy;

    if (h == NULL || line == NULL) return 0;
    if (index < 0 || index >= h->len) return 0;

    linecopy = strdup(line);
    if (linecopy == NULL) return 0;

    free(h->entries[index]);
    h->entries[index] = linecopy;
    return 1;
}
