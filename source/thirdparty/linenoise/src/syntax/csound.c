/* csound.c -- Tree-sitter based Csound syntax highlighting
 *
 * Uses tree-sitter to parse Csound CSD files and highlight them.
 */

#include <stdlib.h>
#include <string.h>
#include "syntax/csound.h"
#include "syntax/theme.h"
#include <tree_sitter/api.h>

/* External function from tree-sitter-csound */
extern const TSLanguage *tree_sitter_csound(void);

/* Global state for the highlighter */
static TSParser *parser = NULL;
static TSQuery *query = NULL;
static TSQueryCursor *cursor = NULL;

/* Csound highlighting query */
static const char *CSOUND_HIGHLIGHT_QUERY =
    /* Comments */
    "(comment) @comment\n"
    "(block_comment) @comment\n"

    /* XML tags */
    "(xml_tag) @keyword.directive\n"

    /* Header variables */
    "(header_var) @variable.builtin\n"

    /* Keywords */
    "(instrument_keyword) @keyword.function\n"
    "(opcode_keyword) @keyword.function\n"
    "(block_keyword) @keyword\n"
    "(control_keyword) @keyword.control\n"
    "(type_keyword) @keyword\n"

    /* Variables */
    "(variable) @variable\n"
    "(pfield) @variable.parameter\n"

    /* Numbers */
    "(number) @number\n"

    /* Strings */
    "(string) @string\n"

    /* Operators */
    "(operator) @operator\n"

    /* Identifiers (opcodes, labels) */
    "(identifier) @function\n"
;

/* Map tree-sitter capture names to theme token types */
static unsigned char get_color_for_capture(const char *name, uint32_t len) {
    /* Comments */
    if (len >= 7 && strncmp(name, "comment", 7) == 0) {
        return theme_color(TOK_COMMENT);
    }

    /* Strings */
    if (len >= 6 && strncmp(name, "string", 6) == 0) {
        return theme_color(TOK_STRING);
    }

    /* Numbers */
    if (len >= 6 && strncmp(name, "number", 6) == 0) {
        return theme_color(TOK_NUMBER);
    }

    /* Keywords */
    if (len >= 7 && strncmp(name, "keyword", 7) == 0) {
        if (len > 8) {
            const char *suffix = name + 8;
            if (strncmp(suffix, "function", 8) == 0) {
                return theme_color(TOK_KEYWORD_FUNCTION);
            }
            if (strncmp(suffix, "control", 7) == 0) {
                return theme_color(TOK_KEYWORD_CONTROL);
            }
            if (strncmp(suffix, "directive", 9) == 0) {
                return theme_color(TOK_KEYWORD);
            }
        }
        return theme_color(TOK_KEYWORD);
    }

    /* Variables */
    if (len >= 8 && strncmp(name, "variable", 8) == 0) {
        if (len > 9 && strncmp(name + 9, "builtin", 7) == 0) {
            return theme_color(TOK_VARIABLE_BUILTIN);
        }
        if (len > 9 && strncmp(name + 9, "parameter", 9) == 0) {
            return theme_color(TOK_VARIABLE_PARAMETER);
        }
        return theme_color(TOK_VARIABLE);
    }

    /* Functions (opcodes) */
    if (len >= 8 && strncmp(name, "function", 8) == 0) {
        return theme_color(TOK_FUNCTION);
    }

    /* Operators */
    if (len >= 8 && strncmp(name, "operator", 8) == 0) {
        return theme_color(TOK_OPERATOR);
    }

    return theme_color(TOK_DEFAULT);
}

int csound_highlight_init(void) {
    uint32_t error_offset;
    TSQueryError error_type;

    if (parser != NULL) {
        return 0; /* Already initialized */
    }

    /* Create parser */
    parser = ts_parser_new();
    if (parser == NULL) {
        return -1;
    }

    /* Set the Csound language */
    if (!ts_parser_set_language(parser, tree_sitter_csound())) {
        ts_parser_delete(parser);
        parser = NULL;
        return -1;
    }

    /* Create the highlighting query */
    query = ts_query_new(
        tree_sitter_csound(),
        CSOUND_HIGHLIGHT_QUERY,
        (uint32_t)strlen(CSOUND_HIGHLIGHT_QUERY),
        &error_offset,
        &error_type
    );
    if (query == NULL) {
        ts_parser_delete(parser);
        parser = NULL;
        return -1;
    }

    /* Create a reusable query cursor */
    cursor = ts_query_cursor_new();
    if (cursor == NULL) {
        ts_query_delete(query);
        ts_parser_delete(parser);
        query = NULL;
        parser = NULL;
        return -1;
    }

    return 0;
}

void csound_highlight_free(void) {
    if (cursor != NULL) {
        ts_query_cursor_delete(cursor);
        cursor = NULL;
    }
    if (query != NULL) {
        ts_query_delete(query);
        query = NULL;
    }
    if (parser != NULL) {
        ts_parser_delete(parser);
        parser = NULL;
    }
}

void csound_highlight_callback(const char *buf, char *colors, size_t len) {
    TSTree *tree;
    TSNode root;
    TSQueryMatch match;
    uint32_t capture_index;

    if (parser == NULL || query == NULL || cursor == NULL) {
        return;
    }

    if (len == 0) {
        return;
    }

    /* Parse the input */
    tree = ts_parser_parse_string(parser, NULL, buf, (uint32_t)len);
    if (tree == NULL) {
        return;
    }

    root = ts_tree_root_node(tree);

    /* Execute the query */
    ts_query_cursor_exec(cursor, query, root);

    /* Iterate through captures and apply colors */
    while (ts_query_cursor_next_capture(cursor, &match, &capture_index)) {
        TSQueryCapture capture = match.captures[capture_index];
        uint32_t start = ts_node_start_byte(capture.node);
        uint32_t end = ts_node_end_byte(capture.node);
        uint32_t name_len;
        const char *capture_name;
        unsigned char color;
        uint32_t i;

        /* Get the capture name */
        capture_name = ts_query_capture_name_for_id(query, capture.index, &name_len);
        color = get_color_for_capture(capture_name, name_len);

        /* Apply color to the byte range */
        if (start < len && color != 0) {
            if (end > len) {
                end = (uint32_t)len;
            }
            for (i = start; i < end; i++) {
                if (colors[i] == 0) {
                    colors[i] = (char)color;
                }
            }
        }
    }

    ts_tree_delete(tree);
}
