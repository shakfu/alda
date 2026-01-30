/* lua.c -- Tree-sitter based Lua syntax highlighting
 *
 * Uses tree-sitter to parse Lua code and highlight it using query captures
 * from the Lua grammar's highlights.scm patterns.
 */

#include <stdlib.h>
#include <string.h>
#include "syntax/lua.h"
#include "syntax/theme.h"
#include <tree_sitter/api.h>

/* External function from tree-sitter-lua */
extern const TSLanguage *tree_sitter_lua(void);

/* Global state for the highlighter */
static TSParser *parser = NULL;
static TSQuery *query = NULL;
static TSQueryCursor *cursor = NULL;

/* Lua highlighting query.
 * Based on tree-sitter-lua's highlights.scm but simplified for linenoise use. */
static const char *LUA_HIGHLIGHT_QUERY =
    /* Keywords */
    "\"return\" @keyword.return\n"
    "[\"goto\" \"in\" \"local\"] @keyword\n"
    "(break_statement) @keyword\n"
    "(do_statement [\"do\" \"end\"] @keyword)\n"
    "(while_statement [\"while\" \"do\" \"end\"] @keyword.repeat)\n"
    "(repeat_statement [\"repeat\" \"until\"] @keyword.repeat)\n"
    "(if_statement [\"if\" \"elseif\" \"else\" \"then\" \"end\"] @keyword.conditional)\n"
    "(elseif_statement [\"elseif\" \"then\"] @keyword.conditional)\n"
    "(else_statement [\"else\"] @keyword.conditional)\n"
    "(for_statement [\"for\" \"do\" \"end\"] @keyword.repeat)\n"
    "(function_declaration [\"function\" \"end\"] @keyword.function)\n"
    "(function_definition [\"function\" \"end\"] @keyword.function)\n"

    /* Operators */
    "[\"and\" \"not\" \"or\"] @keyword.operator\n"
    "\"=\" @operator\n"
    "(binary_expression operator: _ @operator)\n"
    "(unary_expression operator: _ @operator)\n"

    /* Punctuation */
    "[\";\" \":\" \",\" \".\"] @punctuation.delimiter\n"
    "[\"(\" \")\" \"[\" \"]\" \"{\" \"}\"] @punctuation.bracket\n"

    /* Variables and identifiers */
    "(identifier) @variable\n"
    "((identifier) @variable.builtin (#eq? @variable.builtin \"self\"))\n"

    /* Constants */
    "(nil) @constant.builtin\n"
    "[(false) (true)] @boolean\n"
    "(vararg_expression) @constant\n"

    /* Tables */
    "(field name: (identifier) @field)\n"
    "(dot_index_expression field: (identifier) @field)\n"
    "(table_constructor [\"{\"\"}\" ] @constructor)\n"

    /* Functions */
    "(parameters (identifier) @parameter)\n"
    "(function_declaration name: (identifier) @function)\n"
    "(function_declaration name: (dot_index_expression field: (identifier) @function))\n"
    "(function_declaration name: (method_index_expression method: (identifier) @method))\n"
    "(function_call name: (identifier) @function.call)\n"
    "(function_call name: (dot_index_expression field: (identifier) @function.call))\n"
    "(function_call name: (method_index_expression method: (identifier) @method.call))\n"

    /* Built-in functions */
    "(function_call (identifier) @function.builtin\n"
    "  (#any-of? @function.builtin\n"
    "    \"assert\" \"collectgarbage\" \"dofile\" \"error\" \"getfenv\" \"getmetatable\"\n"
    "    \"ipairs\" \"load\" \"loadfile\" \"loadstring\" \"module\" \"next\" \"pairs\"\n"
    "    \"pcall\" \"print\" \"rawequal\" \"rawget\" \"rawset\" \"require\" \"select\"\n"
    "    \"setfenv\" \"setmetatable\" \"tonumber\" \"tostring\" \"type\" \"unpack\" \"xpcall\"))\n"

    /* Literals */
    "(comment) @comment\n"
    "(hash_bang_line) @comment\n"
    "(number) @number\n"
    "(string) @string\n"
    "(escape_sequence) @string.escape\n"
;

/* Map tree-sitter capture names to theme token types.
 * Returns the appropriate color from the current theme. */
