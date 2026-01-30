/* terminal_windows.c -- Windows terminal implementation.
 *
 * This module implements the terminal abstraction for Windows systems.
 * It supports two modes:
 * 1. VT mode (Windows 10+): Uses ENABLE_VIRTUAL_TERMINAL_PROCESSING for
 *    ANSI escape sequence support, allowing the same escape sequences as POSIX.
 * 2. Legacy mode (pre-Win10): Uses native Console API for input/output.
 *
 * The implementation automatically detects VT mode availability and falls back
 * to legacy mode if not supported.
 */

#ifdef _WIN32

#include "terminal.h"

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

/* Windows equivalents for POSIX functions. */
#define isatty _isatty
#define fileno _fileno

/* VT mode flags (may not be defined in older Windows SDKs). */
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif
#ifndef DISABLE_NEWLINE_AUTO_RETURN
#define DISABLE_NEWLINE_AUTO_RETURN 0x0008
#endif

/* Terminal context for Windows systems. */
struct linenoise_terminal {
    HANDLE input_handle;            /* Console input handle */
    HANDLE output_handle;           /* Console output handle */
    DWORD orig_input_mode;          /* Original input console mode */
    DWORD orig_output_mode;         /* Original output console mode */
    int rawmode;                    /* 1 if in raw mode */
    int vt_mode;                    /* 1 if VT mode is available and enabled */
    int atexit_registered;          /* 1 if atexit handler registered */
};

/* Global for atexit cleanup. */
static linenoise_terminal_t *g_atexit_terminal = NULL;

/* atexit handler to restore terminal on exit. */
static void terminal_atexit_handler(void) {
    if (g_atexit_terminal && g_atexit_terminal->rawmode) {
        terminal_disable_raw(g_atexit_terminal);
    }
}

/* Check if VT mode is available by attempting to enable it. */
static int check_vt_mode_available(HANDLE output_handle) {
    DWORD mode;
    if (!GetConsoleMode(output_handle, &mode)) {
        return 0;
    }
    /* Try to enable VT processing. */
    if (!SetConsoleMode(output_handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        return 0;
    }
    /* Restore original mode. */
    SetConsoleMode(output_handle, mode);
    return 1;
}

linenoise_terminal_t *terminal_create(void) {
    return terminal_create_with_fds(-1, -1);
}

linenoise_terminal_t *terminal_create_with_fds(int input_fd, int output_fd) {
    linenoise_terminal_t *term = malloc(sizeof(linenoise_terminal_t));
    if (!term) return NULL;

    (void)input_fd;  /* Ignored on Windows - we use console handles directly. */
    (void)output_fd;

    /* Get console handles. */
    term->input_handle = GetStdHandle(STD_INPUT_HANDLE);
    term->output_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (term->input_handle == INVALID_HANDLE_VALUE ||
        term->output_handle == INVALID_HANDLE_VALUE) {
        free(term);
        return NULL;
    }

    term->rawmode = 0;
    term->atexit_registered = 0;
    term->orig_input_mode = 0;
    term->orig_output_mode = 0;

    /* Check if VT mode is available (Windows 10+). */
    term->vt_mode = check_vt_mode_available(term->output_handle);

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

    /* Check if handles are valid console handles. */
    DWORD mode;
    return GetConsoleMode(term->input_handle, &mode) != 0;
}

int terminal_enable_raw(linenoise_terminal_t *term) {
    DWORD input_mode, output_mode;

    if (!term) return -1;

    /* Test mode: when LINENOISE_ASSUME_TTY is set, skip terminal setup. */
    if (getenv("LINENOISE_ASSUME_TTY")) {
        term->rawmode = 1;
        return 0;
    }

    if (!terminal_is_tty(term)) {
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

    /* Save original modes. */
    if (!GetConsoleMode(term->input_handle, &term->orig_input_mode)) {
        return -1;
    }
    if (!GetConsoleMode(term->output_handle, &term->orig_output_mode)) {
        return -1;
    }

    /* Configure input mode:
     * - Disable line input (ENABLE_LINE_INPUT)
     * - Disable echo (ENABLE_ECHO_INPUT)
     * - Disable processed input (ENABLE_PROCESSED_INPUT) for raw Ctrl+C etc.
     * - Enable window input for resize events (optional)
     */
    input_mode = term->orig_input_mode;
    input_mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);

    /* Enable VT input if available. */
    if (term->vt_mode) {
        input_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    }

    if (!SetConsoleMode(term->input_handle, input_mode)) {
        return -1;
    }

    /* Configure output mode for VT processing if available. */
    if (term->vt_mode) {
        output_mode = term->orig_output_mode;
        output_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        output_mode |= DISABLE_NEWLINE_AUTO_RETURN;

        if (!SetConsoleMode(term->output_handle, output_mode)) {
            /* VT mode failed, restore input and try without. */
            SetConsoleMode(term->input_handle, term->orig_input_mode);
            term->vt_mode = 0;
            /* Retry without VT mode. */
            input_mode &= ~ENABLE_VIRTUAL_TERMINAL_INPUT;
            if (!SetConsoleMode(term->input_handle, input_mode)) {
                return -1;
            }
        }
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
        SetConsoleMode(term->input_handle, term->orig_input_mode);
        SetConsoleMode(term->output_handle, term->orig_output_mode);
        term->rawmode = 0;
    }
    return 0;
}

int terminal_is_raw(linenoise_terminal_t *term) {
    return term ? term->rawmode : 0;
}

int terminal_get_size(linenoise_terminal_t *term, int *cols, int *rows) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (!term) {
        if (cols) *cols = 80;
        if (rows) *rows = 24;
        return -1;
    }

    /* Test mode: use LINENOISE_COLS env var for fixed width. */
    char *cols_env = getenv("LINENOISE_COLS");
    if (cols_env) {
        if (cols) *cols = atoi(cols_env);
        if (rows) *rows = 24;
        return 0;
    }

    if (GetConsoleScreenBufferInfo(term->output_handle, &csbi)) {
        if (cols) *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        if (rows) *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return 0;
    }

    if (cols) *cols = 80;
    if (rows) *rows = 24;
    return -1;
}

