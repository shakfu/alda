/* terminal_win.c - Windows terminal implementation
 *
 * Platform-specific terminal operations for Windows systems.
 * Supports two modes:
 * 1. VT mode (Windows 10+): Uses ENABLE_VIRTUAL_TERMINAL_PROCESSING for
 *    ANSI escape sequence support, same as POSIX terminals.
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
#include <ctype.h>

/* Windows equivalents for POSIX functions */
#define isatty _isatty
#define fileno _fileno
#define write _write
#define read _read

/* VT mode flags (may not be defined in older Windows SDKs) */
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif
#ifndef DISABLE_NEWLINE_AUTO_RETURN
#define DISABLE_NEWLINE_AUTO_RETURN 0x0008
#endif

/* ======================= Terminal Host State =============================== */

/* Static terminal host instance */
static TerminalHost g_terminal_host_instance = {0};

/* Global pointer for compatibility with POSIX code */
TerminalHost *g_terminal_host = &g_terminal_host_instance;

/* ======================= Helper Functions ================================== */

/* Check if VT mode is available by attempting to enable it */
static int check_vt_mode_available(HANDLE output_handle) {
    DWORD mode;
    if (!GetConsoleMode(output_handle, &mode)) {
        return 0;
    }
    /* Try to enable VT processing */
    if (!SetConsoleMode(output_handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        return 0;
    }
    /* Restore original mode */
    SetConsoleMode(output_handle, mode);
    return 1;
}

/* ======================= Terminal Host Implementation ===================== */

int terminal_host_init(TerminalHost *host, int fd) {
    (void)fd;  /* fd is ignored on Windows, we use console handles */

    if (!host) return -1;

    memset(host, 0, sizeof(*host));

    /* Get console handles */
    host->input_handle = GetStdHandle(STD_INPUT_HANDLE);
    host->output_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (host->input_handle == INVALID_HANDLE_VALUE ||
        host->output_handle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    host->rawmode = 0;
    host->winsize_changed = 0;
    host->orig_input_mode = 0;
    host->orig_output_mode = 0;

    /* Check if VT mode is available (Windows 10+) */
    host->vt_mode = check_vt_mode_available(host->output_handle);

    /* Get initial window size for resize detection */
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(host->output_handle, &csbi)) {
        host->last_cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        host->last_rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        host->last_cols = 80;
        host->last_rows = 24;
    }

    /* Set global pointer */
    g_terminal_host = host;

    return 0;
}

void terminal_host_cleanup(TerminalHost *host) {
    if (!host) return;

    /* Disable raw mode if active */
    terminal_host_disable_raw_mode(host);

    /* Clear global pointer */
    if (g_terminal_host == host) {
        g_terminal_host = NULL;
    }
}

int terminal_host_enable_raw_mode(TerminalHost *host) {
    DWORD input_mode, output_mode;

    if (!host) return -1;
    if (host->rawmode) return 0;  /* Already enabled */

    /* Check if handles are valid console handles */
    DWORD mode;
    if (!GetConsoleMode(host->input_handle, &mode)) {
        return -1;  /* Not a console */
    }

    /* Save original modes */
    if (!GetConsoleMode(host->input_handle, &host->orig_input_mode)) {
        return -1;
    }
    if (!GetConsoleMode(host->output_handle, &host->orig_output_mode)) {
        return -1;
    }

    /* Configure input mode:
     * - Disable line input (ENABLE_LINE_INPUT)
     * - Disable echo (ENABLE_ECHO_INPUT)
     * - Disable processed input (ENABLE_PROCESSED_INPUT) for raw Ctrl+C etc.
     * - Enable window input for resize events
     */
    input_mode = host->orig_input_mode;
    input_mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
    input_mode |= ENABLE_WINDOW_INPUT;  /* Get resize events */

    /* Enable VT input if available */
    if (host->vt_mode) {
        input_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    }

    if (!SetConsoleMode(host->input_handle, input_mode)) {
        return -1;
    }

    /* Configure output mode for VT processing if available */
    if (host->vt_mode) {
        output_mode = host->orig_output_mode;
        output_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        output_mode |= DISABLE_NEWLINE_AUTO_RETURN;

        if (!SetConsoleMode(host->output_handle, output_mode)) {
            /* VT mode failed, restore input and try without */
            SetConsoleMode(host->input_handle, host->orig_input_mode);
            host->vt_mode = 0;
            /* Retry without VT mode */
            input_mode &= ~ENABLE_VIRTUAL_TERMINAL_INPUT;
            if (!SetConsoleMode(host->input_handle, input_mode)) {
                return -1;
            }
        }
    }

    host->rawmode = 1;

    /* Enter alternate screen buffer (VT mode only) */
    if (host->vt_mode) {
        DWORD written;
        WriteConsoleA(host->output_handle, "\x1b[?1049h", 8, &written, NULL);
    }

    return 0;
}

void terminal_host_disable_raw_mode(TerminalHost *host) {
    if (!host || !host->rawmode) return;

    /* Exit alternate screen buffer (VT mode only) */
    if (host->vt_mode) {
        DWORD written;
        WriteConsoleA(host->output_handle, "\x1b[?1049l", 8, &written, NULL);
    }

    SetConsoleMode(host->input_handle, host->orig_input_mode);
    SetConsoleMode(host->output_handle, host->orig_output_mode);
    host->rawmode = 0;
}

int terminal_host_resize_pending(TerminalHost *host) {
    if (!host) return 0;

    /* On Windows, check for size changes by polling */
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(host->output_handle, &csbi)) {
        int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        if (cols != host->last_cols || rows != host->last_rows) {
            host->winsize_changed = 1;
        }
    }

    return host->winsize_changed;
}

