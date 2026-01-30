/* csound.h -- Tree-sitter based Csound syntax highlighting for linenoise
 *
 * This module provides a syntax highlighting callback that uses tree-sitter
 * to parse Csound CSD files and colorize them.
 */

#ifndef HIGHLIGHT_CSOUND_H
#define HIGHLIGHT_CSOUND_H

#include <stddef.h>

/* Initialize the Csound highlighter.
 * Must be called before using csound_highlight_callback.
 * Returns 0 on success, -1 on failure. */
int csound_highlight_init(void);

/* Free resources used by the Csound highlighter.
 * Should be called when highlighting is no longer needed. */
void csound_highlight_free(void);

/* Syntax highlighting callback for linenoise.
 * Can be passed to linenoise_set_highlight_callback(). */
void csound_highlight_callback(const char *buf, char *colors, size_t len);

#endif /* HIGHLIGHT_CSOUND_H */
