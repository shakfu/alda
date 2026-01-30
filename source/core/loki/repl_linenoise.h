/**
 * @file repl_linenoise.h
 * @brief Linenoise integration layer for REPL with tree-sitter syntax highlighting.
 *
 * This module provides a wrapper around linenoise that integrates:
 * - Tree-sitter based syntax highlighting for Lua
 * - Completion callback adaptation
 * - History management
 */

#ifndef LOKI_REPL_LINENOISE_H
#define LOKI_REPL_LINENOISE_H

#ifdef LOKI_USE_LINENOISE

#include <linenoise.h>

/**
 * Supported languages for REPL syntax highlighting.
 */
typedef enum {
    REPL_LANG_NONE = 0,
    REPL_LANG_LUA,
    REPL_LANG_ALDA,
    REPL_LANG_CSOUND,
    REPL_LANG_JOY,
    REPL_LANG_HASKELL,
    REPL_LANG_SCHEME
} ReplLanguage;

/**
 * Initialize the linenoise REPL context for a specific language.
 *
 * @param lang The language to use for syntax highlighting
 * @return 0 on success, -1 on failure
 */
int repl_linenoise_init(ReplLanguage lang);

/**
 * Cleanup linenoise resources.
 */
void repl_linenoise_cleanup(void);

/**
 * Get the linenoise context for direct API access.
 *
 * @return The linenoise context, or NULL if not initialized
 */
linenoise_context_t *repl_linenoise_get_context(void);

/**
 * Set the language for syntax highlighting.
 *
 * @param lang The language to use
 */
void repl_linenoise_set_language(ReplLanguage lang);

/**
 * Get the current language.
 *
 * @return The current language
 */
ReplLanguage repl_linenoise_get_language(void);

#endif /* LOKI_USE_LINENOISE */

#endif /* LOKI_REPL_LINENOISE_H */
