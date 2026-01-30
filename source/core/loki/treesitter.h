/**
 * @file treesitter.h
 * @brief Tree-sitter syntax highlighting for editor buffers.
 *
 * This module provides tree-sitter based syntax highlighting that replaces
 * the old lexical analysis code with proper AST-based highlighting.
 * Only Lua is supported in this build to minimize binary size.
 */

#ifndef LOKI_TREESITTER_H
#define LOKI_TREESITTER_H

#ifdef LOKI_USE_LINENOISE

#include <tree_sitter/api.h>
#include <stddef.h>

/* Forward declarations - types are defined in internal.h */
struct editor_ctx;
struct t_erow;

/**
 * Tree-sitter state for a buffer.
 */
typedef struct TreeSitterState {
    TSParser *parser;
    TSTree *tree;
    TSQuery *query;
    TSQueryCursor *cursor;
    const TSLanguage *language;
    char *source;           /* Copy of source for reparsing */
    size_t source_len;
    size_t source_cap;
} TreeSitterState;

/**
 * Initialize tree-sitter for a language.
 *
 * @param lang_name Language name ("lua" only in this build)
 * @return Tree-sitter state, or NULL if language unavailable
 */
TreeSitterState *treesitter_init(const char *lang_name);

/**
 * Free tree-sitter state.
 *
 * @param ts Tree-sitter state to free
 */
void treesitter_free(TreeSitterState *ts);

/**
 * Update highlighting for a row using tree-sitter.
 *
 * @param ctx Editor context (for HL_* mapping)
 * @param row Row to highlight
 * @param ts Tree-sitter state
 */
void treesitter_update_row(struct editor_ctx *ctx, struct t_erow *row, TreeSitterState *ts);

/**
 * Notify tree-sitter of an edit for incremental parsing.
 *
 * @param ts Tree-sitter state
 * @param edit Edit information
 */
void treesitter_edit(TreeSitterState *ts, TSInputEdit *edit);

/**
 * Reparse the buffer after edits.
 *
 * @param ts Tree-sitter state
 * @param source Source text
 * @param len Length of source
 */
void treesitter_reparse(TreeSitterState *ts, const char *source, size_t len);

/**
 * Get tree-sitter language from language name.
 *
 * @param lang_name Language name
 * @return Language function, or NULL if not available
 */
const TSLanguage *treesitter_get_language(const char *lang_name);

/**
 * Get language name from file extension.
 *
 * @param filename Filename with extension
 * @return Language name, or NULL if not recognized
 */
const char *treesitter_lang_from_filename(const char *filename);

#endif /* LOKI_USE_LINENOISE */

#endif /* LOKI_TREESITTER_H */