void terminal_host_clear_resize(TerminalHost *host) {
    if (!host) return;

    /* Update cached size */
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(host->output_handle, &csbi)) {
        host->last_cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        host->last_rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }

    host->winsize_changed = 0;
}

/* ======================= Input Reading ==================================== */

/* Read a key from the terminal in raw mode, handling escape sequences */
int terminal_read_key(int fd) {
    (void)fd;  /* fd is ignored on Windows */

    TerminalHost *host = g_terminal_host;
    if (!host) return -1;

    if (host->vt_mode) {
        /* VT mode: read characters directly, escape sequences come through */
        char c;
        char seq[6];
        DWORD read_count;
        DWORD wait_result;

        /* Wait for input with timeout (100ms like POSIX) */
        int retries = 0;
        while (1) {
            wait_result = WaitForSingleObject(host->input_handle, 100);
            if (wait_result == WAIT_OBJECT_0) {
                break;  /* Input available */
            }
            if (++retries > 1000) {
                /* After ~100 seconds of no input, assume console closed */
                fprintf(stderr, "\nNo input received, exiting.\n");
                exit(0);
            }
        }

        if (!ReadConsoleA(host->input_handle, &c, 1, &read_count, NULL) || read_count == 0) {
            return -1;
        }

        /* Handle escape sequences (same logic as POSIX) */
        if (c == ESC) {
            /* Try to read more of the escape sequence */
            wait_result = WaitForSingleObject(host->input_handle, 50);
            if (wait_result != WAIT_OBJECT_0) return ESC;
            if (!ReadConsoleA(host->input_handle, seq, 1, &read_count, NULL) || read_count == 0) return ESC;

            wait_result = WaitForSingleObject(host->input_handle, 50);
            if (wait_result != WAIT_OBJECT_0) return ESC;
            if (!ReadConsoleA(host->input_handle, seq+1, 1, &read_count, NULL) || read_count == 0) return ESC;

            /* ESC [ sequences */
            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    /* Extended escape, read additional byte */
                    wait_result = WaitForSingleObject(host->input_handle, 50);
                    if (wait_result != WAIT_OBJECT_0) return ESC;
                    if (!ReadConsoleA(host->input_handle, seq+2, 1, &read_count, NULL) || read_count == 0) return ESC;

                    if (seq[2] == '~') {
                        switch(seq[1]) {
                        case '3': return DEL_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        }
                    } else if (seq[2] == ';') {
                        /* ESC[1;2X for Shift+Arrow */
                        wait_result = WaitForSingleObject(host->input_handle, 50);
                        if (wait_result != WAIT_OBJECT_0) return ESC;
                        if (!ReadConsoleA(host->input_handle, seq+3, 1, &read_count, NULL) || read_count == 0) return ESC;

                        wait_result = WaitForSingleObject(host->input_handle, 50);
                        if (wait_result != WAIT_OBJECT_0) return ESC;
                        if (!ReadConsoleA(host->input_handle, seq+4, 1, &read_count, NULL) || read_count == 0) return ESC;

                        if (seq[1] == '1' && seq[3] == '2') {
                            switch(seq[4]) {
                            case 'A': return SHIFT_ARROW_UP;
                            case 'B': return SHIFT_ARROW_DOWN;
                            case 'C': return SHIFT_ARROW_RIGHT;
                            case 'D': return SHIFT_ARROW_LEFT;
                            }
                        }
                    }
                } else {
                    switch(seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                    }
                }
            }
            /* ESC O sequences */
            else if (seq[0] == 'O') {
                switch(seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
                }
            }
            return ESC;
        }
        return (unsigned char)c;

    } else {
        /* Legacy mode: use ReadConsoleInput to handle key events */
        INPUT_RECORD ir;
        DWORD events_read;
        DWORD wait_result;
        int retries = 0;

        while (1) {
            wait_result = WaitForSingleObject(host->input_handle, 100);
            if (wait_result == WAIT_TIMEOUT) {
                if (++retries > 1000) {
                    fprintf(stderr, "\nNo input received, exiting.\n");
                    exit(0);
                }
                continue;
            }
            if (wait_result != WAIT_OBJECT_0) {
                return -1;
            }

            if (!ReadConsoleInputA(host->input_handle, &ir, 1, &events_read)) {
                return -1;
            }

            if (events_read == 0) {
                continue;
            }

            /* Handle window resize events */
            if (ir.EventType == WINDOW_BUFFER_SIZE_EVENT) {
                if (g_terminal_host) {
                    g_terminal_host->winsize_changed = 1;
                }
                continue;
            }

            if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) {
                continue;  /* Ignore non-key events and key-up events */
            }

            WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
            char ascii = ir.Event.KeyEvent.uChar.AsciiChar;

            /* If we have an ASCII character, return it */
            if (ascii != 0) {
                return (unsigned char)ascii;
            }

            /* Map virtual keys to our key constants */
            switch (vk) {
            case VK_UP:     return ARROW_UP;
            case VK_DOWN:   return ARROW_DOWN;
            case VK_LEFT:   return ARROW_LEFT;
            case VK_RIGHT:  return ARROW_RIGHT;
            case VK_HOME:   return HOME_KEY;
            case VK_END:    return END_KEY;
            case VK_DELETE: return DEL_KEY;
            case VK_PRIOR:  return PAGE_UP;   /* Page Up */
            case VK_NEXT:   return PAGE_DOWN; /* Page Down */
            case VK_ESCAPE: return ESC;
            default:
                continue;  /* Ignore unknown keys */
            }
        }
    }
}