int terminal_read_byte(linenoise_terminal_t *term, char *c, int timeout_ms) {
    if (!term || !c) return -1;

    if (term->vt_mode) {
        /* VT mode: use ReadFile/ReadConsole for escape sequences. */
        DWORD read_count;
        DWORD wait_result;

        if (timeout_ms == 0) {
            /* Non-blocking: check if input available. */
            DWORD events_available;
            if (!GetNumberOfConsoleInputEvents(term->input_handle, &events_available)) {
                return -1;
            }
            if (events_available == 0) {
                return 0;  /* No data available. */
            }
        } else if (timeout_ms > 0) {
            /* Timeout: wait for input with timeout. */
            wait_result = WaitForSingleObject(term->input_handle, (DWORD)timeout_ms);
            if (wait_result == WAIT_TIMEOUT) {
                return 0;  /* Timeout. */
            }
            if (wait_result != WAIT_OBJECT_0) {
                return -1;  /* Error. */
            }
        }
        /* timeout_ms < 0: blocking read, no wait needed. */

        /* Read a single character. */
        if (!ReadConsoleA(term->input_handle, c, 1, &read_count, NULL)) {
            return -1;
        }
        return (read_count > 0) ? 1 : -1;

    } else {
        /* Legacy mode: use ReadConsoleInput to handle key events. */
        INPUT_RECORD ir;
        DWORD events_read;
        DWORD wait_result;

        while (1) {
            if (timeout_ms == 0) {
                /* Non-blocking. */
                DWORD events_available;
                if (!GetNumberOfConsoleInputEvents(term->input_handle, &events_available)) {
                    return -1;
                }
                if (events_available == 0) {
                    return 0;
                }
            } else if (timeout_ms > 0) {
                /* Timeout. */
                wait_result = WaitForSingleObject(term->input_handle, (DWORD)timeout_ms);
                if (wait_result == WAIT_TIMEOUT) {
                    return 0;
                }
                if (wait_result != WAIT_OBJECT_0) {
                    return -1;
                }
            }

            if (!ReadConsoleInputA(term->input_handle, &ir, 1, &events_read)) {
                return -1;
            }

            if (events_read == 0) {
                continue;
            }

            if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) {
                continue;  /* Ignore non-key events and key-up events. */
            }

            /* Handle special keys by generating escape sequences. */
            WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
            char ascii = ir.Event.KeyEvent.uChar.AsciiChar;

            /* If we have an ASCII character, return it. */
            if (ascii != 0) {
                *c = ascii;
                return 1;
            }

            /* Generate escape sequences for special keys (arrow keys, etc.).
             * We return ESC first, then subsequent calls return the rest. */
            /* For simplicity in legacy mode, we only return basic characters.
             * Full escape sequence generation would require buffering. */
            switch (vk) {
                case VK_UP:
                    *c = '\x1b';  /* Will need buffering for full sequence. */
                    return 1;
                case VK_DOWN:
                    *c = '\x1b';
                    return 1;
                case VK_LEFT:
                    *c = '\x1b';
                    return 1;
                case VK_RIGHT:
                    *c = '\x1b';
                    return 1;
                case VK_HOME:
                    *c = '\x01';  /* Ctrl+A */
                    return 1;
                case VK_END:
                    *c = '\x05';  /* Ctrl+E */
                    return 1;
                case VK_DELETE:
                    *c = '\x1b';
                    return 1;
                default:
                    continue;  /* Ignore unknown keys. */
            }
        }
    }
}

