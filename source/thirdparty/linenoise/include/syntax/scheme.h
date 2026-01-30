/* scheme.h -- Tree-sitter based Scheme syntax highlighting for linenoise
 *
 * This module provides a syntax highlighting callback that uses tree-sitter
 * to parse Scheme code and colorize it.
 */

#ifndef HIGHLIGHT_SCHEME_H
#define HIGHLIGHT_SCHEME_H

#include <stddef.h>

/* Initialize the Scheme highlighter.
 * Must be called before using scheme_highlight_callback.
 * Returns 0 on success, -1 on failure. */
int scheme_highlight_init(void);

/* Free resources used by the Scheme highlighter.
 * Should be called when highlighting is no longer needed. */
void scheme_highlight_free(void);

/* Syntax highlighting callback for linenoise.
 * Can be passed to linenoise_set_highlight_callback(). */
void scheme_highlight_callback(const char *buf, char *colors, size_t len);

#endif /* HIGHLIGHT_SCHEME_H */
