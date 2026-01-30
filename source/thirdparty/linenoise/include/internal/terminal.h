/* terminal.h -- Platform-independent terminal abstraction.
 *
 * This module provides a terminal abstraction layer that allows linenoise
 * to work on different platforms (POSIX, Windows) without changing the
 * core line-editing logic.
 */

#ifndef LINENOISE_TERMINAL_H
#define LINENOISE_TERMINAL_H

#include <stddef.h>

/* Opaque terminal context - platform-specific implementation. */
typedef struct linenoise_terminal linenoise_terminal_t;

/* Create a terminal instance. Returns NULL on failure.
 * Uses stdin/stdout by default. */
linenoise_terminal_t *terminal_create(void);

/* Create a terminal instance with specific file descriptors.
 * Pass -1 to use defaults (stdin/stdout). */
linenoise_terminal_t *terminal_create_with_fds(int input_fd, int output_fd);

/* Destroy a terminal instance and free resources. */
void terminal_destroy(linenoise_terminal_t *term);

/* Check if the terminal is a TTY (interactive terminal).
 * Returns 1 if TTY, 0 if not (pipe, file, etc.). */
int terminal_is_tty(linenoise_terminal_t *term);

/* Enable raw mode for character-by-character input.
 * Returns 0 on success, -1 on failure. */
int terminal_enable_raw(linenoise_terminal_t *term);

/* Disable raw mode and restore original terminal settings.
 * Returns 0 on success, -1 on failure. */
int terminal_disable_raw(linenoise_terminal_t *term);

/* Check if terminal is currently in raw mode. */
int terminal_is_raw(linenoise_terminal_t *term);

/* Get terminal size in columns and rows.
 * Returns 0 on success, -1 on failure.
 * On failure, cols and rows are set to defaults (80, 24). */
int terminal_get_size(linenoise_terminal_t *term, int *cols, int *rows);

/* Read a single byte from the terminal.
 * timeout_ms: timeout in milliseconds, 0 for non-blocking, -1 for blocking.
 * Returns 1 on success, 0 on timeout, -1 on error/EOF. */
int terminal_read_byte(linenoise_terminal_t *term, char *c, int timeout_ms);

/* Write data to the terminal.
 * Returns number of bytes written, or -1 on error. */
int terminal_write(linenoise_terminal_t *term, const char *buf, size_t len);

/* Clear the screen and move cursor to home position. */
void terminal_clear_screen(linenoise_terminal_t *term);

/* Sound the terminal bell. */
void terminal_beep(linenoise_terminal_t *term);

/* Get the input file descriptor (for use with select, etc.).
 * Returns -1 if not applicable (e.g., Windows handles). */
int terminal_get_input_fd(linenoise_terminal_t *term);

/* Get the output file descriptor.
 * Returns -1 if not applicable. */
int terminal_get_output_fd(linenoise_terminal_t *term);

#endif /* LINENOISE_TERMINAL_H */
