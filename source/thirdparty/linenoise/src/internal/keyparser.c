/* src/keyparser.c - Keyboard input parsing implementation for linenoise
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

#include "keyparser.h"

#include <stdlib.h>
#include <string.h>

/* Default timeout for escape sequences in milliseconds. */
#define DEFAULT_ESCAPE_TIMEOUT 100

/* Key parser state structure. */
struct keyparser {
    keyparser_read_fn read_fn;
    int fd;
    int escape_timeout_ms;
};

/* Helper to determine the number of bytes in a UTF-8 sequence from the first byte. */
static int utf8_byte_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;       /* 0xxxxxxx: ASCII */
    if ((c & 0xE0) == 0xC0) return 2;    /* 110xxxxx */
    if ((c & 0xF0) == 0xE0) return 3;    /* 1110xxxx */
    if ((c & 0xF8) == 0xF0) return 4;    /* 11110xxx */
    return 1; /* Invalid, treat as single byte */
}

/* Helper to read a byte with the configured timeout. */
static int read_byte(keyparser_t *kp, char *c, int timeout_ms) {
    return kp->read_fn(kp->fd, c, timeout_ms);
}

/* Parse CSI (Control Sequence Introducer) sequences: ESC [ ... */
static keycode_t parse_csi_sequence(keyparser_t *kp, int *modifiers) {
    char seq[16];
    int seq_len = 0;
    char c;
    int ret;
    int param1 = 0, param2 = 0;

    *modifiers = KEY_MOD_NONE;

    /* Read characters until we get a final byte (0x40-0x7E). */
    while (seq_len < 15) {
        ret = read_byte(kp, &c, kp->escape_timeout_ms);
        if (ret <= 0) {
            /* Timeout or error - return unknown sequence. */
            return KEY_UNKNOWN;
        }

        seq[seq_len++] = c;
        seq[seq_len] = '\0';

        /* Final byte of CSI sequence is in range 0x40-0x7E. */
        if (c >= 0x40 && c <= 0x7E) {
            break;
        }
    }

    /* Parse the sequence. */
    if (seq_len == 1) {
        /* Simple arrow keys: ESC [ A/B/C/D */
        switch (seq[0]) {
            case 'A': return KEY_UP;
            case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT;
            case 'D': return KEY_LEFT;
            case 'H': return KEY_HOME;
            case 'F': return KEY_END;
        }
    }

    /* Parse numeric parameters. */
    if (seq_len >= 2) {
        char *p = seq;
        param1 = 0;
        param2 = 1; /* Default modifier value is 1 (no modifier). */

        /* First parameter. */
        while (*p >= '0' && *p <= '9') {
            param1 = param1 * 10 + (*p - '0');
            p++;
        }

        /* Check for separator and second parameter (modifier). */
        if (*p == ';') {
            p++;
            param2 = 0;
            while (*p >= '0' && *p <= '9') {
                param2 = param2 * 10 + (*p - '0');
                p++;
            }
        }

        /* Decode modifiers from param2.
         * Value = 1 + (shift ? 1 : 0) + (alt ? 2 : 0) + (ctrl ? 4 : 0)
         */
        if (param2 > 1) {
            if (param2 & 1) *modifiers |= KEY_MOD_SHIFT;
            if (param2 & 2) *modifiers |= KEY_MOD_ALT;
            if (param2 & 4) *modifiers |= KEY_MOD_CTRL;
        }

        /* VT sequences: ESC [ n ~ */
        if (seq[seq_len - 1] == '~') {
            switch (param1) {
                case 1:  return KEY_HOME;
                case 2:  return KEY_INSERT;
                case 3:  return KEY_DELETE;
                case 4:  return KEY_END;
                case 5:  return KEY_PAGE_UP;
                case 6:  return KEY_PAGE_DOWN;
                case 7:  return KEY_HOME;
                case 8:  return KEY_END;
                case 11: return KEY_F1;
                case 12: return KEY_F2;
                case 13: return KEY_F3;
                case 14: return KEY_F4;
                case 15: return KEY_F5;
                case 17: return KEY_F6;
                case 18: return KEY_F7;
                case 19: return KEY_F8;
                case 20: return KEY_F9;
                case 21: return KEY_F10;
                case 23: return KEY_F11;
                case 24: return KEY_F12;
            }
        }

        /* xterm-style modified arrow keys: ESC [ 1 ; mod X */
        if (param1 == 1 && seq[seq_len - 1] >= 'A' && seq[seq_len - 1] <= 'D') {
            switch (seq[seq_len - 1]) {
                case 'A':
                    if (*modifiers & KEY_MOD_CTRL) return KEY_CTRL_UP;
                    if (*modifiers & KEY_MOD_ALT) return KEY_ALT_UP;
                    if (*modifiers & KEY_MOD_SHIFT) return KEY_SHIFT_UP;
                    return KEY_UP;
                case 'B':
                    if (*modifiers & KEY_MOD_CTRL) return KEY_CTRL_DOWN;
                    if (*modifiers & KEY_MOD_ALT) return KEY_ALT_DOWN;
                    if (*modifiers & KEY_MOD_SHIFT) return KEY_SHIFT_DOWN;
                    return KEY_DOWN;
                case 'C':
                    if (*modifiers & KEY_MOD_CTRL) return KEY_CTRL_RIGHT;
                    if (*modifiers & KEY_MOD_ALT) return KEY_ALT_RIGHT;
                    if (*modifiers & KEY_MOD_SHIFT) return KEY_SHIFT_RIGHT;
                    return KEY_RIGHT;
                case 'D':
                    if (*modifiers & KEY_MOD_CTRL) return KEY_CTRL_LEFT;
                    if (*modifiers & KEY_MOD_ALT) return KEY_ALT_LEFT;
                    if (*modifiers & KEY_MOD_SHIFT) return KEY_SHIFT_LEFT;
                    return KEY_LEFT;
            }
        }

        /* Home/End with modifiers. */
        if (seq[seq_len - 1] == 'H') return KEY_HOME;
        if (seq[seq_len - 1] == 'F') return KEY_END;
    }

    return KEY_UNKNOWN;
}

