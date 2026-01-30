/**
 * @file treesitter.c
 * @brief Tree-sitter syntax highlighting for editor buffers.
 *
 * This module provides tree-sitter based syntax highlighting for the editor.
 * Only Lua is supported in this build to minimize binary size.
 */

#include "treesitter.h"

#ifdef LOKI_USE_LINENOISE

#include "internal.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* External tree-sitter language function */
extern const TSLanguage *tree_sitter_lua(void);

/* Highlight query for Lua */
static const char *LUA_HIGHLIGHT_QUERY =
    "\"return\" @keyword.return\n"
    "[\"goto\" \"in\" \"local\"] @keyword\n"
    "(break_statement) @keyword\n"
    "(do_statement [\"do\" \"end\"] @keyword)\n"
    "(while_statement [\"while\" \"do\" \"end\"] @keyword.repeat)\n"
    "(repeat_statement [\"repeat\" \"until\"] @keyword.repeat)\n"
    "(if_statement [\"if\" \"elseif\" \"else\" \"then\" \"end\"] @keyword.conditional)\n"
    "(for_statement [\"for\" \"do\" \"end\"] @keyword.repeat)\n"
    "(function_declaration [\"function\" \"end\"] @keyword.function)\n"
    "(function_definition [\"function\" \"end\"] @keyword.function)\n"
    "[\"and\" \"not\" \"or\"] @keyword.operator\n"
    "(identifier) @variable\n"
    "(nil) @constant.builtin\n"
    "[(false) (true)] @boolean\n"
    "(comment) @comment\n"
    "(number) @number\n"
    "(string) @string\n"
;

/**
 * Map capture name to HL_* constant.
 */
static int capture_to_hl(const char *capture_name, uint32_t len) {
    /* Keywords */
    if (len >= 7 && strncmp(capture_name, "keyword", 7) == 0) {
        return HL_KEYWORD1;
    }

    /* Comments */
    if (len >= 7 && strncmp(capture_name, "comment", 7) == 0) {
        return HL_COMMENT;
    }

    /* Strings */
    if (len >= 6 && strncmp(capture_name, "string", 6) == 0) {
        return HL_STRING;
    }

    /* Numbers */
    if (len >= 6 && strncmp(capture_name, "number", 6) == 0) {
        return HL_NUMBER;
    }

    /* Types and type-like keywords */
    if (len >= 4 && strncmp(capture_name, "type", 4) == 0) {
        return HL_KEYWORD2;
    }

    /* Booleans and constants */
    if ((len >= 7 && strncmp(capture_name, "boolean", 7) == 0) ||
        (len >= 8 && strncmp(capture_name, "constant", 8) == 0)) {
        return HL_KEYWORD2;
    }

    /* Functions */
    if (len >= 8 && strncmp(capture_name, "function", 8) == 0) {
        return HL_KEYWORD1;
    }

    /* Variables - use normal highlighting */
    if (len >= 8 && strncmp(capture_name, "variable", 8) == 0) {
        return HL_NORMAL;
    }

    return HL_NORMAL;
}

const TSLanguage *treesitter_get_language(const char *lang_name) {
    if (!lang_name) return NULL;

    if (strcmp(lang_name, "lua") == 0) {
        return tree_sitter_lua();
    }

    return NULL;
}

static const char *get_highlight_query(const char *lang_name) {
    if (!lang_name) return NULL;

    if (strcmp(lang_name, "lua") == 0) {
        return LUA_HIGHLIGHT_QUERY;
    }

    return NULL;
}

const char *treesitter_lang_from_filename(const char *filename) {
    if (!filename) return NULL;

    const char *ext = strrchr(filename, '.');
    if (!ext) return NULL;
    ext++; /* Skip the dot */

    if (strcmp(ext, "lua") == 0) return "lua";

    return NULL;
}