/* ======================= Window Size Detection ============================ */

int terminal_get_cursor_position(int ifd, int ofd, int *rows, int *cols) {
    (void)ifd;
    (void)ofd;

    TerminalHost *host = g_terminal_host;
    if (!host) {
        if (rows) *rows = 1;
        if (cols) *cols = 1;
        return -1;
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(host->output_handle, &csbi)) {
        if (rows) *rows = csbi.dwCursorPosition.Y + 1;  /* 1-based */
        if (cols) *cols = csbi.dwCursorPosition.X + 1;
        return 0;
    }

    if (rows) *rows = 1;
    if (cols) *cols = 1;
    return -1;
}

int terminal_get_window_size(int ifd, int ofd, int *rows, int *cols) {
    (void)ifd;
    (void)ofd;

    TerminalHost *host = g_terminal_host;
    if (!host) {
        if (cols) *cols = 80;
        if (rows) *rows = 24;
        return -1;
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(host->output_handle, &csbi)) {
        if (cols) *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        if (rows) *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return 0;
    }

    if (cols) *cols = 80;
    if (rows) *rows = 24;
    return -1;
}

/* Update window size and adjust screen layout */
void terminal_update_window_size(editor_ctx_t *ctx) {
    int rows, cols;
    if (terminal_get_window_size(0, 0, &rows, &cols) == -1) {
        rows = 24;
        cols = 80;
    }
    ctx->view.screencols = cols;
    rows -= STATUS_ROWS;
    if (rows < 1) rows = 1;
    ctx->view.screenrows_total = rows;
    ctx->view.screenrows = ctx->view.screenrows_total;
}

/* ======================= Resize Handling ================================== */

void terminal_handle_resize(editor_ctx_t *ctx) {
    if (!ctx) return;

    if (terminal_host_resize_pending(g_terminal_host)) {
        terminal_host_clear_resize(g_terminal_host);
        terminal_update_window_size(ctx);
        /* cy/cx are 0-indexed, so valid range is [0, screenrows-1] */
        if (ctx->view.cy >= ctx->view.screenrows) ctx->view.cy = ctx->view.screenrows - 1;
        if (ctx->view.cx >= ctx->view.screencols) ctx->view.cx = ctx->view.screencols - 1;
    }
}

/* ======================= Screen Buffer ==================================== */

void terminal_buffer_append(struct abuf *ab, const char *s, int len) {
    char *new_buf = realloc(ab->b, ab->len + len);

    if (new_buf == NULL) {
        /* Out of memory - attempt to restore terminal and exit cleanly */
        TerminalHost *host = g_terminal_host;
        if (host && host->vt_mode) {
            DWORD written;
            WriteConsoleA(host->output_handle, "\x1b[2J", 4, &written, NULL);
            WriteConsoleA(host->output_handle, "\x1b[H", 3, &written, NULL);
        }
        perror("Out of memory during screen refresh");
        exit(1);
    }
    memcpy(new_buf + ab->len, s, len);
    ab->b = new_buf;
    ab->len += len;
}

void terminal_buffer_free(struct abuf *ab) {
    free(ab->b);
}

#else /* !_WIN32 */

/* Provide an empty declaration to avoid "empty translation unit" warning */
typedef int terminal_win_not_available_placeholder;

#endif /* _WIN32 */
