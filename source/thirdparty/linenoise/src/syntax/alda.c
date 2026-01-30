/* alda.c -- Tree-sitter based Alda syntax highlighting
 *
 * Uses tree-sitter to parse Alda music notation and highlight it.
 */

#include <stdlib.h>
#include <string.h>
#include "syntax/alda.h"
#include "syntax/theme.h"
#include <tree_sitter/api.h>

/* External function from tree-sitter-alda */
extern const TSLanguage *tree_sitter_alda(void);

/* Global state for the highlighter */
static TSParser *parser = NULL;
static TSQuery *query = NULL;
static TSQueryCursor *cursor = NULL;

/* Alda highlighting query */
static const char *ALDA_HIGHLIGHT_QUERY =
    /* Comments */
    "(comment) @comment\n"

    /* Strings */
    "(string) @string\n"

    /* Numbers and durations */
    "(note_length) @number\n"
    "(duration_ms) @number\n"
    "(duration_s) @number\n"
    "(sexp_number) @number\n"

    /* Notes and pitches */
    "(note_letter) @constant\n"
    "(pitch) @constant\n"

    /* Accidentals */
    "(accidental) @operator\n"

    /* Rests */
    "(rest) @constant.builtin\n"

    /* Octave control */
    "(octave_set) @keyword\n"
    "(octave_up) @operator\n"
    "(octave_down) @operator\n"

    /* Chords */
    "(chord) @constant\n"

    /* Instruments and parts */
    "(instrument_call) @function\n"
    "(identifier) @variable\n"

    /* Markers */
    "(marker) @label\n"
    "(at_marker) @label\n"
    "(voice_marker) @keyword\n"

    /* Grouping */
    "(cram [\"{\" \"}\"] @punctuation.bracket)\n"
    "(bracket_seq [\"[\" \"]\"] @punctuation.bracket)\n"

    /* Repetition */
    "(repeat_count) @number\n"
    "(on_repetitions) @number\n"

    /* S-expressions (Lisp-like) */
    "(sexp [\"(\" \")\"] @punctuation.bracket)\n"
    "(sexp_symbol) @function.builtin\n"
    "(quoted_list) @constant\n"

    /* Operators and delimiters */
    "(barline) @punctuation.delimiter\n"
    "(dot) @operator\n"
    "(tie_duration) @operator\n"
    "\"=\" @operator\n"
    "\"/\" @operator\n"
    "\":\" @punctuation.delimiter\n"
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
        return theme_color(TOK_KEYWORD);
    }

    /* Constants (notes, pitches) */
    if (len >= 8 && strncmp(name, "constant", 8) == 0) {
        if (len > 9 && strncmp(name + 9, "builtin", 7) == 0) {
            return theme_color(TOK_CONSTANT_BUILTIN);
        }
        return theme_color(TOK_CONSTANT);
    }

    /* Functions */
    if (len >= 8 && strncmp(name, "function", 8) == 0) {
        if (len > 9 && strncmp(name + 9, "builtin", 7) == 0) {
            return theme_color(TOK_FUNCTION_BUILTIN);
        }
        return theme_color(TOK_FUNCTION);
    }

    /* Variables */
    if (len >= 8 && strncmp(name, "variable", 8) == 0) {
        return theme_color(TOK_VARIABLE);
    }

    /* Labels (markers) */
    if (len >= 5 && strncmp(name, "label", 5) == 0) {
        return theme_color(TOK_KEYWORD_CONTROL);
    }

    /* Operators */
    if (len >= 8 && strncmp(name, "operator", 8) == 0) {
        return theme_color(TOK_OPERATOR);
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

    return theme_color(TOK_DEFAULT);
}

int alda_highlight_init(void) {
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

    /* Set the Alda language */
    if (!ts_parser_set_language(parser, tree_sitter_alda())) {
        ts_parser_delete(parser);
        parser = NULL;
        return -1;
    }

    /* Create the highlighting query */
    query = ts_query_new(
        tree_sitter_alda(),
        ALDA_HIGHLIGHT_QUERY,
        (uint32_t)strlen(ALDA_HIGHLIGHT_QUERY),
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

void alda_highlight_free(void) {
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

void alda_highlight_callback(const char *buf, char *colors, size_t len) {
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