TreeSitterState *treesitter_init(const char *lang_name) {
    uint32_t error_offset;
    TSQueryError error_type;

    const TSLanguage *language = treesitter_get_language(lang_name);
    if (!language) {
        return NULL;
    }

    const char *query_str = get_highlight_query(lang_name);
    if (!query_str) {
        return NULL;
    }

    TreeSitterState *ts = calloc(1, sizeof(TreeSitterState));
    if (!ts) {
        return NULL;
    }

    ts->language = language;

    /* Create parser */
    ts->parser = ts_parser_new();
    if (!ts->parser) {
        free(ts);
        return NULL;
    }

    if (!ts_parser_set_language(ts->parser, language)) {
        ts_parser_delete(ts->parser);
        free(ts);
        return NULL;
    }

    /* Create query */
    ts->query = ts_query_new(
        language,
        query_str,
        (uint32_t)strlen(query_str),
        &error_offset,
        &error_type
    );
    if (!ts->query) {
        ts_parser_delete(ts->parser);
        free(ts);
        return NULL;
    }

    /* Create cursor */
    ts->cursor = ts_query_cursor_new();
    if (!ts->cursor) {
        ts_query_delete(ts->query);
        ts_parser_delete(ts->parser);
        free(ts);
        return NULL;
    }

    return ts;
}

void treesitter_free(TreeSitterState *ts) {
    if (!ts) return;

    if (ts->cursor) {
        ts_query_cursor_delete(ts->cursor);
    }
    if (ts->query) {
        ts_query_delete(ts->query);
    }
    if (ts->tree) {
        ts_tree_delete(ts->tree);
    }
    if (ts->parser) {
        ts_parser_delete(ts->parser);
    }
    if (ts->source) {
        free(ts->source);
    }

    free(ts);
}

void treesitter_reparse(TreeSitterState *ts, const char *source, size_t len) {
    if (!ts || !ts->parser) return;

    /* Store source for later use */
    if (ts->source_cap < len + 1) {
        char *new_source = realloc(ts->source, len + 1);
        if (!new_source) return;
        ts->source = new_source;
        ts->source_cap = len + 1;
    }
    memcpy(ts->source, source, len);
    ts->source[len] = '\0';
    ts->source_len = len;

    /* Delete old tree if exists */
    if (ts->tree) {
        ts_tree_delete(ts->tree);
    }

    /* Parse the source */
    ts->tree = ts_parser_parse_string(ts->parser, NULL, source, (uint32_t)len);
}

void treesitter_edit(TreeSitterState *ts, TSInputEdit *edit) {
    if (!ts || !ts->tree || !edit) return;

    ts_tree_edit(ts->tree, edit);
}

void treesitter_update_row(editor_ctx_t *ctx, t_erow *row, TreeSitterState *ts) {
    if (!ctx || !row || !ts || !ts->parser || !ts->query || !ts->cursor) {
        return;
    }

    /* Parse just this row for simple highlighting */
    TSTree *tree = ts_parser_parse_string(ts->parser, NULL, row->render, (uint32_t)row->rsize);
    if (!tree) {
        return;
    }

    TSNode root = ts_tree_root_node(tree);

    /* Execute query */
    ts_query_cursor_exec(ts->cursor, ts->query, root);

    /* Apply captures to highlight array */
    TSQueryMatch match;
    uint32_t capture_index;

    while (ts_query_cursor_next_capture(ts->cursor, &match, &capture_index)) {
        TSQueryCapture capture = match.captures[capture_index];
        uint32_t start = ts_node_start_byte(capture.node);
        uint32_t end = ts_node_end_byte(capture.node);
        uint32_t name_len;
        const char *capture_name;
        int hl_type;

        capture_name = ts_query_capture_name_for_id(ts->query, capture.index, &name_len);
        hl_type = capture_to_hl(capture_name, name_len);

        if (start < (uint32_t)row->rsize && hl_type != HL_NORMAL) {
            if (end > (uint32_t)row->rsize) {
                end = (uint32_t)row->rsize;
            }
            for (uint32_t i = start; i < end; i++) {
                /* Only set if not already set (first match wins) */
                if (row->hl[i] == HL_NORMAL) {
                    row->hl[i] = (unsigned char)hl_type;
                }
            }
        }
    }

    ts_tree_delete(tree);
}

#endif /* LOKI_USE_LINENOISE */
