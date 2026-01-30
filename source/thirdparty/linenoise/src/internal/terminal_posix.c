/* terminal_posix.c -- POSIX terminal implementation.
 *
 * This module implements the terminal abstraction for POSIX systems
 * (Linux, macOS, BSD, etc.) using termios and standard POSIX APIs.
 */

#include "terminal.h"

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>

/* Terminal context for POSIX systems. */
struct linenoise_terminal {
    int input_fd;               /* Input file descriptor (usually stdin) */
    int output_fd;              /* Output file descriptor (usually stdout) */
    struct termios orig_termios;/* Original terminal settings */
    int rawmode;                /* 1 if in raw mode */
    int atexit_registered;      /* 1 if atexit handler registered */
};

/* Global for atexit cleanup - we need to track one terminal for cleanup. */
static linenoise_terminal_t *g_atexit_terminal = NULL;

/* atexit handler to restore terminal on exit. */
static void terminal_atexit_handler(void) {
    if (g_atexit_terminal && g_atexit_terminal->rawmode) {
        terminal_disable_raw(g_atexit_terminal);
    }
}

linenoise_terminal_t *terminal_create(void) {
    return terminal_create_with_fds(STDIN_FILENO, STDOUT_FILENO);
}

linenoise_terminal_t *terminal_create_with_fds(int input_fd, int output_fd) {
    linenoise_terminal_t *term = malloc(sizeof(linenoise_terminal_t));
    if (!term) return NULL;

    term->input_fd = (input_fd >= 0) ? input_fd : STDIN_FILENO;
    term->output_fd = (output_fd >= 0) ? output_fd : STDOUT_FILENO;
    term->rawmode = 0;
    term->atexit_registered = 0;
    memset(&term->orig_termios, 0, sizeof(term->orig_termios));

    return term;
}

void terminal_destroy(linenoise_terminal_t *term) {
    if (!term) return;

    /* Restore terminal if still in raw mode. */
    if (term->rawmode) {
        terminal_disable_raw(term);
    }

    /* Clear global if this was the atexit terminal. */
    if (g_atexit_terminal == term) {
        g_atexit_terminal = NULL;
    }

    free(term);
}

int terminal_is_tty(linenoise_terminal_t *term) {
    if (!term) return 0;

    /* Test mode: LINENOISE_ASSUME_TTY forces TTY behavior. */
    if (getenv("LINENOISE_ASSUME_TTY")) return 1;

    return isatty(term->input_fd);
}

int terminal_enable_raw(linenoise_terminal_t *term) {
    struct termios raw;

    if (!term) return -1;

    /* Test mode: when LINENOISE_ASSUME_TTY is set, skip terminal setup.
     * This allows testing via pipes without a real terminal. */
    if (getenv("LINENOISE_ASSUME_TTY")) {
        term->rawmode = 1;
        return 0;
    }

    if (!isatty(term->input_fd)) {
        errno = ENOTTY;
        return -1;
    }

    /* Register atexit handler if not already done. */
    if (!term->atexit_registered) {
        if (g_atexit_terminal == NULL) {
            atexit(terminal_atexit_handler);
            g_atexit_terminal = term;
        }
        term->atexit_registered = 1;
    }

    if (tcgetattr(term->input_fd, &term->orig_termios) == -1) {
        errno = ENOTTY;
        return -1;
    }

    raw = term->orig_termios;

    /* Input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    /* Output modes - disable post processing. */
    raw.c_oflag &= ~(OPOST);

    /* Control modes - set 8 bit chars. */
    raw.c_cflag |= (CS8);

    /* Local modes - echo off, canonical off, no extended functions,
     * no signal chars (^Z,^C). */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    /* Control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    /* Put terminal in raw mode after flushing. */
    if (tcsetattr(term->input_fd, TCSAFLUSH, &raw) < 0) {
        errno = ENOTTY;
        return -1;
    }

    term->rawmode = 1;
    return 0;
}

int terminal_disable_raw(linenoise_terminal_t *term) {
    if (!term) return -1;

    /* Test mode: nothing to restore. */
    if (getenv("LINENOISE_ASSUME_TTY")) {
        term->rawmode = 0;
        return 0;
    }

    if (term->rawmode) {
        if (tcsetattr(term->input_fd, TCSAFLUSH, &term->orig_termios) != -1) {
            term->rawmode = 0;
            return 0;
        }
        return -1;
    }
    return 0;
}

