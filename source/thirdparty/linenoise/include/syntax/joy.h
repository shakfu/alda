/* joy.h -- Tree-sitter based Joy syntax highlighting for linenoise
 *
 * This module provides a syntax highlighting callback that uses tree-sitter
 * to parse Joy code and colorize it.
 */

#ifndef HIGHLIGHT_JOY_H
#define HIGHLIGHT_JOY_H

#include <stddef.h>

/* Initialize the Joy highlighter.
 * Must be called before using joy_highlight_callback.
 * Returns 0 on success, -1 on failure. */
int joy_highlight_init(void);

/* Free resources used by the Joy highlighter.
 * Should be called when highlighting is no longer needed. */
void joy_highlight_free(void);

/* Syntax highlighting callback for linenoise.
 * Can be passed to linenoise_set_highlight_callback(). */
void joy_highlight_callback(const char *buf, char *colors, size_t len);

#endif /* HIGHLIGHT_JOY_H */
