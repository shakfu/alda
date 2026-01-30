/* alda.h -- Tree-sitter based Alda syntax highlighting for linenoise
 *
 * This module provides a syntax highlighting callback that uses tree-sitter
 * to parse Alda music notation code and colorize it.
 */

#ifndef HIGHLIGHT_ALDA_H
#define HIGHLIGHT_ALDA_H

#include <stddef.h>

/* Initialize the Alda highlighter.
 * Must be called before using alda_highlight_callback.
 * Returns 0 on success, -1 on failure. */
int alda_highlight_init(void);

/* Free resources used by the Alda highlighter.
 * Should be called when highlighting is no longer needed. */
void alda_highlight_free(void);

/* Syntax highlighting callback for linenoise.
 * Can be passed to linenoise_set_highlight_callback(). */
void alda_highlight_callback(const char *buf, char *colors, size_t len);

#endif /* HIGHLIGHT_ALDA_H */