int terminal_is_raw(linenoise_terminal_t *term) {
    return term ? term->rawmode : 0;
}

/* Helper: query cursor position using ESC [6n. */
static int get_cursor_position(linenoise_terminal_t *term) {
    char buf[32];
    int cols, rows;
    unsigned int i = 0;

    /* Report cursor location. */
    if (write(term->output_fd, "\x1b[6n", 4) != 4) return -1;

    /* Read the response: ESC [ rows ; cols R */
    while (i < sizeof(buf) - 1) {
        if (read(term->input_fd, buf + i, 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    /* Parse it. */
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(buf + 2, "%d;%d", &rows, &cols) != 2) return -1;
    return cols;
}

int terminal_get_size(linenoise_terminal_t *term, int *cols, int *rows) {
    struct winsize ws;

    if (!term) {
        if (cols) *cols = 80;
        if (rows) *rows = 24;
        return -1;
    }

    /* Test mode: use LINENOISE_COLS env var for fixed width. */
    char *cols_env = getenv("LINENOISE_COLS");
    if (cols_env) {
        if (cols) *cols = atoi(cols_env);
        if (rows) *rows = 24;  /* Default rows */
        return 0;
    }

    if (ioctl(term->output_fd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* ioctl() failed. Try to query the terminal itself. */
        int start, end;

        /* Get the initial position so we can restore it later. */
        start = get_cursor_position(term);
        if (start == -1) goto failed;

        /* Go to right margin and get position. */
        if (write(term->output_fd, "\x1b[999C", 6) != 6) goto failed;
        end = get_cursor_position(term);
        if (end == -1) goto failed;

        /* Restore position. */
        if (end > start) {
            char seq[32];
            snprintf(seq, 32, "\x1b[%dD", end - start);
            if (write(term->output_fd, seq, strlen(seq)) == -1) {
                /* Can't recover... */
            }
        }

        if (cols) *cols = end;
        if (rows) *rows = 24;  /* Can't easily get rows this way */
        return 0;
    }

    if (cols) *cols = ws.ws_col;
    if (rows) *rows = ws.ws_row;
    return 0;

failed:
    if (cols) *cols = 80;
    if (rows) *rows = 24;
    return -1;
}

int terminal_read_byte(linenoise_terminal_t *term, char *c, int timeout_ms) {
    if (!term || !c) return -1;

    if (timeout_ms == 0) {
        /* Non-blocking: use select with zero timeout. */
        fd_set readfds;
        struct timeval tv = {0, 0};

        FD_ZERO(&readfds);
        FD_SET(term->input_fd, &readfds);

        int ret = select(term->input_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret <= 0) return ret;
        return read(term->input_fd, c, 1);
    } else if (timeout_ms < 0) {
        /* Blocking: just read. */
        return read(term->input_fd, c, 1);
    } else {
        /* Timeout: use select. */
        fd_set readfds;
        struct timeval tv;

        FD_ZERO(&readfds);
        FD_SET(term->input_fd, &readfds);
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int ret = select(term->input_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret <= 0) return ret;
        return read(term->input_fd, c, 1);
    }
}

int terminal_write(linenoise_terminal_t *term, const char *buf, size_t len) {
    if (!term || !buf) return -1;
    return write(term->output_fd, buf, len);
}

void terminal_clear_screen(linenoise_terminal_t *term) {
    if (!term) return;
    if (write(term->output_fd, "\x1b[H\x1b[2J", 7) <= 0) {
        /* Nothing to do, just to avoid warning. */
    }
}

void terminal_beep(linenoise_terminal_t *term) {
    if (!term) return;
    if (write(term->output_fd, "\x07", 1) <= 0) {
        /* Nothing to do. */
    }
}

int terminal_get_input_fd(linenoise_terminal_t *term) {
    return term ? term->input_fd : -1;
}

int terminal_get_output_fd(linenoise_terminal_t *term) {
    return term ? term->output_fd : -1;
}
