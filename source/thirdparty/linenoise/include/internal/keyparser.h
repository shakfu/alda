/* internal/keyparser.h - Keyboard input parsing for linenoise
 *
 * This module provides a state machine for parsing keyboard input,
 * including escape sequences for special keys like arrows, function keys,
 * and modified key combinations.
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

#ifndef LINENOISE_KEYPARSER_H
#define LINENOISE_KEYPARSER_H

/* Key codes representing parsed input. */
typedef enum {
    /* Special values (negative to avoid conflict with control codes) */
    KEY_NONE = -3,          /* No key available (timeout or error) */
    KEY_CHAR = -2,          /* Regular character (check utf8 field) */
    KEY_UNKNOWN = -1,       /* Unknown escape sequence */

    /* Control keys (match ASCII values 0-31) */
    KEY_CTRL_A = 1,
    KEY_CTRL_B = 2,
    KEY_CTRL_C = 3,
    KEY_CTRL_D = 4,
    KEY_CTRL_E = 5,
    KEY_CTRL_F = 6,
    KEY_CTRL_G = 7,
    KEY_CTRL_H = 8,         /* Also backspace on some terminals */
    KEY_TAB = 9,            /* Ctrl+I */
    KEY_CTRL_J = 10,        /* Also newline */
    KEY_CTRL_K = 11,
    KEY_CTRL_L = 12,
    KEY_ENTER = 13,         /* Ctrl+M */
    KEY_CTRL_N = 14,
    KEY_CTRL_O = 15,
    KEY_CTRL_P = 16,
    KEY_CTRL_Q = 17,
    KEY_CTRL_R = 18,
    KEY_CTRL_S = 19,
    KEY_CTRL_T = 20,
    KEY_CTRL_U = 21,
    KEY_CTRL_V = 22,
    KEY_CTRL_W = 23,
    KEY_CTRL_X = 24,
    KEY_CTRL_Y = 25,
    KEY_CTRL_Z = 26,
    KEY_ESC = 27,

    /* Arrow keys */
    KEY_UP = 128,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_LEFT,

    /* Navigation keys */
    KEY_HOME,
    KEY_END,
    KEY_INSERT,
    KEY_DELETE,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,

    /* Backspace (127 or Ctrl+H depending on terminal) */
    KEY_BACKSPACE,

    /* Function keys */
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,

    /* Modified arrow keys */
    KEY_CTRL_UP,
    KEY_CTRL_DOWN,
    KEY_CTRL_RIGHT,
    KEY_CTRL_LEFT,

    KEY_ALT_UP,
    KEY_ALT_DOWN,
    KEY_ALT_RIGHT,
    KEY_ALT_LEFT,

    KEY_SHIFT_UP,
    KEY_SHIFT_DOWN,
    KEY_SHIFT_RIGHT,
    KEY_SHIFT_LEFT,

    /* Alt+letter combinations */
    KEY_ALT_B,
    KEY_ALT_D,
    KEY_ALT_F,
    KEY_ALT_BACKSPACE,
} keycode_t;

/* Key modifier flags. */
#define KEY_MOD_NONE  0
#define KEY_MOD_SHIFT (1 << 0)
#define KEY_MOD_ALT   (1 << 1)
#define KEY_MOD_CTRL  (1 << 2)

/* Key event structure returned by the parser. */
typedef struct {
    keycode_t code;         /* Parsed key code */
    char utf8[8];           /* UTF-8 bytes for KEY_CHAR */
    int utf8_len;           /* Length of UTF-8 sequence */
    int modifiers;          /* Modifier flags (KEY_MOD_*) */
} key_event_t;

/* Function type for reading a byte with timeout.
 * Parameters:
 *   fd - file descriptor to read from
 *   c - pointer to store the read byte
 *   timeout_ms - timeout in milliseconds (-1 for blocking, 0 for non-blocking)
 * Returns:
 *   1 on success (byte read)
 *   0 on timeout
 *   -1 on error
 */
typedef int (*keyparser_read_fn)(int fd, char *c, int timeout_ms);

/* Opaque key parser state. */
typedef struct keyparser keyparser_t;

/* Create a new key parser.
 * read_fn - function to read bytes from input
 * fd - file descriptor to pass to read_fn
 * escape_timeout_ms - timeout for escape sequence completion (default: 100)
 * Returns NULL on allocation failure. */
keyparser_t *keyparser_create(keyparser_read_fn read_fn, int fd, int escape_timeout_ms);

/* Destroy a key parser and free associated memory. */
void keyparser_destroy(keyparser_t *kp);

/* Read and parse the next key event.
 * Returns 1 if a key event was read, 0 on timeout, -1 on error.
 * The event is stored in the provided key_event_t structure. */
int keyparser_read(keyparser_t *kp, key_event_t *event);

/* Set the escape sequence timeout in milliseconds. */
void keyparser_set_timeout(keyparser_t *kp, int timeout_ms);

/* Get a human-readable name for a key code. */
const char *keyparser_keyname(keycode_t code);

#endif /* LINENOISE_KEYPARSER_H */