int terminal_write(linenoise_terminal_t *term, const char *buf, size_t len) {
    if (!term || !buf) return -1;

    if (term->vt_mode) {
        /* VT mode: write directly, escape sequences are processed. */
        DWORD written;
        if (!WriteConsoleA(term->output_handle, buf, (DWORD)len, &written, NULL)) {
            return -1;
        }
        return (int)written;
    } else {
        /* Legacy mode: need to interpret escape sequences manually.
         * For simplicity, we write directly and hope for the best.
         * Full legacy support would parse and translate escape sequences. */
        DWORD written;
        if (!WriteConsoleA(term->output_handle, buf, (DWORD)len, &written, NULL)) {
            return -1;
        }
        return (int)written;
    }
}

void terminal_clear_screen(linenoise_terminal_t *term) {
    if (!term) return;

    if (term->vt_mode) {
        /* Use escape sequence. */
        terminal_write(term, "\x1b[H\x1b[2J", 7);
    } else {
        /* Use Console API. */
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        COORD coord = {0, 0};
        DWORD written;
        DWORD console_size;

        if (!GetConsoleScreenBufferInfo(term->output_handle, &csbi)) {
            return;
        }

        console_size = csbi.dwSize.X * csbi.dwSize.Y;
        FillConsoleOutputCharacterA(term->output_handle, ' ', console_size, coord, &written);
        FillConsoleOutputAttribute(term->output_handle, csbi.wAttributes, console_size, coord, &written);
        SetConsoleCursorPosition(term->output_handle, coord);
    }
}

void terminal_beep(linenoise_terminal_t *term) {
    if (!term) return;

    if (term->vt_mode) {
        terminal_write(term, "\x07", 1);
    } else {
        MessageBeep(MB_OK);
    }
}

int terminal_get_input_fd(linenoise_terminal_t *term) {
    (void)term;
    /* Windows uses handles, not file descriptors for console I/O.
     * Return -1 to indicate fd-based operations aren't supported. */
    return -1;
}

int terminal_get_output_fd(linenoise_terminal_t *term) {
    (void)term;
    return -1;
}

/* Windows-specific: get the input handle for WaitForSingleObject, etc. */
HANDLE terminal_get_input_handle(linenoise_terminal_t *term) {
    return term ? term->input_handle : INVALID_HANDLE_VALUE;
}

/* Windows-specific: check if VT mode is active. */
int terminal_is_vt_mode(linenoise_terminal_t *term) {
    return term ? term->vt_mode : 0;
}

#endif /* _WIN32 */