/* Parse O-sequences: ESC O ... */
static keycode_t parse_o_sequence(keyparser_t *kp) {
    char c;
    int ret;

    ret = read_byte(kp, &c, kp->escape_timeout_ms);
    if (ret <= 0) {
        return KEY_UNKNOWN;
    }

    switch (c) {
        /* Arrow keys (some terminals). */
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;

        /* Home/End (some terminals). */
        case 'H': return KEY_HOME;
        case 'F': return KEY_END;

        /* Function keys F1-F4 (VT100 style). */
        case 'P': return KEY_F1;
        case 'Q': return KEY_F2;
        case 'R': return KEY_F3;
        case 'S': return KEY_F4;
    }

    return KEY_UNKNOWN;
}

/* Parse escape sequences starting with ESC. */
static keycode_t parse_escape_sequence(keyparser_t *kp, key_event_t *event) {
    char c;
    int ret;

    event->modifiers = KEY_MOD_NONE;

    /* Try to read the next character with a timeout. */
    ret = read_byte(kp, &c, kp->escape_timeout_ms);
    if (ret <= 0) {
        /* Timeout - this was just the ESC key by itself. */
        return KEY_ESC;
    }

    switch (c) {
        case '[':
            /* CSI sequence. */
            return parse_csi_sequence(kp, &event->modifiers);

        case 'O':
            /* SS3 (Single Shift 3) sequence. */
            return parse_o_sequence(kp);

        /* Alt+letter combinations (ESC followed by letter). */
        case 'b': return KEY_ALT_B;
        case 'd': return KEY_ALT_D;
        case 'f': return KEY_ALT_F;
        case 127: return KEY_ALT_BACKSPACE;

        /* Some terminals send ESC ESC for Alt+ESC or double-ESC. */
        case 27:
            return KEY_ESC;

        default:
            /* Unknown escape sequence - treat as ESC + character.
             * Store the character for potential further processing. */
            event->utf8[0] = c;
            event->utf8_len = 1;
            event->modifiers = KEY_MOD_ALT;
            return KEY_CHAR;
    }
}

keyparser_t *keyparser_create(keyparser_read_fn read_fn, int fd, int escape_timeout_ms) {
    keyparser_t *kp;

    if (read_fn == NULL) return NULL;

    kp = malloc(sizeof(keyparser_t));
    if (kp == NULL) return NULL;

    kp->read_fn = read_fn;
    kp->fd = fd;
    kp->escape_timeout_ms = escape_timeout_ms > 0 ? escape_timeout_ms : DEFAULT_ESCAPE_TIMEOUT;

    return kp;
}

void keyparser_destroy(keyparser_t *kp) {
    free(kp);
}

int keyparser_read(keyparser_t *kp, key_event_t *event) {
    char c;
    int ret;
    int utf8_len;

    if (kp == NULL || event == NULL) return -1;

    /* Initialize the event structure. */
    memset(event, 0, sizeof(key_event_t));

    /* Read the first byte (blocking). */
    ret = read_byte(kp, &c, -1);
    if (ret <= 0) {
        event->code = KEY_NONE;
        return ret;
    }

    /* Handle escape sequences. */
    if (c == 27) {
        event->code = parse_escape_sequence(kp, event);
        return 1;
    }

    /* Handle control characters (0-31). */
    if ((unsigned char)c < 32) {
        event->code = (keycode_t)c;
        event->modifiers = KEY_MOD_CTRL;
        return 1;
    }

    /* Handle backspace (127). */
    if (c == 127) {
        event->code = KEY_BACKSPACE;
        return 1;
    }

    /* Handle regular characters (possibly multi-byte UTF-8). */
    event->code = KEY_CHAR;
    event->utf8[0] = c;
    utf8_len = utf8_byte_len((unsigned char)c);
    event->utf8_len = 1;

    /* Read remaining UTF-8 bytes if needed. */
    for (int i = 1; i < utf8_len; i++) {
        ret = read_byte(kp, &c, kp->escape_timeout_ms);
        if (ret <= 0) {
            /* Incomplete UTF-8 sequence. */
            break;
        }
        event->utf8[event->utf8_len++] = c;
    }

    event->utf8[event->utf8_len] = '\0';
    return 1;
}

