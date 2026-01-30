/* linenoise.c -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * You can find the latest source code at:
 *
 *   http://github.com/antirez/linenoise
 *
 * Does a number of crazy assumptions that happen to be true in 99.9999% of
 * the 2010 UNIX computers around.
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
 *
 * ------------------------------------------------------------------------
 *
 * References:
 * - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - http://www.3waylabs.com/nw/WWW/products/wizcon/vt220.html
 *
 * Todo list:
 * - Filter bogus Ctrl+<char> combinations.
 * - Win32 support
 *
 * Bloat:
 * - History search like Ctrl+r in readline?
 *
 * List of escape sequences used by this program, we do everything just
 * with three sequences. In order to be so cheap we may have some
 * flickering effect with some slow terminal, but the lesser sequences
 * the more compatible.
 *
 * EL (Erase Line)
 *    Sequence: ESC [ n K
 *    Effect: if n is 0 or missing, clear from cursor to end of line
 *    Effect: if n is 1, clear from beginning of line to cursor
 *    Effect: if n is 2, clear entire line
 *
 * CUF (CUrsor Forward)
 *    Sequence: ESC [ n C
 *    Effect: moves cursor forward n chars
 *
 * CUB (CUrsor Backward)
 *    Sequence: ESC [ n D
 *    Effect: moves cursor backward n chars
 *
 * The following is used to get the terminal width if getting
 * the width with the TIOCGWINSZ ioctl fails
 *
 * DSR (Device Status Report)
 *    Sequence: ESC [ 6 n
 *    Effect: reports the current cusor position as ESC [ n ; m R
 *            where n is the row and m is the column
 *
 * When multi line mode is enabled, we also use an additional escape
 * sequence. However multi line editing is disabled by default.
 *
 * CUU (Cursor Up)
 *    Sequence: ESC [ n A
 *    Effect: moves cursor up of n chars.
 *
 * CUD (Cursor Down)
 *    Sequence: ESC [ n B
 *    Effect: moves cursor down of n chars.
 *
 * When linenoise_clear_screen() is called, two additional escape sequences
 * are used in order to clear the screen and position the cursor at home
 * position.
 *
 * CUP (Cursor position)
 *    Sequence: ESC [ H
 *    Effect: moves the cursor to upper left corner
 *
 * ED (Erase display)
 *    Sequence: ESC [ 2 J
 *    Effect: clear the whole screen
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#ifdef _WIN32
/* Windows-specific includes. */
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#define isatty _isatty
#define strcasecmp _stricmp
/* MSVC's _snprintf doesn't null-terminate on overflow, so wrap it. */
static int linenoise_snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap;
    int ret;
    va_start(ap, fmt);
    ret = _vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    if (size > 0) buf[size - 1] = '\0';
    return ret;
}
#define snprintf linenoise_snprintf
/* Windows doesn't have these, stub them out. */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
/* POSIX permission flags for open() */
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#else
/* POSIX-specific includes. */
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#endif

#include "linenoise.h"
#include "internal/utf8.h"

#ifdef _WIN32
/* Windows I/O wrappers using console handles. */
static HANDLE win_get_output_handle(int fd) {
    (void)fd;
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

static int win_write(int fd, const void *buf, size_t count) {
    HANDLE h = win_get_output_handle(fd);
    DWORD written;
    if (!WriteConsoleA(h, buf, (DWORD)count, &written, NULL)) {
        /* Fallback to WriteFile for redirected output. */
        if (!WriteFile(h, buf, (DWORD)count, &written, NULL)) {
            return -1;
        }
    }
    return (int)written;
}

static int win_read(int fd, void *buf, size_t count) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode, read_count;
    (void)fd;

    /* Check if we're reading from a console or redirected input. */
    if (GetConsoleMode(h, &mode)) {
        if (!ReadConsoleA(h, buf, (DWORD)count, &read_count, NULL)) {
            return -1;
        }
    } else {
        if (!ReadFile(h, buf, (DWORD)count, &read_count, NULL)) {
            return -1;
        }
    }
    return (int)read_count;
}

#define write(fd, buf, count) win_write(fd, buf, count)
#define read(fd, buf, count) win_read(fd, buf, count)
#endif

/* Compatibility macros mapping old function names to new utf8 module. */
#define utf8ByteLen         utf8_byte_len
#define utf8DecodeChar      utf8_decode
#define utf8DecodePrev      utf8_decode_prev
#define isVariationSelector utf8_is_variation_selector
#define isSkinToneModifier  utf8_is_skin_tone_modifier
#define isZWJ               utf8_is_zwj
#define isRegionalIndicator utf8_is_regional_indicator
#define isCombiningMark     utf8_is_combining_mark
#define isGraphemeExtend    utf8_is_grapheme_extend
#define utf8PrevCharLen     utf8_prev_grapheme_len
#define utf8NextCharLen     utf8_next_grapheme_len
#define utf8CharWidth       utf8_codepoint_width
#define utf8StrWidth        utf8_str_width
#define utf8SingleCharWidth utf8_single_char_width

/* List of terminals known to not support escape sequences. */
static char *unsupported_term[] = {"dumb","cons25","emacs",NULL};

/* ======================= Error Handling ==================================== */

/* Thread-local error code (using static for single-threaded simplicity).
 * For true thread-safety, this should use thread-local storage. */
static linenoise_error_t last_error = LINENOISE_OK;

static void set_error(linenoise_error_t err) {
    last_error = err;
}

linenoise_error_t linenoise_get_error(void) {
    return last_error;
}

const char *linenoise_error_string(linenoise_error_t err) {
    switch (err) {
        case LINENOISE_OK:              return "Success";
        case LINENOISE_ERR_ERRNO:       return "System error (check errno)";
        case LINENOISE_ERR_NOT_TTY:     return "Not a terminal";
        case LINENOISE_ERR_NOT_SUPPORTED: return "Terminal not supported";
        case LINENOISE_ERR_READ:        return "Read error";
        case LINENOISE_ERR_WRITE:       return "Write error";
        case LINENOISE_ERR_MEMORY:      return "Memory allocation failed";
        case LINENOISE_ERR_INVALID:     return "Invalid argument";
        case LINENOISE_ERR_EOF:         return "End of file";
        case LINENOISE_ERR_INTERRUPTED: return "Interrupted";
        default:                        return "Unknown error";
    }
}

/* ======================= Custom Allocator ================================== */

/* Custom allocator function pointers. NULL means use standard functions. */
static linenoise_malloc_fn custom_malloc = NULL;
static linenoise_free_fn custom_free = NULL;
static linenoise_realloc_fn custom_realloc = NULL;

void linenoise_set_allocator(linenoise_malloc_fn malloc_fn,
                             linenoise_free_fn free_fn,
                             linenoise_realloc_fn realloc_fn) {
    custom_malloc = malloc_fn;
    custom_free = free_fn;
    custom_realloc = realloc_fn;
}

/* Internal allocation wrappers. */
static void *ln_malloc(size_t size) {
    void *ptr = custom_malloc ? custom_malloc(size) : malloc(size);
    if (ptr == NULL) set_error(LINENOISE_ERR_MEMORY);
    return ptr;
}

static void ln_free(void *ptr) {
    if (custom_free) {
        custom_free(ptr);
    } else {
        free(ptr);
    }
}

static void *ln_realloc(void *ptr, size_t size) {
    void *new_ptr = custom_realloc ? custom_realloc(ptr, size) : realloc(ptr, size);
    if (new_ptr == NULL && size > 0) set_error(LINENOISE_ERR_MEMORY);
    return new_ptr;
}

static char *ln_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = ln_malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

/* Forward declarations. */
static char *linenoise_no_tty(void);
static void refresh_line_with_completion(linenoise_state_t *ls, linenoise_completions_t *lc, int flags);
static void refresh_line_with_flags(linenoise_state_t *l, int flags);
static int history_add(const char *line);

/* ======================= Context Structure ================================= */

/* The linenoise_context structure encapsulates all state for one linenoise
 * instance. This enables thread-safe usage and multiple independent instances.
 * The old global API uses a default context for backward compatibility. */
struct linenoise_context {
    /* Terminal state */
#ifdef _WIN32
    DWORD orig_console_mode;
#else
    struct termios orig_termios;
#endif
    int rawmode;
    int atexit_registered;

    /* Configuration */
    int maskmode;
    int mlmode;
    int mousemode;

    /* History */
    int history_max_len;
    int history_len;
    char **history;

    /* Callbacks */
    linenoise_completion_cb_t *completion_callback;
    linenoise_hints_cb_t *hints_callback;
    linenoise_free_hints_cb_t *free_hints_callback;
    linenoise_highlight_cb_t *highlight_callback;
};

/* Internal global variables used by the editing functions. */
static linenoise_completion_cb_t *completion_callback = NULL;
static linenoise_hints_cb_t *hints_callback = NULL;
static linenoise_free_hints_cb_t *free_hints_callback = NULL;
static linenoise_highlight_cb_t *highlight_callback = NULL;
#ifdef _WIN32
static DWORD orig_console_mode; /* In order to restore at exit.*/
#else
static struct termios orig_termios; /* In order to restore at exit.*/
#endif
static int maskmode = 0; /* Show "***" instead of input. For passwords. */
static int rawmode = 0; /* For atexit() function to check if restore is needed*/
static int mlmode = 0;  /* Multi line mode. Default is single line. */
static int mousemode = 0; /* Mouse tracking mode. */
static int atexit_registered = 0; /* Register atexit just 1 time. */
static int history_max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
static int history_len = 0;
static char **history = NULL;

/* UTF-8 support is now provided by src/utf8.c via internal/utf8.h.
 * The compatibility macros above map old function names to the new module. */

enum KEY_ACTION{
	KEY_NULL = 0,	    /* NULL */
	CTRL_A = 1,         /* Ctrl+a */
	CTRL_B = 2,         /* Ctrl-b */
	CTRL_C = 3,         /* Ctrl-c */
	CTRL_D = 4,         /* Ctrl-d */
	CTRL_E = 5,         /* Ctrl-e */
	CTRL_F = 6,         /* Ctrl-f */
	CTRL_H = 8,         /* Ctrl-h */
	TAB = 9,            /* Tab */
	CTRL_K = 11,        /* Ctrl+k */
	CTRL_L = 12,        /* Ctrl+l */
	ENTER = 13,         /* Enter */
	CTRL_N = 14,        /* Ctrl-n */
	CTRL_P = 16,        /* Ctrl-p */
	CTRL_T = 20,        /* Ctrl-t */
	CTRL_U = 21,        /* Ctrl+u */
	CTRL_W = 23,        /* Ctrl+w */
	CTRL_Y = 25,        /* Ctrl+y (redo) */
	CTRL_Z = 26,        /* Ctrl+z (undo) */
	ESC = 27,           /* Escape */
	BACKSPACE =  127    /* Backspace */
};