static unsigned char get_color_for_capture(const char *name, uint32_t len) {
    /* Keywords */
    if (len >= 7 && strncmp(name, "keyword", 7) == 0) {
        if (len > 8) {
            /* Check for specific keyword types */
            const char *suffix = name + 8;
            if (strncmp(suffix, "return", 6) == 0) {
                return theme_color(TOK_KEYWORD_RETURN);
            }
            if (strncmp(suffix, "repeat", 6) == 0 ||
                strncmp(suffix, "conditional", 11) == 0) {
                return theme_color(TOK_KEYWORD_CONTROL);
            }
            if (strncmp(suffix, "function", 8) == 0) {
                return theme_color(TOK_KEYWORD_FUNCTION);
            }
            if (strncmp(suffix, "operator", 8) == 0) {
                return theme_color(TOK_KEYWORD_OPERATOR);
            }
        }
        return theme_color(TOK_KEYWORD);
    }

    /* Comments */
    if (len >= 7 && strncmp(name, "comment", 7) == 0) {
        return theme_color(TOK_COMMENT);
    }

    /* Strings */
    if (len >= 6 && strncmp(name, "string", 6) == 0) {
        if (len > 7 && strncmp(name + 7, "escape", 6) == 0) {
            return theme_color(TOK_STRING_ESCAPE);
        }
        return theme_color(TOK_STRING);
    }

    /* Numbers */
    if (len >= 6 && strncmp(name, "number", 6) == 0) {
        return theme_color(TOK_NUMBER);
    }

    /* Functions */
    if (len >= 8 && strncmp(name, "function", 8) == 0) {
        if (len > 9 && strncmp(name + 9, "builtin", 7) == 0) {
            return theme_color(TOK_FUNCTION_BUILTIN);
        }
        if (len > 9 && strncmp(name + 9, "call", 4) == 0) {
            return theme_color(TOK_FUNCTION_CALL);
        }
        return theme_color(TOK_FUNCTION);
    }

    /* Methods */
    if (len >= 6 && strncmp(name, "method", 6) == 0) {
        return theme_color(TOK_FUNCTION_METHOD);
    }

    /* Booleans */
    if (len >= 7 && strncmp(name, "boolean", 7) == 0) {
        return theme_color(TOK_BOOLEAN);
    }

    /* Constants */
    if (len >= 8 && strncmp(name, "constant", 8) == 0) {
        if (len > 9 && strncmp(name + 9, "builtin", 7) == 0) {
            return theme_color(TOK_CONSTANT_BUILTIN);
        }
        return theme_color(TOK_CONSTANT);
    }

    /* Operators */
    if (len >= 8 && strncmp(name, "operator", 8) == 0) {
        return theme_color(TOK_OPERATOR);
    }

    /* Parameters */
    if (len >= 9 && strncmp(name, "parameter", 9) == 0) {
        return theme_color(TOK_VARIABLE_PARAMETER);
    }

    /* Fields */
    if (len >= 5 && strncmp(name, "field", 5) == 0) {
        return theme_color(TOK_VARIABLE_FIELD);
    }

    /* Constructors */
    if (len >= 11 && strncmp(name, "constructor", 11) == 0) {
        return theme_color(TOK_CONSTRUCTOR);
    }

    /* Punctuation */
    if (len >= 11 && strncmp(name, "punctuation", 11) == 0) {
        if (len > 12 && strncmp(name + 12, "bracket", 7) == 0) {
            return theme_color(TOK_PUNCTUATION_BRACKET);
        }
        if (len > 12 && strncmp(name + 12, "delimiter", 9) == 0) {
            return theme_color(TOK_PUNCTUATION_DELIMITER);
        }
        return theme_color(TOK_PUNCTUATION);
    }

    /* Variables */
    if (len >= 8 && strncmp(name, "variable", 8) == 0) {
        if (len > 9 && strncmp(name + 9, "builtin", 7) == 0) {
            return theme_color(TOK_VARIABLE_BUILTIN);
        }
        return theme_color(TOK_VARIABLE);
    }

    return theme_color(TOK_DEFAULT);
}

int lua_highlight_init(void) {
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

    /* Set the Lua language */
    if (!ts_parser_set_language(parser, tree_sitter_lua())) {
        ts_parser_delete(parser);
        parser = NULL;
        return -1;
    }

    /* Create the highlighting query */
    query = ts_query_new(
        tree_sitter_lua(),
        LUA_HIGHLIGHT_QUERY,
        (uint32_t)strlen(LUA_HIGHLIGHT_QUERY),
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

void lua_highlight_free(void) {
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

void lua_highlight_callback(const char *buf, char *colors, size_t len) {
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
                /* Only set color if not already set (first match wins for overlapping) */
                if (colors[i] == 0) {
                    colors[i] = (char)color;
                }
            }
        }
    }

    ts_tree_delete(tree);
}