void keyparser_set_timeout(keyparser_t *kp, int timeout_ms) {
    if (kp != NULL) {
        kp->escape_timeout_ms = timeout_ms > 0 ? timeout_ms : DEFAULT_ESCAPE_TIMEOUT;
    }
}

const char *keyparser_keyname(keycode_t code) {
    switch (code) {
        case KEY_NONE:          return "NONE";
        case KEY_CHAR:          return "CHAR";
        case KEY_UNKNOWN:       return "UNKNOWN";
        case KEY_CTRL_A:        return "CTRL_A";
        case KEY_CTRL_B:        return "CTRL_B";
        case KEY_CTRL_C:        return "CTRL_C";
        case KEY_CTRL_D:        return "CTRL_D";
        case KEY_CTRL_E:        return "CTRL_E";
        case KEY_CTRL_F:        return "CTRL_F";
        case KEY_CTRL_G:        return "CTRL_G";
        case KEY_CTRL_H:        return "CTRL_H";
        case KEY_TAB:           return "TAB";
        case KEY_CTRL_J:        return "CTRL_J";
        case KEY_CTRL_K:        return "CTRL_K";
        case KEY_CTRL_L:        return "CTRL_L";
        case KEY_ENTER:         return "ENTER";
        case KEY_CTRL_N:        return "CTRL_N";
        case KEY_CTRL_O:        return "CTRL_O";
        case KEY_CTRL_P:        return "CTRL_P";
        case KEY_CTRL_Q:        return "CTRL_Q";
        case KEY_CTRL_R:        return "CTRL_R";
        case KEY_CTRL_S:        return "CTRL_S";
        case KEY_CTRL_T:        return "CTRL_T";
        case KEY_CTRL_U:        return "CTRL_U";
        case KEY_CTRL_V:        return "CTRL_V";
        case KEY_CTRL_W:        return "CTRL_W";
        case KEY_CTRL_X:        return "CTRL_X";
        case KEY_CTRL_Y:        return "CTRL_Y";
        case KEY_CTRL_Z:        return "CTRL_Z";
        case KEY_ESC:           return "ESC";
        case KEY_UP:            return "UP";
        case KEY_DOWN:          return "DOWN";
        case KEY_RIGHT:         return "RIGHT";
        case KEY_LEFT:          return "LEFT";
        case KEY_HOME:          return "HOME";
        case KEY_END:           return "END";
        case KEY_INSERT:        return "INSERT";
        case KEY_DELETE:        return "DELETE";
        case KEY_PAGE_UP:       return "PAGE_UP";
        case KEY_PAGE_DOWN:     return "PAGE_DOWN";
        case KEY_BACKSPACE:     return "BACKSPACE";
        case KEY_F1:            return "F1";
        case KEY_F2:            return "F2";
        case KEY_F3:            return "F3";
        case KEY_F4:            return "F4";
        case KEY_F5:            return "F5";
        case KEY_F6:            return "F6";
        case KEY_F7:            return "F7";
        case KEY_F8:            return "F8";
        case KEY_F9:            return "F9";
        case KEY_F10:           return "F10";
        case KEY_F11:           return "F11";
        case KEY_F12:           return "F12";
        case KEY_CTRL_UP:       return "CTRL_UP";
        case KEY_CTRL_DOWN:     return "CTRL_DOWN";
        case KEY_CTRL_RIGHT:    return "CTRL_RIGHT";
        case KEY_CTRL_LEFT:     return "CTRL_LEFT";
        case KEY_ALT_UP:        return "ALT_UP";
        case KEY_ALT_DOWN:      return "ALT_DOWN";
        case KEY_ALT_RIGHT:     return "ALT_RIGHT";
        case KEY_ALT_LEFT:      return "ALT_LEFT";
        case KEY_SHIFT_UP:      return "SHIFT_UP";
        case KEY_SHIFT_DOWN:    return "SHIFT_DOWN";
        case KEY_SHIFT_RIGHT:   return "SHIFT_RIGHT";
        case KEY_SHIFT_LEFT:    return "SHIFT_LEFT";
        case KEY_ALT_B:         return "ALT_B";
        case KEY_ALT_D:         return "ALT_D";
        case KEY_ALT_F:         return "ALT_F";
        case KEY_ALT_BACKSPACE: return "ALT_BACKSPACE";
        default:                return "UNKNOWN";
    }
}