static void linenoise_at_exit(void);
#define REFRESH_CLEAN (1<<0)    // Clean the old prompt from the screen
#define REFRESH_WRITE (1<<1)    // Rewrite the prompt on the screen.
#define REFRESH_ALL (REFRESH_CLEAN|REFRESH_WRITE) // Do both.
static void refresh_line(linenoise_state_t *l);

/* Debugging macro. */
#if 0
FILE *lndebug_fp = NULL;
#define lndebug(...) \
    do { \
        if (lndebug_fp == NULL) { \
            lndebug_fp = fopen("/tmp/lndebug.txt","a"); \
            fprintf(lndebug_fp, \
            "[%d %d %d] p: %d, rows: %d, rpos: %d, max: %d, oldmax: %d\n", \
            (int)l->len,(int)l->pos,(int)l->oldpos,plen,rows,rpos, \
            (int)l->oldrows,old_rows); \
        } \
        fprintf(lndebug_fp, ", " __VA_ARGS__); \
        fflush(lndebug_fp); \
    } while (0)
#else
#define lndebug(...) ((void)0)
#endif

/* ======================= Low level terminal handling ====================== */

/* Return true if the terminal name is in the list of terminals we know are
 * not able to understand basic escape sequences. */
static int is_unsupported_term(void) {
#ifdef _WIN32
    /* Windows console with VT mode is always supported. */
    return 0;
#else
    char *term = getenv("TERM");
    int j;

    if (term == NULL) return 0;
    for (j = 0; unsupported_term[j]; j++)
        if (!strcasecmp(term,unsupported_term[j])) return 1;
    return 0;
#endif
}

#ifdef _WIN32
/* Windows VT mode flags (may not be defined in older SDKs). */
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif

static HANDLE h_console_input = INVALID_HANDLE_VALUE;
static HANDLE h_console_output = INVALID_HANDLE_VALUE;
static DWORD orig_input_mode = 0;
static DWORD orig_output_mode = 0;

/* Raw mode for Windows using VT100 emulation (Windows 10+). */
static int enable_raw_mode(int fd) {
    DWORD input_mode, output_mode;
    (void)fd;  /* Unused on Windows. */

    /* Test mode: when LINENOISE_ASSUME_TTY is set, skip terminal setup. */
    if (getenv("LINENOISE_ASSUME_TTY")) {
        rawmode = 1;
        return 0;
    }

    h_console_input = GetStdHandle(STD_INPUT_HANDLE);
    h_console_output = GetStdHandle(STD_OUTPUT_HANDLE);

    if (h_console_input == INVALID_HANDLE_VALUE ||
        h_console_output == INVALID_HANDLE_VALUE) {
        return -1;
    }

    if (!GetConsoleMode(h_console_input, &orig_input_mode)) {
        return -1;
    }
    if (!GetConsoleMode(h_console_output, &orig_output_mode)) {
        return -1;
    }

    if (!atexit_registered) {
        atexit(linenoise_at_exit);
        atexit_registered = 1;
    }

    /* Configure input: disable line input, echo, and processed input.
     * Enable VT input for escape sequence support. */
    input_mode = orig_input_mode;
    input_mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
    input_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;

    if (!SetConsoleMode(h_console_input, input_mode)) {
        return -1;
    }

    /* Configure output: enable VT processing for escape sequences. */
    output_mode = orig_output_mode;
    output_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    if (!SetConsoleMode(h_console_output, output_mode)) {
        /* VT mode not available, restore input mode. */
        SetConsoleMode(h_console_input, orig_input_mode);
        return -1;
    }

    rawmode = 1;
    return 0;
}

static void disable_raw_mode(int fd) {
    (void)fd;

    if (getenv("LINENOISE_ASSUME_TTY")) {
        rawmode = 0;
        return;
    }

    if (rawmode) {
        SetConsoleMode(h_console_input, orig_input_mode);
        SetConsoleMode(h_console_output, orig_output_mode);
        rawmode = 0;
    }
}

/* Read a single byte with a timeout on Windows. */
static int read_byte_with_timeout(int fd, char *c, int timeout_ms) {
    DWORD wait_result;
    DWORD read_count;
    (void)fd;

    if (timeout_ms == 0) {
        /* Non-blocking check. */
        DWORD events;
        if (!GetNumberOfConsoleInputEvents(h_console_input, &events) || events == 0) {
            return 0;
        }
    } else if (timeout_ms > 0) {
        wait_result = WaitForSingleObject(h_console_input, (DWORD)timeout_ms);
        if (wait_result == WAIT_TIMEOUT) return 0;
        if (wait_result != WAIT_OBJECT_0) return -1;
    }

    if (!ReadConsoleA(h_console_input, c, 1, &read_count, NULL)) {
        return -1;
    }
    return (read_count > 0) ? 1 : -1;
}

#else /* POSIX */

/* Raw mode: 1960 magic shit. */
static int enable_raw_mode(int fd) {
    struct termios raw;

    /* Test mode: when LINENOISE_ASSUME_TTY is set, skip terminal setup.
     * This allows testing via pipes without a real terminal. */
    if (getenv("LINENOISE_ASSUME_TTY")) {
        rawmode = 1;
        return 0;
    }

    if (!isatty(STDIN_FILENO)) goto fatal;
    if (!atexit_registered) {
        atexit(linenoise_at_exit);
        atexit_registered = 1;
    }
    if (tcgetattr(fd,&orig_termios) == -1) goto fatal;

    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
    rawmode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}

static void disable_raw_mode(int fd) {
    /* Test mode: nothing to restore. */
    if (getenv("LINENOISE_ASSUME_TTY")) {
        rawmode = 0;
        return;
    }
    /* Don't even check the return value as it's too late. */
    if (rawmode && tcsetattr(fd,TCSAFLUSH,&orig_termios) != -1)
        rawmode = 0;
}

/* Read a single byte with a timeout. Returns 1 on success, 0 on timeout,
 * -1 on error. timeout_ms is the timeout in milliseconds. */
static int read_byte_with_timeout(int fd, char *c, int timeout_ms) {
    fd_set readfds;
    struct timeval tv;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int ret = select(fd + 1, &readfds, NULL, NULL, &tv);
    if (ret <= 0) return ret;  /* 0 = timeout, -1 = error */
    return read(fd, c, 1);
}

#endif /* _WIN32 */

/* Enable mouse tracking mode. Sends escape sequences to enable X10 mouse
 * reporting with SGR extended coordinates. */
static void enable_mouse_tracking(int fd) {
    /* \x1b[?1000h - Enable X10 mouse reporting (button press)
     * \x1b[?1002h - Enable cell motion mouse tracking (drag events)
     * \x1b[?1006h - Enable SGR extended mouse mode (better coordinate encoding) */
    const char *seq = "\x1b[?1000h\x1b[?1002h\x1b[?1006h";
    if (write(fd, seq, strlen(seq)) == -1) { /* Ignore errors */ }
}

/* Disable mouse tracking mode. */
static void disable_mouse_tracking(int fd) {
    const char *seq = "\x1b[?1006l\x1b[?1002l\x1b[?1000l";
    if (write(fd, seq, strlen(seq)) == -1) { /* Ignore errors */ }
}

#ifdef _WIN32

/* Get terminal columns on Windows. */
static int get_columns(int ifd, int ofd) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    (void)ifd; (void)ofd;

    /* Test mode: use LINENOISE_COLS env var for fixed width. */
    char *cols_env = getenv("LINENOISE_COLS");
    if (cols_env) return atoi(cols_env);

    if (GetConsoleScreenBufferInfo(h_console_output, &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80;
}

/* Internal: Clear the screen on Windows. */
static void clear_screen(void) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD coord = {0, 0};
    DWORD written, console_size;

    if (!GetConsoleScreenBufferInfo(h_console_output, &csbi)) {
        /* Fallback to escape sequence. */
        DWORD dummy;
        WriteConsoleA(h_console_output, "\x1b[H\x1b[2J", 7, &dummy, NULL);
        return;
    }

    console_size = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacterA(h_console_output, ' ', console_size, coord, &written);
    FillConsoleOutputAttribute(h_console_output, csbi.wAttributes, console_size, coord, &written);
    SetConsoleCursorPosition(h_console_output, coord);
}

#else /* POSIX */

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor. */
static int get_cursor_position(int ifd, int ofd) {
    char buf[32];
    int cols, rows;
    unsigned int i = 0;

    /* Report cursor location */
    if (write(ofd, "\x1b[6n", 4) != 4) return -1;

    /* Read the response: ESC [ rows ; cols R */
    while (i < sizeof(buf)-1) {
        if (read(ifd,buf+i,1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    /* Parse it. */
    if (buf[0] != ESC || buf[1] != '[') return -1;
    if (sscanf(buf+2,"%d;%d",&rows,&cols) != 2) return -1;
    return cols;
}

/* Try to get the number of columns in the current terminal, or assume 80
 * if it fails. */
static int get_columns(int ifd, int ofd) {
    struct winsize ws;

    /* Test mode: use LINENOISE_COLS env var for fixed width. */
    char *cols_env = getenv("LINENOISE_COLS");
    if (cols_env) return atoi(cols_env);

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* ioctl() failed. Try to query the terminal itself. */
        int start, cols;

        /* Get the initial position so we can restore it later. */
        start = get_cursor_position(ifd,ofd);
        if (start == -1) goto failed;

        /* Go to right margin and get position. */
        if (write(ofd,"\x1b[999C",6) != 6) goto failed;
        cols = get_cursor_position(ifd,ofd);
        if (cols == -1) goto failed;

        /* Restore position. */
        if (cols > start) {
            char seq[LINENOISE_SEQ_SIZE];
            snprintf(seq,sizeof(seq),"\x1b[%dD",cols-start);
            if (write(ofd,seq,strlen(seq)) == -1) {
                /* Can't recover... */
            }
        }
        return cols;
    } else {
        return ws.ws_col;
    }

failed:
    return 80;
}

/* Internal: Clear the screen. Used to handle ctrl+l */
static void clear_screen(void) {
    if (write(STDOUT_FILENO,"\x1b[H\x1b[2J",7) <= 0) {
        /* nothing to do, just to avoid warning. */
    }
}

#endif /* _WIN32 */

/* Beep, used for completion when there is nothing to complete or when all
 * the choices were already shown. */
static void linenoise_beep(void) {
    fprintf(stderr, "\x7");
    fflush(stderr);
}

/* ============================== Completion ================================ */

/* Free a list of completion option populated by linenoise_add_completion(). */
static void free_completions(linenoise_completions_t *lc) {
    size_t i;
    for (i = 0; i < lc->len; i++)
        ln_free(lc->cvec[i]);
    if (lc->cvec != NULL)
        ln_free(lc->cvec);
}

/* Called by complete_line() and linenoise_show() to render the current
 * edited line with the proposed completion. If the current completion table
 * is already available, it is passed as second argument, otherwise the
 * function will use the callback to obtain it.
 *
 * Flags are the same as refresh_line*(), that is REFRESH_* macros. */
static void refresh_line_with_completion(linenoise_state_t *ls, linenoise_completions_t *lc, int flags) {
    /* Obtain the table of completions if the caller didn't provide one. */
    linenoise_completions_t ctable = { 0, NULL };
    if (lc == NULL) {
        completion_callback(ls->buf,&ctable);
        lc = &ctable;
    }

    /* Show the edited line with completion if possible, or just refresh. */
    if (ls->completion_idx < lc->len) {
        linenoise_state_t saved = *ls;
        ls->len = ls->pos = strlen(lc->cvec[ls->completion_idx]);
        ls->buf = lc->cvec[ls->completion_idx];
        refresh_line_with_flags(ls,flags);
        ls->len = saved.len;
        ls->pos = saved.pos;
        ls->buf = saved.buf;
    } else {
        refresh_line_with_flags(ls,flags);
    }

    /* Free the completions table if we allocated it locally. */
    if (lc == &ctable) free_completions(&ctable);
}

/* This is an helper function for linenoise_edit_*() and is called when the
 * user types the <tab> key in order to complete the string currently in the
 * input.
 *
 * The state of the editing is encapsulated into the pointed linenoise_state_t
 * structure as described in the structure definition.
 *
 * If the function returns non-zero, the caller should handle the
 * returned value as a byte read from the standard input, and process
 * it as usually: this basically means that the function may return a byte
 * read from the termianl but not processed. Otherwise, if zero is returned,
 * the input was consumed by the complete_line() function to navigate the
 * possible completions, and the caller should read for the next characters
 * from stdin. */
static int complete_line(linenoise_state_t *ls, int keypressed) {
    linenoise_completions_t lc = { 0, NULL };
    int nwritten;
    char c = keypressed;

    completion_callback(ls->buf,&lc);
    if (lc.len == 0) {
        linenoise_beep();
        ls->in_completion = 0;
    } else {
        switch(c) {
            case 9: /* tab */
                if (ls->in_completion == 0) {
                    ls->in_completion = 1;
                    ls->completion_idx = 0;
                } else {
                    ls->completion_idx = (ls->completion_idx+1) % (lc.len+1);
                    if (ls->completion_idx == lc.len) linenoise_beep();
                }
                c = 0;
                break;
            case 27: /* escape */
                /* Re-show original buffer */
                if (ls->completion_idx < lc.len) refresh_line(ls);
                ls->in_completion = 0;
                c = 0;
                break;
            default:
                /* Update buffer and return */
                if (ls->completion_idx < lc.len) {
                    nwritten = snprintf(ls->buf,ls->buflen,"%s",
                        lc.cvec[ls->completion_idx]);
                    ls->len = ls->pos = nwritten;
                }
                ls->in_completion = 0;
                break;
        }

        /* Show completion or original buffer */
        if (ls->in_completion && ls->completion_idx < lc.len) {
            refresh_line_with_completion(ls,&lc,REFRESH_ALL);
        } else {
            refresh_line(ls);
        }
    }

    free_completions(&lc);
    return c; /* Return last read character */
}

/* This function is used by the callback function registered by the user
 * in order to add completion options given the input string when the
 * user typed <tab>. See the example.c source code for a very easy to
 * understand example. */
void linenoise_add_completion(linenoise_completions_t *lc, const char *str) {
    size_t len = strlen(str);
    char *copy, **cvec;

    copy = ln_malloc(len+1);
    if (copy == NULL) return;
    memcpy(copy,str,len+1);
    cvec = ln_realloc(lc->cvec,sizeof(char*)*(lc->len+1));
    if (cvec == NULL) {
        ln_free(copy);
        return;
    }
    lc->cvec = cvec;
    lc->cvec[lc->len++] = copy;
}

/* =========================== Line editing ================================= */

/* We define a very simple "append buffer" structure, that is an heap
 * allocated string where we can append to. This is useful in order to
 * write all the escape sequences in a buffer and flush them to the standard
 * output in a single call, to avoid flickering effects. */
struct abuf {
    char *b;
    int len;
};

static void ab_init(struct abuf *ab) {
    ab->b = NULL;
    ab->len = 0;
}

static void ab_append(struct abuf *ab, const char *s, int len) {
    char *new = ln_realloc(ab->b,ab->len+len);

    if (new == NULL) return;
    memcpy(new+ab->len,s,len);
    ab->b = new;
    ab->len += len;
}

static void ab_free(struct abuf *ab) {
    ln_free(ab->b);
}

/* Helper to append text with syntax highlighting.
 * If highlight_callback is set, calls it to get colors for each byte,
 * then outputs the text with appropriate ANSI color codes.
 *
 * Color values use 256-color palette:
 *   0       = default (no color change)
 *   1-15    = basic ANSI colors (legacy 16-color support)
 *   16-255  = extended 256-color palette
 *
 * For colors 1-15, the old behavior is preserved for compatibility:
 *   1-7     = standard colors (red, green, yellow, blue, magenta, cyan, white)
 *   8-15    = bold/bright variants
 *
 * For colors 16-255, we use the 256-color escape sequence:
 *   \x1b[38;5;Xm where X is the color number
 */
static void ab_append_highlighted(struct abuf *ab, const char *buf, size_t len) {
    char seq[32];
    char *colors = NULL;
    size_t i;
    int cur_color = 0;

    if (!highlight_callback || len == 0) {
        /* Output text, clearing each line after newlines for multiline support. */
        size_t start = 0;
        for (i = 0; i < len; i++) {
            if (buf[i] == '\n') {
                ab_append(ab, buf + start, (int)(i - start + 1));
                ab_append(ab, "\r\x1b[0K", 5);  /* Go to column 0 and clear line */
                start = i + 1;
            }
        }
        if (start < len) {
            ab_append(ab, buf + start, (int)(len - start));
        }
        return;
    }

    /* Allocate and zero the colors array. */
    colors = ln_malloc(len);
    if (!colors) {
        ab_append(ab, buf, (int)len);
        return;
    }
    memset(colors, 0, len);

    /* Call the highlight callback. */
    highlight_callback(buf, colors, len);

    /* Output text with color changes. */
    for (i = 0; i < len; i++) {
        int new_color = (unsigned char)colors[i];
        if (new_color != cur_color) {
            /* Output color change sequence. */
            if (new_color == 0) {
                /* Reset to default. */
                ab_append(ab, "\x1b[0m", 4);
            } else if (new_color < 16) {
                /* Legacy 16-color mode for backward compatibility. */
                int bold = (new_color & 8) ? 1 : 0;
                int fg = 30 + (new_color & 7);  /* 31-37 */
                if (bold) {
                    snprintf(seq, sizeof(seq), "\x1b[1;%dm", fg);
                } else {
                    snprintf(seq, sizeof(seq), "\x1b[%dm", fg);
                }
                ab_append(ab, seq, (int)strlen(seq));
            } else {
                /* 256-color mode. */
                snprintf(seq, sizeof(seq), "\x1b[38;5;%dm", new_color);
                ab_append(ab, seq, (int)strlen(seq));
            }
            cur_color = new_color;
        }
        ab_append(ab, buf + i, 1);
        /* After outputting a newline, go to column 0 and clear the line. */
        if (buf[i] == '\n') {
            ab_append(ab, "\r\x1b[0K", 5);
        }
    }

    /* Reset color at the end if we changed it. */
    if (cur_color != 0) {
        ab_append(ab, "\x1b[0m", 4);
    }

    ln_free(colors);
}

/* Helper of refresh_single_line() and refresh_multi_line() to show hints
 * to the right of the prompt. Now uses display widths for proper UTF-8. */
void refresh_show_hints(struct abuf *ab, linenoise_state_t *l, int pwidth) {
    char seq[LINENOISE_SEQ_SIZE];
    size_t bufwidth = utf8StrWidth(l->buf, l->len);
    if (hints_callback && pwidth + bufwidth < l->cols) {
        int color = -1, bold = 0;
        char *hint = hints_callback(l->buf,&color,&bold);
        if (hint) {
            size_t hintlen = strlen(hint);
            size_t hintwidth = utf8StrWidth(hint, hintlen);
            size_t hintmaxwidth = l->cols - (pwidth + bufwidth);
            /* Truncate hint to fit, respecting UTF-8 boundaries. */
            if (hintwidth > hintmaxwidth) {
                size_t i = 0, w = 0;
                while (i < hintlen) {
                    size_t clen = utf8NextCharLen(hint, i, hintlen);
                    int cwidth = utf8SingleCharWidth(hint + i, clen);
                    if (w + cwidth > hintmaxwidth) break;
                    w += cwidth;
                    i += clen;
                }
                hintlen = i;
            }
            if (bold == 1 && color == -1) color = 37;
            if (color != -1 || bold != 0)
                snprintf(seq,sizeof(seq),"\033[%d;%d;49m",bold,color);
            else
                seq[0] = '\0';
            ab_append(ab,seq,strlen(seq));
            ab_append(ab,hint,hintlen);
            if (color != -1 || bold != 0)
                ab_append(ab,"\033[0m",4);
            /* Call the function to free the hint returned. */
            if (free_hints_callback) free_hints_callback(hint);
        }
    }
}

/* Calculate total display rows for text with embedded newlines.
 * Takes into account both terminal width wrapping and explicit newlines.
 * initial_col is the starting column (e.g., prompt width for first line). */
static int calc_rows_with_newlines(const char *buf, size_t len, size_t cols, size_t initial_col) {
    int rows = 1;
    size_t col = initial_col;
    size_t i = 0;

    while (i < len) {
        if (buf[i] == '\n') {
            rows++;
            col = 0;
            i++;
        } else {
            size_t clen = utf8NextCharLen(buf, i, len);
            int cwidth = utf8SingleCharWidth(buf + i, clen);
            col += cwidth;
            if (col >= cols) {
                rows++;
                col = col - cols;  /* Wrap: remaining width goes to next row */
            }
            i += clen;
        }
    }
    return rows;
}

/* Calculate cursor row and column for a position in text with embedded newlines.
 * Returns the row (1-based) in *out_row and column (0-based) in *out_col.
 * initial_col is the starting column (e.g., prompt width for first line).
 * buflen is the total buffer length (needed for UTF-8 boundary detection). */
static void calc_cursor_pos_with_newlines(const char *buf, size_t pos, size_t buflen,
                                          size_t cols, size_t initial_col,
                                          int *out_row, int *out_col) {
    int row = 1;
    size_t col = initial_col;
    size_t i = 0;

    while (i < pos) {
        if (buf[i] == '\n') {
            row++;
            col = 0;
            i++;
        } else {
            size_t clen = utf8NextCharLen(buf, i, buflen);
            int cwidth = utf8SingleCharWidth(buf + i, clen);
            col += cwidth;
            if (col >= cols) {
                row++;
                col = col - cols;
            }
            i += clen;
        }
    }
    *out_row = row;
    *out_col = (int)col;
}

/* Single line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal.
 *
 * Flags is REFRESH_* macros. The function can just remove the old
 * prompt, just write it, or both.
 *
 * This function is UTF-8 aware and uses display widths (not byte counts)
 * for cursor positioning and horizontal scrolling. */
static void refresh_single_line(linenoise_state_t *l, int flags) {
    char seq[LINENOISE_SEQ_SIZE];
    size_t pwidth = utf8StrWidth(l->prompt, l->plen); /* Prompt display width */
    int fd = l->ofd;
    char *buf = l->buf;
    size_t len = l->len;    /* Byte length of buffer to display */
    size_t pos = l->pos;    /* Byte position of cursor */
    size_t poscol;          /* Display column of cursor */
    size_t lencol;          /* Display width of buffer */
    struct abuf ab;

    /* Calculate the display width up to cursor and total display width. */
    poscol = utf8StrWidth(buf, pos);
    lencol = utf8StrWidth(buf, len);

    /* Scroll the buffer horizontally if cursor is past the right edge.
     * We need to trim full UTF-8 characters from the left until the
     * cursor position fits within the terminal width. */
    while (pwidth + poscol >= l->cols) {
        size_t clen = utf8NextCharLen(buf, 0, len);
        int cwidth = utf8SingleCharWidth(buf, clen);
        buf += clen;
        len -= clen;
        pos -= clen;
        poscol -= cwidth;
        lencol -= cwidth;
    }

    /* Trim from the right if the line still doesn't fit. */
    while (pwidth + lencol > l->cols) {
        size_t clen = utf8PrevCharLen(buf, len);
        int cwidth = utf8SingleCharWidth(buf + len - clen, clen);
        len -= clen;
        lencol -= cwidth;
    }

    ab_init(&ab);
    /* Cursor to left edge */
    snprintf(seq,sizeof(seq),"\r");
    ab_append(&ab,seq,strlen(seq));

    if (flags & REFRESH_WRITE) {
        /* Write the prompt and the current buffer content */
        ab_append(&ab,l->prompt,l->plen);
        if (maskmode == 1) {
            /* In mask mode, we output one '*' per UTF-8 character, not byte */
            size_t i = 0;
            while (i < len) {
                ab_append(&ab,"*",1);
                i += utf8NextCharLen(buf, i, len);
            }
        } else {
            ab_append_highlighted(&ab,buf,len);
        }
        /* Show hints if any. */
        refresh_show_hints(&ab,l,pwidth);
    }

    /* Erase to right */
    snprintf(seq,sizeof(seq),"\x1b[0K");
    ab_append(&ab,seq,strlen(seq));

    if (flags & REFRESH_WRITE) {
        /* Move cursor to original position (using display column, not byte). */
        snprintf(seq,sizeof(seq),"\r\x1b[%dC", (int)(poscol+pwidth));
        ab_append(&ab,seq,strlen(seq));
    }

    if (write(fd,ab.b,ab.len) == -1) {} /* Can't recover from write error. */
    ab_free(&ab);
}

/* Multi line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal.
 *
 * Flags is REFRESH_* macros. The function can just remove the old
 * prompt, just write it, or both.
 *
 * This function is UTF-8 aware and uses display widths for positioning. */
static void refresh_multi_line(linenoise_state_t *l, int flags) {
    char seq[LINENOISE_SEQ_SIZE];
    size_t pwidth = utf8StrWidth(l->prompt, l->plen);  /* Prompt display width */
    int rows;      /* rows used by current buffer */
    int rpos = l->oldrpos;   /* cursor relative row from previous refresh. */
    int rpos2; /* rpos after refresh. */
    int col; /* column position, zero-based. */
    int old_rows = l->oldrows;
    int fd = l->ofd, j;
    struct abuf ab;

    /* Calculate rows accounting for embedded newlines in the buffer. */
    rows = calc_rows_with_newlines(l->buf, l->len, l->cols, pwidth);

    l->oldrows = rows;

    /* First step: clear all the lines used before. To do so start by
     * going to the last row. */
    ab_init(&ab);

    if (flags & REFRESH_CLEAN) {
        if (old_rows-rpos > 0) {
            lndebug("go down %d", old_rows-rpos);
            snprintf(seq,sizeof(seq),"\x1b[%dB", old_rows-rpos);
            ab_append(&ab,seq,strlen(seq));
        }

        /* Now for every row clear it, go up. */
        for (j = 0; j < old_rows-1; j++) {
            lndebug("clear+up");
            snprintf(seq,sizeof(seq),"\r\x1b[0K\x1b[1A");
            ab_append(&ab,seq,strlen(seq));
        }
    }

    if (flags & REFRESH_ALL) {
        /* Clean the top line. */
        lndebug("clear");
        snprintf(seq,sizeof(seq),"\r\x1b[0K");
        ab_append(&ab,seq,strlen(seq));

        /* If current content has more rows than old content, clear extra rows.
         * This handles the case where we added a newline to the buffer.
         * old_rows must be at least 1 for this to make sense. */
        if (rows > old_rows && old_rows >= 1) {
            int extra_rows = rows - old_rows;
            for (j = 0; j < extra_rows; j++) {
                snprintf(seq,sizeof(seq),"\n\x1b[0K");
                ab_append(&ab,seq,strlen(seq));
            }
            /* Go back up to the top line. */
            snprintf(seq,sizeof(seq),"\x1b[%dA\r", extra_rows);
            ab_append(&ab,seq,strlen(seq));
        }
    }

    if (flags & REFRESH_WRITE) {
        /* Write the prompt and the current buffer content */
        ab_append(&ab,l->prompt,l->plen);
        if (maskmode == 1) {
            /* In mask mode, output one '*' per UTF-8 character, not byte */
            size_t i = 0;
            while (i < l->len) {
                ab_append(&ab,"*",1);
                i += utf8NextCharLen(l->buf, i, l->len);
            }
        } else {
            ab_append_highlighted(&ab,l->buf,l->len);
        }

        /* Show hints if any. */
        refresh_show_hints(&ab,l,pwidth);

        /* Calculate cursor position accounting for embedded newlines. */
        calc_cursor_pos_with_newlines(l->buf, l->pos, l->len, l->cols, pwidth, &rpos2, &col);
        lndebug("rpos2 %d col %d", rpos2, col);

        /* If we are at the very end of the screen with our prompt, we need to
         * emit a newline and move the prompt to the first column. */
        if (l->pos &&
            l->pos == l->len &&
            col == 0)
        {
            lndebug("<newline>");
            ab_append(&ab,"\n",1);
            snprintf(seq,sizeof(seq),"\r");
            ab_append(&ab,seq,strlen(seq));
            rows++;
            if (rows > (int)l->oldrows) l->oldrows = rows;
        }

        /* Go up till we reach the expected position. */
        if (rows-rpos2 > 0) {
            lndebug("go-up %d", rows-rpos2);
            snprintf(seq,sizeof(seq),"\x1b[%dA", rows-rpos2);
            ab_append(&ab,seq,strlen(seq));
        }

        /* Set column. */
        lndebug("set col %d", 1+col);
        if (col)
            snprintf(seq,sizeof(seq),"\r\x1b[%dC", col);
        else
            snprintf(seq,sizeof(seq),"\r");
        ab_append(&ab,seq,strlen(seq));
    }

    lndebug("\n");
    l->oldpos = l->pos;
    if (flags & REFRESH_WRITE) l->oldrpos = rpos2;

    if (write(fd,ab.b,ab.len) == -1) {} /* Can't recover from write error. */
    ab_free(&ab);
}

/* Calls the two low level functions refresh_single_line() or
 * refresh_multi_line() according to the selected mode. */
static void refresh_line_with_flags(linenoise_state_t *l, int flags) {
    if (mlmode)
        refresh_multi_line(l,flags);
    else
        refresh_single_line(l,flags);
}

/* Utility function to avoid specifying REFRESH_ALL all the times. */
static void refresh_line(linenoise_state_t *l) {
    refresh_line_with_flags(l,REFRESH_ALL);
}

/* Hide the current line, when using the multiplexing API. */
void linenoise_hide(linenoise_state_t *l) {
    if (mlmode)
        refresh_multi_line(l,REFRESH_CLEAN);
    else
        refresh_single_line(l,REFRESH_CLEAN);
}

/* Show the current line, when using the multiplexing API. */
void linenoise_show(linenoise_state_t *l) {
    if (l->in_completion) {
        refresh_line_with_completion(l,NULL,REFRESH_WRITE);
    } else {
        refresh_line_with_flags(l,REFRESH_WRITE);
    }
}

/* Insert the character(s) 'c' of length 'clen' at cursor current position.
 * This handles both single-byte ASCII and multi-byte UTF-8 sequences.
 *
 * On error writing to the terminal -1 is returned, otherwise 0. */
int linenoise_edit_insert(linenoise_state_t *l, const char *c, size_t clen) {
    /* Check if we need to grow the buffer (dynamic mode only). */
    if (l->len + clen > l->buflen) {
        if (l->buf_dynamic) {
            /* Grow buffer: double the size or add at least the needed space. */
            size_t newsize = l->buflen * 2;
            if (newsize < l->len + clen + 1) {
                newsize = l->len + clen + 1;
            }
            char *newbuf = ln_realloc(l->buf, newsize);
            if (newbuf == NULL) {
                return -1;  /* Out of memory */
            }
            l->buf = newbuf;
            l->buflen = newsize - 1;  /* Reserve space for null terminator */
        } else {
            /* Fixed buffer, can't grow - silently ignore. */
            return 0;
        }
    }

    if (l->len == l->pos) {
        /* Append at end of line. */
        memcpy(l->buf+l->pos, c, clen);
        l->pos += clen;
        l->len += clen;
        l->buf[l->len] = '\0';
        if ((!mlmode &&
             utf8StrWidth(l->prompt,l->plen)+utf8StrWidth(l->buf,l->len) < l->cols &&
             !hints_callback &&
             !highlight_callback)) {
            /* Avoid a full update of the line in the trivial case:
             * single-width char, no hints, no highlighting, fits in one line. */
            if (maskmode == 1) {
                if (write(l->ofd,"*",1) == -1) return -1;
            } else {
                if (write(l->ofd,c,clen) == -1) return -1;
            }
        } else {
            refresh_line(l);
        }
    } else {
        /* Insert in the middle of the line. */
        memmove(l->buf+l->pos+clen, l->buf+l->pos, l->len-l->pos);
        memcpy(l->buf+l->pos, c, clen);
        l->len += clen;
        l->pos += clen;
        l->buf[l->len] = '\0';
        refresh_line(l);
    }
    return 0;
}

/* Move cursor on the left. Moves by one UTF-8 character, not byte. */
void linenoise_edit_move_left(linenoise_state_t *l) {
    if (l->pos > 0) {
        l->pos -= utf8PrevCharLen(l->buf, l->pos);
        refresh_line(l);
    }
}

/* Move cursor on the right. Moves by one UTF-8 character, not byte. */
void linenoise_edit_move_right(linenoise_state_t *l) {
    if (l->pos != l->len) {
        l->pos += utf8NextCharLen(l->buf, l->pos, l->len);
        refresh_line(l);
    }
}

/* Move cursor to the start of the line. */
void linenoise_edit_move_home(linenoise_state_t *l) {
    if (l->pos != 0) {
        l->pos = 0;
        refresh_line(l);
    }
}

/* Move cursor to the end of the line. */
void linenoise_edit_move_end(linenoise_state_t *l) {
    if (l->pos != l->len) {
        l->pos = l->len;
        refresh_line(l);
    }
}

/* Move cursor to a specific display column within the buffer.
 * col is 0-based column position relative to start of buffer (not prompt).
 * Used for mouse click positioning. */
static void linenoise_edit_move_to_column(linenoise_state_t *l, int col) {
    size_t pos = 0;
    int current_col = 0;

    /* Walk through the buffer counting display columns until we reach
     * the target column or end of buffer. */
    while (pos < l->len && current_col < col) {
        size_t clen = utf8NextCharLen(l->buf, pos, l->len);
        int cwidth = utf8SingleCharWidth(l->buf + pos, clen);
        /* If this character would put us past the target, stop before it. */
        if (current_col + cwidth > col) {
            break;
        }
        current_col += cwidth;
        pos += clen;
    }

    if (l->pos != pos) {
        l->pos = pos;
        refresh_line(l);
    }
}

/* Substitute the currently edited line with the next or previous history
 * entry as specified by 'dir'. */
#define LINENOISE_HISTORY_NEXT 0
#define LINENOISE_HISTORY_PREV 1
void linenoise_edit_history_next(linenoise_state_t *l, int dir) {
    if (history_len > 1) {
        /* Update the current history entry before to
         * overwrite it with the next one. */
        ln_free(history[history_len - 1 - l->history_index]);
        history[history_len - 1 - l->history_index] = ln_strdup(l->buf);
        /* Show the new entry */
        l->history_index += (dir == LINENOISE_HISTORY_PREV) ? 1 : -1;
        if (l->history_index < 0) {
            l->history_index = 0;
            return;
        } else if (l->history_index >= history_len) {
            l->history_index = history_len-1;
            return;
        }
        strncpy(l->buf,history[history_len - 1 - l->history_index],l->buflen);
        l->buf[l->buflen-1] = '\0';
        l->len = l->pos = strlen(l->buf);
        refresh_line(l);
    }
}

/* Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key.
 * Now handles multi-byte UTF-8 characters. */
void linenoise_edit_delete(linenoise_state_t *l) {
    if (l->len > 0 && l->pos < l->len) {
        size_t clen = utf8NextCharLen(l->buf, l->pos, l->len);
        memmove(l->buf+l->pos, l->buf+l->pos+clen, l->len-l->pos-clen);
        l->len -= clen;
        l->buf[l->len] = '\0';
        refresh_line(l);
    }
}

/* Backspace implementation. Deletes the UTF-8 character before the cursor. */
void linenoise_edit_backspace(linenoise_state_t *l) {
    if (l->pos > 0 && l->len > 0) {
        size_t clen = utf8PrevCharLen(l->buf, l->pos);
        memmove(l->buf+l->pos-clen, l->buf+l->pos, l->len-l->pos);
        l->pos -= clen;
        l->len -= clen;
        l->buf[l->len] = '\0';
        refresh_line(l);
    }
}

/* Delete the previous word, maintaining the cursor at the start of the
 * current word. Handles UTF-8 by moving character-by-character. */
void linenoise_edit_delete_prev_word(linenoise_state_t *l) {
    size_t old_pos = l->pos;
    size_t diff;

    /* Skip spaces before the word (move backwards by UTF-8 chars). */
    while (l->pos > 0 && l->buf[l->pos-1] == ' ')
        l->pos -= utf8PrevCharLen(l->buf, l->pos);
    /* Skip non-space characters (move backwards by UTF-8 chars). */
    while (l->pos > 0 && l->buf[l->pos-1] != ' ')
        l->pos -= utf8PrevCharLen(l->buf, l->pos);
    diff = old_pos - l->pos;
    memmove(l->buf+l->pos, l->buf+old_pos, l->len-old_pos+1);
    l->len -= diff;
    refresh_line(l);
}

/* Move cursor to the start of the previous word. */
void linenoise_edit_move_word_left(linenoise_state_t *l) {
    if (l->pos == 0) return;
    /* Skip spaces before the word. */
    while (l->pos > 0 && l->buf[l->pos-1] == ' ')
        l->pos -= utf8PrevCharLen(l->buf, l->pos);
    /* Skip non-space characters. */
    while (l->pos > 0 && l->buf[l->pos-1] != ' ')
        l->pos -= utf8PrevCharLen(l->buf, l->pos);
    refresh_line(l);
}

/* Move cursor to the end of the next word. */
void linenoise_edit_move_word_right(linenoise_state_t *l) {
    if (l->pos >= l->len) return;
    /* Skip current word characters. */
    while (l->pos < l->len && l->buf[l->pos] != ' ')
        l->pos += utf8NextCharLen(l->buf, l->pos, l->len);
    /* Skip spaces after the word. */
    while (l->pos < l->len && l->buf[l->pos] == ' ')
        l->pos += utf8NextCharLen(l->buf, l->pos, l->len);
    refresh_line(l);
}

/* Delete the word to the right of the cursor. */
void linenoise_edit_delete_word_right(linenoise_state_t *l) {
    size_t old_pos = l->pos;
    size_t diff;

    if (l->pos >= l->len) return;
    /* Skip current word characters. */
    while (l->pos < l->len && l->buf[l->pos] != ' ')
        l->pos += utf8NextCharLen(l->buf, l->pos, l->len);
    /* Skip spaces after the word. */
    while (l->pos < l->len && l->buf[l->pos] == ' ')
        l->pos += utf8NextCharLen(l->buf, l->pos, l->len);
    diff = l->pos - old_pos;
    memmove(l->buf+old_pos, l->buf+l->pos, l->len-l->pos+1);
    l->len -= diff;
    l->pos = old_pos;
    refresh_line(l);
}

/* ======================= Undo/Redo Support ================================= */

#define LINENOISE_UNDO_MAX 100  /* Maximum undo stack size */

/* Undo entry structure. */
typedef struct undo_entry {
    char *buf;          /* Buffer content snapshot */
    size_t len;         /* Buffer length */
    size_t pos;         /* Cursor position */
} undo_entry_t;

/* Undo state - stored per editing state (simplified: using static for now). */
static undo_entry_t *undo_stack = NULL;
static int undo_stack_len = 0;
static int undo_stack_idx = 0;  /* Current position in stack */
static int undo_stack_cap = 0;

/* Save current state to undo stack. */
static void undo_save(linenoise_state_t *l) {
    undo_entry_t *entry;

    /* Initialize stack if needed. */
    if (undo_stack == NULL) {
        undo_stack_cap = LINENOISE_UNDO_MAX;
        undo_stack = ln_malloc(sizeof(undo_entry_t) * undo_stack_cap);
        if (undo_stack == NULL) return;
        memset(undo_stack, 0, sizeof(undo_entry_t) * undo_stack_cap);
    }

    /* Discard any redo entries after current position. */
    while (undo_stack_len > undo_stack_idx) {
        undo_stack_len--;
        ln_free(undo_stack[undo_stack_len].buf);
        undo_stack[undo_stack_len].buf = NULL;
    }

    /* If stack is full, remove oldest entry. */
    if (undo_stack_len >= undo_stack_cap) {
        ln_free(undo_stack[0].buf);
        memmove(undo_stack, undo_stack + 1, sizeof(undo_entry_t) * (undo_stack_cap - 1));
        undo_stack_len--;
        undo_stack_idx--;
    }

    /* Save current state. */
    entry = &undo_stack[undo_stack_len];
    entry->buf = ln_malloc(l->len + 1);
    if (entry->buf == NULL) return;
    memcpy(entry->buf, l->buf, l->len + 1);
    entry->len = l->len;
    entry->pos = l->pos;
    undo_stack_len++;
    undo_stack_idx = undo_stack_len;
}

/* Undo: restore previous state. */
void linenoise_edit_undo(linenoise_state_t *l) {
    undo_entry_t *entry;

    if (undo_stack == NULL || undo_stack_idx <= 0) {
        linenoise_beep();
        return;
    }

    /* Save current state for redo if we're at the top. */
    if (undo_stack_idx == undo_stack_len) {
        undo_save(l);
        undo_stack_idx--;  /* Move back one more since undo_save incremented */
    }

    undo_stack_idx--;
    entry = &undo_stack[undo_stack_idx];

    /* Restore state. */
    if (entry->len <= l->buflen) {
        memcpy(l->buf, entry->buf, entry->len + 1);
        l->len = entry->len;
        l->pos = entry->pos;
        refresh_line(l);
    }
}

/* Redo: restore next state. */
void linenoise_edit_redo(linenoise_state_t *l) {
    undo_entry_t *entry;

    if (undo_stack == NULL || undo_stack_idx >= undo_stack_len - 1) {
        linenoise_beep();
        return;
    }

    undo_stack_idx++;
    entry = &undo_stack[undo_stack_idx];

    /* Restore state. */
    if (entry->len <= l->buflen) {
        memcpy(l->buf, entry->buf, entry->len + 1);
        l->len = entry->len;
        l->pos = entry->pos;
        refresh_line(l);
    }
}

/* Clear undo stack (call when starting new edit session). */
static void undo_clear(void) {
    if (undo_stack != NULL) {
        for (int i = 0; i < undo_stack_len; i++) {
            ln_free(undo_stack[i].buf);
        }
        ln_free(undo_stack);
        undo_stack = NULL;
    }
    undo_stack_len = 0;
    undo_stack_idx = 0;
    undo_stack_cap = 0;
}

/* Saved state for non-blocking API context swapping. */
static struct {
    int active;
    int maskmode;
    int mlmode;
    int mousemode;
    linenoise_completion_cb_t *completion_callback;
    linenoise_hints_cb_t *hints_callback;
    linenoise_free_hints_cb_t *free_hints_callback;
    linenoise_highlight_cb_t *highlight_callback;
    int history_len;
    int history_max_len;
    char **history;
    linenoise_context_t *ctx;  /* Remember the context to update history */
} edit_saved_state = {0};

/* Internal: Start editing (operates on global state). */
static int edit_start(linenoise_state_t *l, int stdin_fd, int stdout_fd, char *buf, size_t buflen, const char *prompt) {
    /* Clear undo stack for new edit session. */
    undo_clear();

    /* Populate the linenoise state that we pass to functions implementing
     * specific editing functionalities. */
    l->in_completion = 0;
    l->ifd = stdin_fd != -1 ? stdin_fd : STDIN_FILENO;
    l->ofd = stdout_fd != -1 ? stdout_fd : STDOUT_FILENO;
    l->buf = buf;
    l->buflen = buflen;
    l->buf_dynamic = 0;  /* Fixed buffer by default */
    l->prompt = prompt;
    l->plen = strlen(prompt);
    l->oldpos = l->pos = 0;
    l->len = 0;

    /* Enter raw mode. */
    if (enable_raw_mode(l->ifd) == -1) return -1;

    /* Enable mouse tracking if requested. */
    if (mousemode) {
        enable_mouse_tracking(l->ofd);
    }

    l->cols = get_columns(stdin_fd, stdout_fd);
    l->oldrows = 0;
    l->oldrpos = 1;  /* Cursor starts on row 1. */
    l->history_index = 0;

    /* Buffer starts empty. */
    l->buf[0] = '\0';
    l->buflen--; /* Make sure there is always space for the nulterm */

    /* If stdin is not a tty, stop here with the initialization. We
     * will actually just read a line from standard input in blocking
     * mode later, in linenoise_edit_feed(). */
    if (!isatty(l->ifd) && !getenv("LINENOISE_ASSUME_TTY")) return 0;

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    history_add("");

    if (write(l->ofd,prompt,l->plen) == -1) return -1;
    return 0;
}

/* Public: Start editing with context support.
 * This is part of the multiplexed API of Linenoise, used for event-driven
 * programs. The context's settings are used for the editing session.
 *
 * Returns 0 on success, -1 on error. */
int linenoise_edit_start(linenoise_context_t *ctx, linenoise_state_t *l,
                         int stdin_fd, int stdout_fd,
                         char *buf, size_t buflen, const char *prompt) {
    if (!ctx) return -1;

    /* Save current global state. */
    edit_saved_state.active = 1;
    edit_saved_state.maskmode = maskmode;
    edit_saved_state.mlmode = mlmode;
    edit_saved_state.mousemode = mousemode;
    edit_saved_state.completion_callback = completion_callback;
    edit_saved_state.hints_callback = hints_callback;
    edit_saved_state.free_hints_callback = free_hints_callback;
    edit_saved_state.highlight_callback = highlight_callback;
    edit_saved_state.history_len = history_len;
    edit_saved_state.history_max_len = history_max_len;
    edit_saved_state.history = history;
    edit_saved_state.ctx = ctx;

    /* Set global state from context. */
    maskmode = ctx->maskmode;
    mlmode = ctx->mlmode;
    mousemode = ctx->mousemode;
    completion_callback = ctx->completion_callback;
    hints_callback = ctx->hints_callback;
    free_hints_callback = ctx->free_hints_callback;
    highlight_callback = ctx->highlight_callback;
    history_len = ctx->history_len;
    history_max_len = ctx->history_max_len;
    history = ctx->history;

    return edit_start(l, stdin_fd, stdout_fd, buf, buflen, prompt);
}

/* Start editing with a dynamically-sized buffer.
 * Unlike linenoise_edit_start(), this allocates its own buffer that grows
 * automatically as needed. The buffer is freed when linenoise_edit_stop()
 * is called, and the returned line from linenoise_edit_feed() should be
 * freed with linenoise_free().
 *
 * initial_size: Starting buffer size (will grow as needed). Use 0 for default.
 * Returns 0 on success, -1 on error. */
int linenoise_edit_start_dynamic(linenoise_context_t *ctx, linenoise_state_t *l,
                                 int stdin_fd, int stdout_fd,
                                 size_t initial_size, const char *prompt) {
    char *buf;

    if (!ctx) return -1;

    /* Use default initial size if not specified. */
    if (initial_size == 0) {
        initial_size = 256;
    }

    /* Allocate the initial buffer. */
    buf = ln_malloc(initial_size);
    if (buf == NULL) {
        set_error(LINENOISE_ERR_MEMORY);
        return -1;
    }
    buf[0] = '\0';

    /* Start editing with the allocated buffer. */
    int result = linenoise_edit_start(ctx, l, stdin_fd, stdout_fd, buf, initial_size, prompt);
    if (result == -1) {
        ln_free(buf);
        return -1;
    }

    /* Mark buffer as dynamic so it will be auto-grown and freed. */
    l->buf_dynamic = 1;
    return 0;
}

char *linenoise_edit_more = "If you see this, you are misusing the API: when linenoise_edit_feed() is called, if it returns linenoise_edit_more the user is yet editing the line. See the README file for more information.";

/* This function is part of the multiplexed API of linenoise, see the top
 * comment on linenoise_edit_start() for more information. Call this function
 * each time there is some data to read from the standard input file
 * descriptor. In the case of blocking operations, this function can just be
 * called in a loop, and block.
 *
 * The function returns linenoise_edit_more to signal that line editing is still
 * in progress, that is, the user didn't yet pressed enter / CTRL-D. Otherwise
 * the function returns the pointer to the heap-allocated buffer with the
 * edited line, that the user should free with linenoise_free().
 *
 * On special conditions, NULL is returned and errno is populated:
 *
 * EAGAIN if the user pressed Ctrl-C
 * ENOENT if the user pressed Ctrl-D
 *
 * Some other errno: I/O error.
 */
char *linenoise_edit_feed(linenoise_state_t *l) {
    /* Not a TTY, pass control to line reading without character
     * count limits. */
    if (!isatty(l->ifd) && !getenv("LINENOISE_ASSUME_TTY")) return linenoise_no_tty();

    char c;
    int nread;
    char seq[8];  /* Enough for extended sequences like ESC [ 1 ; 5 C */

    nread = read(l->ifd,&c,1);
    if (nread < 0) {
        return (errno == EAGAIN || errno == EWOULDBLOCK) ? linenoise_edit_more : NULL;
    } else if (nread == 0) {
        return NULL;
    }

    /* Handle Tab key: on continuation lines insert spaces for indentation,
     * otherwise trigger completion if callback is set. */
    if (c == 9 && !l->in_completion) {
        /* Check if cursor is on a continuation line (after a newline). */
        int on_continuation_line = 0;
        size_t i;
        for (i = 0; i < l->pos; i++) {
            if (l->buf[i] == '\n') {
                on_continuation_line = 1;
                break;
            }
        }
        if (on_continuation_line) {
            /* Insert 4 spaces for indentation. */
            if (linenoise_edit_insert(l, "    ", 4)) return NULL;
            return linenoise_edit_more;
        }
    }

    /* Autocomplete when the callback is set. It returns < 0 when
     * there was an error reading from fd. Otherwise it will return the
     * character that should be handled next. */
    if ((l->in_completion || c == 9) && completion_callback != NULL) {
        c = complete_line(l,c);
        /* Return on errors */
        if (c < 0) return NULL;
        /* Read next character when 0 */
        if (c == 0) return linenoise_edit_more;
    }

    switch(c) {
    case ENTER:    /* enter */
        history_len--;
        ln_free(history[history_len]);
        if (mlmode) linenoise_edit_move_end(l);
        if (hints_callback) {
            /* Force a refresh without hints to leave the previous
             * line as the user typed it after a newline. */
            linenoise_hints_cb_t *hc = hints_callback;
            hints_callback = NULL;
            refresh_line(l);
            hints_callback = hc;
        }
        return ln_strdup(l->buf);
    case CTRL_C:     /* ctrl-c */
        errno = EAGAIN;
        set_error(LINENOISE_ERR_INTERRUPTED);
        return NULL;
    case BACKSPACE:   /* backspace */
    case 8:     /* ctrl-h */
        undo_save(l);
        linenoise_edit_backspace(l);
        break;
    case CTRL_D:     /* ctrl-d, remove char at right of cursor, or if the
                        line is empty, act as end-of-file. */
        if (l->len > 0) {
            undo_save(l);
            linenoise_edit_delete(l);
        } else {
            history_len--;
            ln_free(history[history_len]);
            errno = ENOENT;
            set_error(LINENOISE_ERR_EOF);
            return NULL;
        }
        break;
    case CTRL_T:    /* ctrl-t, swaps current character with previous. */
        /* Handle UTF-8: swap the two UTF-8 characters around cursor. */
        if (l->pos > 0 && l->pos < l->len) {
            char tmp[32];
            size_t prevlen = utf8PrevCharLen(l->buf, l->pos);
            size_t currlen = utf8NextCharLen(l->buf, l->pos, l->len);
            size_t prevstart = l->pos - prevlen;
            undo_save(l);
            /* Copy current char to tmp, move previous char right, paste tmp. */
            memcpy(tmp, l->buf + l->pos, currlen);
            memmove(l->buf + prevstart + currlen, l->buf + prevstart, prevlen);
            memcpy(l->buf + prevstart, tmp, currlen);
            if (l->pos + currlen <= l->len) l->pos += currlen;
            refresh_line(l);
        }
        break;
    case CTRL_B:     /* ctrl-b */
        linenoise_edit_move_left(l);
        break;
    case CTRL_F:     /* ctrl-f */
        linenoise_edit_move_right(l);
        break;
    case CTRL_P:    /* ctrl-p */
        linenoise_edit_history_next(l, LINENOISE_HISTORY_PREV);
        break;
    case CTRL_N:    /* ctrl-n */
        linenoise_edit_history_next(l, LINENOISE_HISTORY_NEXT);
        break;
    case ESC:    /* escape sequence */
        /* Read the next two bytes representing the escape sequence.
         * Use timeout to avoid hanging on partial sequences (e.g., user
         * pressing ESC alone). 100ms is enough for terminal responses. */
        if (read_byte_with_timeout(l->ifd,seq,100) != 1) break;
        if (read_byte_with_timeout(l->ifd,seq+1,100) != 1) break;

        /* ESC [ sequences. */
        if (seq[0] == '[') {
            /* SGR mouse event: ESC [ < button ; x ; y M/m */
            if (seq[1] == '<' && mousemode) {
                /* Read the rest of the mouse sequence. */
                char mouse_seq[32];
                int mi = 0;
                while (mi < 30) {
                    if (read_byte_with_timeout(l->ifd, &mouse_seq[mi], 100) != 1) break;
                    if (mouse_seq[mi] == 'M' || mouse_seq[mi] == 'm') {
                        mouse_seq[mi+1] = '\0';
                        break;
                    }
                    mi++;
                }
                if (mi > 0 && (mouse_seq[mi] == 'M' || mouse_seq[mi] == 'm')) {
                    /* Parse: button;x;y */
                    int button = 0, x = 0, y = 0;
                    char *p = mouse_seq;
                    button = atoi(p);
                    while (*p && *p != ';') p++;
                    if (*p == ';') { p++; x = atoi(p); }
                    while (*p && *p != ';') p++;
                    if (*p == ';') { p++; y = atoi(p); }
                    (void)y;  /* Row not used for single-line mode */

                    /* Handle left button press (button 0) for click-to-position.
                     * x is 1-based terminal column. Subtract prompt width to get
                     * position within the edit buffer. */
                    if (button == 0 && mouse_seq[mi] == 'M') {
                        int pwidth = (int)utf8StrWidth(l->prompt, l->plen);
                        int col = x - 1 - pwidth;  /* Convert to 0-based buffer column */
                        if (col >= 0) {
                            linenoise_edit_move_to_column(l, col);
                        } else {
                            /* Click was on prompt, move to start. */
                            linenoise_edit_move_home(l);
                        }
                    }
                }
            }
            else if (seq[1] >= '0' && seq[1] <= '9') {
                /* Extended escape, read additional byte. */
                if (read_byte_with_timeout(l->ifd,seq+2,100) != 1) break;
                if (seq[2] == '~') {
                    switch(seq[1]) {
                    case '3': /* Delete key. */
                        undo_save(l);
                        linenoise_edit_delete(l);
                        break;
                    case '5': /* Page Up - treat as history prev for now */
                        linenoise_edit_history_next(l, LINENOISE_HISTORY_PREV);
                        break;
                    case '6': /* Page Down - treat as history next for now */
                        linenoise_edit_history_next(l, LINENOISE_HISTORY_NEXT);
                        break;
                    }
                } else if (seq[2] == ';') {
                    /* Modified key sequence: ESC [ 1 ; <mod> <key> */
                    if (read_byte_with_timeout(l->ifd,seq+3,100) != 1) break;
                    if (read_byte_with_timeout(l->ifd,seq+4,100) != 1) break;
                    int modifier = seq[3] - '0';
                    /* modifier: 2=Shift, 3=Alt, 5=Ctrl, 7=Ctrl+Alt */
                    if (modifier == 5 || modifier == 3) {  /* Ctrl or Alt */
                        switch(seq[4]) {
                        case 'C': /* Ctrl/Alt+Right - word right */
                            linenoise_edit_move_word_right(l);
                            break;
                        case 'D': /* Ctrl/Alt+Left - word left */
                            linenoise_edit_move_word_left(l);
                            break;
                        }
                    }
                }
            } else {
                switch(seq[1]) {
                case 'A': /* Up */
                    linenoise_edit_history_next(l, LINENOISE_HISTORY_PREV);
                    break;
                case 'B': /* Down */
                    linenoise_edit_history_next(l, LINENOISE_HISTORY_NEXT);
                    break;
                case 'C': /* Right */
                    linenoise_edit_move_right(l);
                    break;
                case 'D': /* Left */
                    linenoise_edit_move_left(l);
                    break;
                case 'H': /* Home */
                    linenoise_edit_move_home(l);
                    break;
                case 'F': /* End*/
                    linenoise_edit_move_end(l);
                    break;
                }
            }
        }

        /* ESC O sequences. */
        else if (seq[0] == 'O') {
            switch(seq[1]) {
            case 'H': /* Home */
                linenoise_edit_move_home(l);
                break;
            case 'F': /* End*/
                linenoise_edit_move_end(l);
                break;
            }
        }

        /* Alt+letter sequences (ESC followed by letter). */
        else if (seq[0] == 'b' || seq[0] == 'B') {
            /* Alt+b: move word left */
            linenoise_edit_move_word_left(l);
        }
        else if (seq[0] == 'f' || seq[0] == 'F') {
            /* Alt+f: move word right */
            linenoise_edit_move_word_right(l);
        }
        else if (seq[0] == 'd' || seq[0] == 'D') {
            /* Alt+d: delete word right */
            undo_save(l);
            linenoise_edit_delete_word_right(l);
        }
        else if (seq[0] == BACKSPACE || seq[0] == CTRL_H) {
            /* Alt+Backspace: delete previous word */
            undo_save(l);
            linenoise_edit_delete_prev_word(l);
        }
        break;
    default:
        /* Handle UTF-8 multi-byte sequences. When we receive the first byte
         * of a multi-byte UTF-8 character, read the remaining bytes to
         * complete the sequence before inserting. */
        {
            char utf8[4];
            int utf8_len = utf8ByteLen(c);
            utf8[0] = c;
            if (utf8_len > 1) {
                /* Read remaining bytes of the UTF-8 sequence. */
                int i;
                for (i = 1; i < utf8_len; i++) {
                    if (read(l->ifd, utf8+i, 1) != 1) break;
                }
            }
            if (linenoise_edit_insert(l, utf8, utf8_len)) return NULL;
        }
        break;
    case CTRL_U: /* Ctrl+u, delete the whole line. */
        undo_save(l);
        l->buf[0] = '\0';
        l->pos = l->len = 0;
        refresh_line(l);
        break;
    case CTRL_K: /* Ctrl+k, delete from current to end of line. */
        undo_save(l);
        l->buf[l->pos] = '\0';
        l->len = l->pos;
        refresh_line(l);
        break;
    case CTRL_A: /* Ctrl+a, go to the start of the line */
        linenoise_edit_move_home(l);
        break;
    case CTRL_E: /* ctrl+e, go to the end of the line */
        linenoise_edit_move_end(l);
        break;
    case CTRL_L: /* ctrl+l, clear screen */
        clear_screen();
        refresh_line(l);
        break;
    case CTRL_W: /* ctrl+w, delete previous word */
        undo_save(l);
        linenoise_edit_delete_prev_word(l);
        break;
    case CTRL_Y: /* Ctrl+y, redo */
        linenoise_edit_redo(l);
        break;
    case CTRL_Z: /* Ctrl+z, undo */
        linenoise_edit_undo(l);
        break;
    }
    return linenoise_edit_more;
}

/* Internal: Stop editing (restores terminal). */
static void edit_stop(linenoise_state_t *l) {
    if (!isatty(l->ifd) && !getenv("LINENOISE_ASSUME_TTY")) return;
    /* Disable mouse tracking before restoring terminal. */
    if (mousemode) {
        disable_mouse_tracking(l->ofd);
    }
    disable_raw_mode(l->ifd);
    printf("\n");
}

/* Public: Stop editing and restore terminal.
 * This is part of the multiplexed linenoise API. Call this when
 * linenoise_edit_feed() returns something other than linenoise_edit_more. */
void linenoise_edit_stop(linenoise_state_t *l) {
    edit_stop(l);

    /* Free dynamic buffer if allocated by linenoise_edit_start_dynamic(). */
    if (l->buf_dynamic && l->buf) {
        ln_free(l->buf);
        l->buf = NULL;
        l->buf_dynamic = 0;
    }

    /* Restore global state if we're in an active edit session. */
    if (edit_saved_state.active) {
        /* Update context history from global state. */
        if (edit_saved_state.ctx) {
            edit_saved_state.ctx->history_len = history_len;
            edit_saved_state.ctx->history = history;
        }

        /* Restore previous global state. */
        maskmode = edit_saved_state.maskmode;
        mlmode = edit_saved_state.mlmode;
        mousemode = edit_saved_state.mousemode;
        completion_callback = edit_saved_state.completion_callback;
        hints_callback = edit_saved_state.hints_callback;
        free_hints_callback = edit_saved_state.free_hints_callback;
        highlight_callback = edit_saved_state.highlight_callback;
        history_len = edit_saved_state.history_len;
        history_max_len = edit_saved_state.history_max_len;
        history = edit_saved_state.history;
        edit_saved_state.active = 0;
        edit_saved_state.ctx = NULL;
    }
}

/* This just implements a blocking loop for the multiplexed API.
 * In many applications that are not event-drivern, we can just call
 * the blocking linenoise API, wait for the user to complete the editing
 * and return the buffer. */
static char *blocking_edit(int stdin_fd, int stdout_fd, char *buf, size_t buflen, const char *prompt)
{
    linenoise_state_t l;

    /* Editing without a buffer is invalid. */
    if (buflen == 0) {
        errno = EINVAL;
        return NULL;
    }

    edit_start(&l,stdin_fd,stdout_fd,buf,buflen,prompt);
    char *res;
    while((res = linenoise_edit_feed(&l)) == linenoise_edit_more);
    edit_stop(&l);
    return res;
}

/* This special mode is used by linenoise in order to print scan codes
 * on screen for debugging / development purposes. It is implemented
 * by the linenoise_example program using the --keycodes option. */
void linenoise_print_key_codes(void) {
    char quit[4];

    printf("Linenoise key codes debugging mode.\n"
            "Press keys to see scan codes. Type 'quit' at any time to exit.\n");
    if (enable_raw_mode(STDIN_FILENO) == -1) return;
    memset(quit,' ',4);
    while(1) {
        char c;
        int nread;

        nread = read(STDIN_FILENO,&c,1);
        if (nread <= 0) continue;
        memmove(quit,quit+1,sizeof(quit)-1); /* shift string to left. */
        quit[sizeof(quit)-1] = c; /* Insert current char on the right. */
        if (memcmp(quit,"quit",sizeof(quit)) == 0) break;

        printf("'%c' %02x (%d) (type quit to exit)\n",
            isprint(c) ? c : '?', (int)c, (int)c);
        printf("\r"); /* Go left edge manually, we are in raw mode. */
        fflush(stdout);
    }
    disable_raw_mode(STDIN_FILENO);
}

/* This function is called when linenoise() is called with the standard
 * input file descriptor not attached to a TTY. So for example when the
 * program using linenoise is called in pipe or with a file redirected
 * to its standard input. In this case, we want to be able to return the
 * line regardless of its length (by default we are limited to 4k). */
static char *linenoise_no_tty(void) {
    char *line = NULL;
    size_t len = 0, maxlen = 0;

    while(1) {
        if (len == maxlen) {
            if (maxlen == 0) maxlen = 16;
            maxlen *= 2;
            char *oldval = line;
            line = ln_realloc(line,maxlen);
            if (line == NULL) {
                if (oldval) ln_free(oldval);
                return NULL;
            }
        }
        int c = fgetc(stdin);
        if (c == EOF || c == '\n') {
            if (c == EOF && len == 0) {
                ln_free(line);
                set_error(LINENOISE_ERR_EOF);
                return NULL;
            } else {
                line[len] = '\0';
                return line;
            }
        } else {
            line[len] = c;
            len++;
        }
    }
}

/* Internal: The high level line reading function using global state.
 * This function checks if the terminal has basic capabilities, just checking
 * for a blacklist of stupid terminals, and later either calls the line
 * editing function or uses dummy fgets() so that you will be able to type
 * something even in the most desperate of the conditions. */
static char *read_line(const char *prompt) {
    char buf[LINENOISE_MAX_LINE];

    if (!isatty(STDIN_FILENO) && !getenv("LINENOISE_ASSUME_TTY")) {
        /* Not a tty: read from file / pipe. In this mode we don't want any
         * limit to the line size, so we call a function to handle that. */
        return linenoise_no_tty();
    } else if (is_unsupported_term()) {
        size_t len;

        printf("%s",prompt);
        fflush(stdout);
        if (fgets(buf,LINENOISE_MAX_LINE,stdin) == NULL) {
            set_error(LINENOISE_ERR_EOF);
            return NULL;
        }
        len = strlen(buf);
        while(len && (buf[len-1] == '\n' || buf[len-1] == '\r')) {
            len--;
            buf[len] = '\0';
        }
        return ln_strdup(buf);
    } else {
        char *retval = blocking_edit(STDIN_FILENO,STDOUT_FILENO,buf,LINENOISE_MAX_LINE,prompt);
        return retval;
    }
}

/* This is just a wrapper the user may want to call in order to make sure
 * the linenoise returned buffer is freed with the same allocator it was
 * created with. Useful when the main program is using an alternative
 * allocator. */
void linenoise_free(void *ptr) {
    if (ptr == linenoise_edit_more) return; /* Protect from API misuse. */
    ln_free(ptr);
}

/* ================================ History ================================= */

/* Free the history, but does not reset it. Only used when we have to
 * exit() to avoid memory leaks are reported by valgrind & co. */
static void free_history(void) {
    if (history) {
        int j;

        for (j = 0; j < history_len; j++)
            ln_free(history[j]);
        ln_free(history);
    }
}

/* At exit we'll try to fix the terminal to the initial conditions. */
static void linenoise_at_exit(void) {
    disable_raw_mode(STDIN_FILENO);
    free_history();
}

/* Internal: Add a new entry to the global history.
 * It uses a fixed array of char pointers that are shifted (memmoved)
 * when the history max length is reached in order to remove the older
 * entry and make room for the new one, so it is not exactly suitable for huge
 * histories, but will work well for a few hundred of entries.
 *
 * Using a circular buffer is smarter, but a bit more complex to handle. */
static int history_add(const char *line) {
    char *linecopy;

    if (history_max_len == 0) return 0;

    /* Initialization on first call. */
    if (history == NULL) {
        history = ln_malloc(sizeof(char*)*history_max_len);
        if (history == NULL) return 0;
        memset(history,0,(sizeof(char*)*history_max_len));
    }

    /* Don't add duplicated lines. */
    if (history_len && !strcmp(history[history_len-1], line)) return 0;

    /* Add an heap allocated copy of the line in the history.
     * If we reached the max length, remove the older line. */
    linecopy = ln_strdup(line);
    if (!linecopy) return 0;
    if (history_len == history_max_len) {
        ln_free(history[0]);
        memmove(history,history+1,sizeof(char*)*(history_max_len-1));
        history_len--;
    }
    history[history_len] = linecopy;
    history_len++;
    return 1;
}

/* ========================= Context-based API ============================== */

/* Create a new linenoise context with default settings. */
linenoise_context_t *linenoise_context_create(void) {
    linenoise_context_t *ctx = ln_malloc(sizeof(linenoise_context_t));
    if (!ctx) return NULL;

#ifdef _WIN32
    ctx->orig_console_mode = 0;
#else
    memset(&ctx->orig_termios, 0, sizeof(ctx->orig_termios));
#endif
    ctx->rawmode = 0;
    ctx->atexit_registered = 0;
    ctx->maskmode = 0;
    ctx->mlmode = 0;
    ctx->mousemode = 0;
    ctx->history_max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
    ctx->history_len = 0;
    ctx->history = NULL;
    ctx->completion_callback = NULL;
    ctx->hints_callback = NULL;
    ctx->free_hints_callback = NULL;
    ctx->highlight_callback = NULL;

    return ctx;
}

/* Destroy a linenoise context and free all associated resources. */
void linenoise_context_destroy(linenoise_context_t *ctx) {
    if (!ctx) return;

    /* Free history. */
    if (ctx->history) {
        for (int j = 0; j < ctx->history_len; j++) {
            ln_free(ctx->history[j]);
        }
        ln_free(ctx->history);
    }

    ln_free(ctx);
}

/* Set multi-line mode for a context. */
void linenoise_set_multiline(linenoise_context_t *ctx, int ml) {
    if (ctx) ctx->mlmode = ml;
}

/* Set mask mode for a context (password entry). */
void linenoise_set_mask_mode(linenoise_context_t *ctx, int enable) {
    if (ctx) ctx->maskmode = enable;
}

/* Set mouse mode for a context (click to position cursor). */
void linenoise_set_mouse_mode(linenoise_context_t *ctx, int enable) {
    if (ctx) ctx->mousemode = enable;
}

/* Set completion callback for a context. */
void linenoise_set_completion_callback(linenoise_context_t *ctx, linenoise_completion_cb_t *fn) {
    if (ctx) ctx->completion_callback = fn;
}

/* Set hints callback for a context. */
void linenoise_set_hints_callback(linenoise_context_t *ctx, linenoise_hints_cb_t *fn) {
    if (ctx) ctx->hints_callback = fn;
}

/* Set free hints callback for a context. */
void linenoise_set_free_hints_callback(linenoise_context_t *ctx, linenoise_free_hints_cb_t *fn) {
    if (ctx) ctx->free_hints_callback = fn;
}

/* Set syntax highlighting callback for a context. */
void linenoise_set_highlight_callback(linenoise_context_t *ctx, linenoise_highlight_cb_t *fn) {
    if (ctx) ctx->highlight_callback = fn;
}

/* Add a line to history for a context. */
int linenoise_history_add(linenoise_context_t *ctx, const char *line) {
    char *linecopy;

    if (!ctx) return 0;
    if (ctx->history_max_len == 0) return 0;

    /* Don't add duplicates of the previous line. */
    if (ctx->history_len && !strcmp(ctx->history[ctx->history_len-1], line))
        return 0;

    linecopy = ln_strdup(line);
    if (!linecopy) return 0;

    if (ctx->history == NULL) {
        ctx->history = ln_malloc(sizeof(char*) * ctx->history_max_len);
        if (ctx->history == NULL) {
            ln_free(linecopy);
            return 0;
        }
        memset(ctx->history, 0, sizeof(char*) * ctx->history_max_len);
    }

    if (ctx->history_len == ctx->history_max_len) {
        ln_free(ctx->history[0]);
        memmove(ctx->history, ctx->history + 1, sizeof(char*) * (ctx->history_max_len - 1));
        ctx->history_len--;
    }
    ctx->history[ctx->history_len] = linecopy;
    ctx->history_len++;
    return 1;
}

/* Set maximum history length for a context. */
int linenoise_history_set_max_len(linenoise_context_t *ctx, int len) {
    char **new_history;

    if (!ctx) return 0;
    if (len < 1) return 0;

    if (ctx->history) {
        int tocopy = ctx->history_len;
        new_history = ln_malloc(sizeof(char*) * len);
        if (new_history == NULL) return 0;

        if (len < tocopy) {
            int j;
            for (j = 0; j < tocopy - len; j++)
                ln_free(ctx->history[j]);
            tocopy = len;
        }
        memset(new_history, 0, sizeof(char*) * len);
        memcpy(new_history, ctx->history + (ctx->history_len - tocopy),
               sizeof(char*) * tocopy);
        ln_free(ctx->history);
        ctx->history = new_history;
    }
    ctx->history_max_len = len;
    if (ctx->history_len > ctx->history_max_len)
        ctx->history_len = ctx->history_max_len;
    return 1;
}

/* Save history to file for a context. */
int linenoise_history_save(linenoise_context_t *ctx, const char *filename) {
    int fd;
    FILE *fp;

    if (!ctx) return -1;

    fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    if (fd == -1) return -1;

    fp = fdopen(fd, "w");
    if (fp == NULL) {
        close(fd);
        return -1;
    }
    for (int j = 0; j < ctx->history_len; j++)
        fprintf(fp, "%s\n", ctx->history[j]);
    fclose(fp);
    return 0;
}

/* Load history from file for a context. */
int linenoise_history_load(linenoise_context_t *ctx, const char *filename) {
    FILE *fp;
    char buf[LINENOISE_MAX_LINE];

    if (!ctx) return -1;

    fp = fopen(filename, "r");
    if (fp == NULL) return -1;

    while (fgets(buf, LINENOISE_MAX_LINE, fp) != NULL) {
        char *p;
        p = strchr(buf, '\r');
        if (!p) p = strchr(buf, '\n');
        if (p) *p = '\0';
        linenoise_history_add(ctx, buf);
    }
    fclose(fp);
    return 0;
}

/* Main line editing function using a context.
 * Note: This is a simplified version that uses the global state internally
 * but respects the context's settings. A full implementation would require
 * threading the context through all internal functions. */
char *linenoise_read(linenoise_context_t *ctx, const char *prompt) {
    if (!ctx) return NULL;

    /* Temporarily set global state from context for backward compatibility
     * with internal functions. This is a transitional approach. */
    int saved_maskmode = maskmode;
    int saved_mlmode = mlmode;
    int saved_mousemode = mousemode;
    linenoise_completion_cb_t *saved_completion = completion_callback;
    linenoise_hints_cb_t *saved_hints = hints_callback;
    linenoise_free_hints_cb_t *saved_freehints = free_hints_callback;
    linenoise_highlight_cb_t *saved_highlight = highlight_callback;

    maskmode = ctx->maskmode;
    mlmode = ctx->mlmode;
    mousemode = ctx->mousemode;
    completion_callback = ctx->completion_callback;
    hints_callback = ctx->hints_callback;
    free_hints_callback = ctx->free_hints_callback;
    highlight_callback = ctx->highlight_callback;

    /* Also temporarily swap history. */
    int saved_history_len = history_len;
    int saved_history_max_len = history_max_len;
    char **saved_history = history;
    history_len = ctx->history_len;
    history_max_len = ctx->history_max_len;
    history = ctx->history;

    /* Call the internal line reading function. */
    char *result = read_line(prompt);

    /* Copy back any history changes. */
    ctx->history_len = history_len;
    ctx->history = history;

    /* Restore global state. */
    maskmode = saved_maskmode;
    mlmode = saved_mlmode;
    mousemode = saved_mousemode;
    completion_callback = saved_completion;
    hints_callback = saved_hints;
    free_hints_callback = saved_freehints;
    highlight_callback = saved_highlight;
    history_len = saved_history_len;
    history_max_len = saved_history_max_len;
    history = saved_history;

    return result;
}

/* Clear the screen using the context's output (currently just uses stdout). */
void linenoise_clear_screen(linenoise_context_t *ctx) {
    (void)ctx;  /* Currently unused, but kept for API consistency. */
    clear_screen();
}
