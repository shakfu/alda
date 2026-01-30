/* loki_syntax.c - Syntax highlighting implementation
 *
 * This module implements syntax highlighting for the loki editor.
 * It uses a simple token-based approach with support for:
 * - Keywords (primary and type keywords)
 * - String literals (with escape sequence handling)
 * - Single-line and multi-line comments
 * - Numeric literals
 * - Non-printable character visualization
 *
 * The highlighting is performed on the "rendered" version of each row
 * (after tab expansion) and stores highlight types in the row->hl array.
 *
 * When built with linenoise support (LOKI_USE_LINENOISE), colors are
 * pulled from the current tree-sitter theme for consistency.
 */

#include "syntax.h"
#include "languages.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#ifdef LOKI_USE_LINENOISE
#include <syntax/theme.h>
#include "treesitter.h"
#endif

/* ====================== Syntax highlight color scheme  ==================== */

int syntax_is_separator(int c, char *separators) {
    return c == '\0' || isspace(c) || strchr(separators, c) != NULL;
}

/* Return true if the specified row last char is part of a multi line comment
 * that starts at this row or at one before, and does not end at the end
 * of the row but spawns to the next row. */
int syntax_row_has_open_comment(t_erow *row) {
    if (row->hl && row->rsize && row->hl[row->rsize-1] == HL_MLCOMMENT &&
        (row->rsize < 2 || (row->render[row->rsize-2] != '*' ||
                            row->render[row->rsize-1] != '/'))) return 1;
    return 0;
}

/* Forward declaration for markdown highlighter */
void editor_update_syntax_markdown(editor_ctx_t *ctx, t_erow *row);

/* Map human-readable style names to HL_* constants */
int syntax_name_to_code(const char *name) {
    if (name == NULL) return -1;
    /* Base types (0-8) */
    if (strcasecmp(name, "normal") == 0) return HL_NORMAL;
    if (strcasecmp(name, "nonprint") == 0) return HL_NONPRINT;
    if (strcasecmp(name, "comment") == 0) return HL_COMMENT;
    if (strcasecmp(name, "mlcomment") == 0) return HL_MLCOMMENT;
    if (strcasecmp(name, "keyword1") == 0) return HL_KEYWORD1;
    if (strcasecmp(name, "keyword2") == 0) return HL_KEYWORD2;
    if (strcasecmp(name, "string") == 0) return HL_STRING;
    if (strcasecmp(name, "number") == 0) return HL_NUMBER;
    if (strcasecmp(name, "match") == 0) return HL_MATCH;
    /* Extended types (9-23) */
    if (strcasecmp(name, "function") == 0) return HL_FUNCTION;
    if (strcasecmp(name, "function_builtin") == 0) return HL_FUNCTION_BUILTIN;
    if (strcasecmp(name, "function_call") == 0) return HL_FUNCTION_CALL;
    if (strcasecmp(name, "variable_builtin") == 0) return HL_VARIABLE_BUILTIN;
    if (strcasecmp(name, "variable_parameter") == 0) return HL_VARIABLE_PARAMETER;
    if (strcasecmp(name, "operator") == 0) return HL_OPERATOR;
    if (strcasecmp(name, "punctuation") == 0) return HL_PUNCTUATION;
    if (strcasecmp(name, "constructor") == 0) return HL_CONSTRUCTOR;
    if (strcasecmp(name, "namespace") == 0) return HL_NAMESPACE;
    if (strcasecmp(name, "label") == 0) return HL_LABEL;
    if (strcasecmp(name, "tag") == 0) return HL_TAG;
    if (strcasecmp(name, "keyword_control") == 0) return HL_KEYWORD_CONTROL;
    if (strcasecmp(name, "keyword_function") == 0) return HL_KEYWORD_FUNCTION;
    if (strcasecmp(name, "keyword_return") == 0) return HL_KEYWORD_RETURN;
    if (strcasecmp(name, "constant_builtin") == 0) return HL_CONSTANT_BUILTIN;
    return -1;
}

/* Set every byte of row->hl (that corresponds to every character in the line)
 * to the right syntax highlight type (HL_* defines). */
void syntax_update_row(editor_ctx_t *ctx, t_erow *row) {
    if (row->rsize == 0) {
        free(row->hl);
        row->hl = NULL;
        return;
    }
    unsigned char *new_hl = realloc(row->hl,row->rsize);
    if (new_hl == NULL) return; /* Out of memory, keep old highlighting */
    row->hl = new_hl;
    memset(row->hl,HL_NORMAL,row->rsize);

#ifdef LOKI_USE_LINENOISE
    /* Use tree-sitter highlighting when available */
    if (ctx->model.ts_state != NULL) {
        treesitter_update_row(ctx, row, ctx->model.ts_state);
        return;
    }
#endif

    int default_ran = 0;

    if (ctx->view.syntax != NULL) {
        if (ctx->view.syntax->type == HL_TYPE_MARKDOWN) {
            editor_update_syntax_markdown(ctx, row);
            default_ran = 1;
        } else if (ctx->view.syntax->type == HL_TYPE_CSOUND) {
            editor_update_syntax_csound(ctx, row);
            default_ran = 1;
        } else {
            int i, prev_sep, in_string, in_comment;
            char *p;
            char **keywords = ctx->view.syntax->keywords;
            char *scs = ctx->view.syntax->singleline_comment_start;
            char *mcs = ctx->view.syntax->multiline_comment_start;
            char *mce = ctx->view.syntax->multiline_comment_end;
            char *separators = ctx->view.syntax->separators;

            /* Point to the first non-space char. */
            p = row->render;
            i = 0; /* Current char offset */
            while(*p && isspace(*p)) {
                p++;
                i++;
            }
            prev_sep = 1; /* Tell the parser if 'i' points to start of word. */
            in_string = 0; /* Are we inside "" or '' ? */
            in_comment = 0; /* Are we inside multi-line comment? */

            /* If the previous line has an open comment, this line starts
             * with an open comment state. */
            if (row->idx > 0 && syntax_row_has_open_comment(&ctx->model.row[row->idx-1]))
                in_comment = 1;

            while(*p) {
                /* Handle single-line comments (e.g., //, #, --) */
                if (prev_sep && scs[0] && *p == scs[0] &&
                    (scs[1] == '\0' || (i < row->rsize - 1 && *(p+1) == scs[1]))) {
                    /* From here to end is a comment */
                    memset(row->hl+i,HL_COMMENT,row->rsize-i);
                    break;
                }

                /* Handle multi line comments. */
                if (in_comment) {
                    row->hl[i] = HL_MLCOMMENT;
                    if (i < row->rsize - 1 && *p == mce[0] && *(p+1) == mce[1]) {
                        row->hl[i+1] = HL_MLCOMMENT;
                        p += 2; i += 2;
                        in_comment = 0;
                        prev_sep = 1;
                        continue;
                    } else {
                        prev_sep = 0;
                        p++; i++;
                        continue;
                    }
                } else if (i < row->rsize - 1 && *p == mcs[0] && *(p+1) == mcs[1]) {
                    row->hl[i] = HL_MLCOMMENT;
                    row->hl[i+1] = HL_MLCOMMENT;
                    p += 2; i += 2;
                    in_comment = 1;
                    prev_sep = 0;
                    continue;
                }

                /* Handle "" and '' */
                if (in_string) {
                    row->hl[i] = HL_STRING;
                    if (i < row->rsize - 1 && *p == '\\') {
                        row->hl[i+1] = HL_STRING;
                        p += 2; i += 2;
                        prev_sep = 0;
                        continue;
                    }
                    if (*p == in_string) in_string = 0;
                    p++; i++;
                    continue;
                } else {
                    if (*p == '"' || *p == '\'') {
                        in_string = *p;
                        row->hl[i] = HL_STRING;
                        p++; i++;
                        prev_sep = 0;
                        continue;
                    }
                }

                /* Handle non printable chars. */
                if (!isprint(*p)) {
                    row->hl[i] = HL_NONPRINT;
                    p++; i++;
                    prev_sep = 0;
                    continue;
                }

                /* Handle numbers */
                if ((isdigit(*p) && (prev_sep || row->hl[i-1] == HL_NUMBER)) ||
                    (*p == '.' && i > 0 && row->hl[i-1] == HL_NUMBER &&
                     i < row->rsize - 1 && isdigit(*(p+1)))) {
                    row->hl[i] = HL_NUMBER;
                    p++; i++;
                    prev_sep = 0;
                    continue;
                }

                /* Handle keywords and lib calls */
                if (prev_sep) {
                    int j;
                    for (j = 0; keywords[j]; j++) {
                        int klen = strlen(keywords[j]);
                        int kw2 = keywords[j][klen-1] == '|';
                        if (kw2) klen--;

                        if (i + klen <= row->rsize &&
                            !memcmp(p,keywords[j],klen) &&
                            (i + klen == row->rsize || syntax_is_separator(*(p+klen), separators)))
                        {
                            /* Keyword */
                            memset(row->hl+i,kw2 ? HL_KEYWORD2 : HL_KEYWORD1,klen);
                            p += klen;
                            i += klen;
                            break;
                        }
                    }
                    if (keywords[j] != NULL) {
                        prev_sep = 0;
                        continue; /* We had a keyword match */
                    }
                }

                /* Not special chars */
                prev_sep = syntax_is_separator(*p, separators);
                p++; i++;
            }

            default_ran = 1;
        }
    }

    /* Lua custom highlighting is in loki_editor.c */
    (void)default_ran; /* Suppress unused variable warning */

    /* Propagate syntax change to the next row if the open comment
     * state changed. This may recursively affect all the following rows
     * in the file. */
    int oc = syntax_row_has_open_comment(row);
    if (row->hl_oc != oc && row->idx+1 < ctx->model.numrows)
        syntax_update_row(ctx, &ctx->model.row[row->idx+1]);
    row->hl_oc = oc;
}

/* Format RGB color escape sequence for syntax highlighting.
 * Uses true color (24-bit) escape codes: ESC[38;2;R;G;Bm
 * Returns the length of the formatted string. */
int syntax_format_color(editor_ctx_t *ctx, int hl, char *buf, size_t bufsize) {
    if (hl < 0 || hl >= HL_TYPE_COUNT) hl = 0;  /* Default to HL_NORMAL */
    t_hlcolor *color = &ctx->view.colors[hl];
    return snprintf(buf, bufsize, "\x1b[38;2;%d;%d;%dm",
                    color->r, color->g, color->b);
}

/* Select the syntax highlight scheme depending on the filename. */
void syntax_select_for_filename(editor_ctx_t *ctx, char *filename) {
#ifdef LOKI_USE_LINENOISE
    /* Free any existing tree-sitter state */
    if (ctx->model.ts_state != NULL) {
        treesitter_free(ctx->model.ts_state);
        ctx->model.ts_state = NULL;
    }

    /* Try to initialize tree-sitter for this file */
    const char *ts_lang = treesitter_lang_from_filename(filename);
    if (ts_lang != NULL) {
        ctx->model.ts_state = treesitter_init(ts_lang);
        /* If tree-sitter succeeded, we still set view.syntax for fallback */
    }
#endif

    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        struct t_editor_syntax *s = HLDB+j;
        if (!s->filematch) continue;
        unsigned int i = 0;
        while(s->filematch[i]) {
            char *p;
            int patlen = strlen(s->filematch[i]);
            if ((p = strstr(filename,s->filematch[i])) != NULL) {
                if (s->filematch[i][0] != '.' || p[patlen] == '\0') {
                    ctx->view.syntax = s;
                    return;
                }
            }
            i++;
        }
    }

    /* Also check dynamic language registry */
    int dynamic_count = get_dynamic_language_count();
    for (int j = 0; j < dynamic_count; j++) {
        struct t_editor_syntax *s = get_dynamic_language(j);
        if (!s || !s->filematch) continue;
        unsigned int i = 0;
        while(s->filematch[i]) {
            char *p;
            int patlen = strlen(s->filematch[i]);
            if ((p = strstr(filename,s->filematch[i])) != NULL) {
                if (s->filematch[i][0] != '.' || p[patlen] == '\0') {
                    ctx->view.syntax = s;
                    return;
                }
            }
            i++;
        }
    }
}

#ifdef LOKI_USE_LINENOISE
/* Convert 256-color palette index to RGB.
 * Handles: 0-15 (standard), 16-231 (6x6x6 cube), 232-255 (grayscale) */
static void color256_to_rgb(unsigned char c, int *r, int *g, int *b) {
    if (c == 0) {
        /* Color 0 means "default" - use light gray */
        *r = 200; *g = 200; *b = 200;
    } else if (c < 16) {
        /* Standard ANSI colors */
        static const int ansi[16][3] = {
            {0, 0, 0},       /* 0: black */
            {205, 49, 49},   /* 1: red */
            {13, 188, 121},  /* 2: green */
            {229, 229, 16},  /* 3: yellow */
            {36, 114, 200},  /* 4: blue */
            {188, 63, 188},  /* 5: magenta */
            {17, 168, 205},  /* 6: cyan */
            {229, 229, 229}, /* 7: white */
            {102, 102, 102}, /* 8: bright black */
            {241, 76, 76},   /* 9: bright red */
            {35, 209, 139},  /* 10: bright green */
            {245, 245, 67},  /* 11: bright yellow */
            {59, 142, 234},  /* 12: bright blue */
            {214, 112, 214}, /* 13: bright magenta */
            {41, 184, 219},  /* 14: bright cyan */
            {255, 255, 255}, /* 15: bright white */
        };
        *r = ansi[c][0]; *g = ansi[c][1]; *b = ansi[c][2];
    } else if (c < 232) {
        /* 6x6x6 color cube (indices 16-231) */
        int idx = c - 16;
        int ri = idx / 36;
        int gi = (idx % 36) / 6;
        int bi = idx % 6;
        *r = ri ? (ri * 40 + 55) : 0;
        *g = gi ? (gi * 40 + 55) : 0;
        *b = bi ? (bi * 40 + 55) : 0;
    } else {
        /* Grayscale (indices 232-255) */
        int gray = (c - 232) * 10 + 8;
        *r = gray; *g = gray; *b = gray;
    }
}

/* Helper to set a color entry from a TOK_* value */
static void set_color_from_tok(editor_ctx_t *ctx, int hl_type, syntax_token_t tok) {
    int r, g, b;
    color256_to_rgb(theme_color(tok), &r, &g, &b);
    ctx->view.colors[hl_type].r = r;
    ctx->view.colors[hl_type].g = g;
    ctx->view.colors[hl_type].b = b;
}

/* Apply current theme colors to the editor's color array */
void syntax_apply_theme_colors(editor_ctx_t *ctx) {
    int r, g, b;

    /* Base types (0-8) */

    /* HL_NORMAL - default foreground */
    set_color_from_tok(ctx, HL_NORMAL, TOK_DEFAULT);

    /* HL_NONPRINT - use muted color */
    color256_to_rgb(theme_color(TOK_COMMENT), &r, &g, &b);
    ctx->view.colors[HL_NONPRINT].r = r / 2;
    ctx->view.colors[HL_NONPRINT].g = g / 2;
    ctx->view.colors[HL_NONPRINT].b = b / 2;

    /* HL_COMMENT */
    set_color_from_tok(ctx, HL_COMMENT, TOK_COMMENT);

    /* HL_MLCOMMENT - same as comment */
    set_color_from_tok(ctx, HL_MLCOMMENT, TOK_COMMENT);

    /* HL_KEYWORD1 - primary keywords */
    set_color_from_tok(ctx, HL_KEYWORD1, TOK_KEYWORD);

    /* HL_KEYWORD2 - type keywords */
    set_color_from_tok(ctx, HL_KEYWORD2, TOK_TYPE);

    /* HL_STRING */
    set_color_from_tok(ctx, HL_STRING, TOK_STRING);

    /* HL_NUMBER */
    set_color_from_tok(ctx, HL_NUMBER, TOK_NUMBER);

    /* HL_MATCH - search match highlight (use function color for visibility) */
    set_color_from_tok(ctx, HL_MATCH, TOK_FUNCTION);

    /* Extended types (9-23) */

    /* Functions */
    set_color_from_tok(ctx, HL_FUNCTION, TOK_FUNCTION);
    set_color_from_tok(ctx, HL_FUNCTION_BUILTIN, TOK_FUNCTION_BUILTIN);
    set_color_from_tok(ctx, HL_FUNCTION_CALL, TOK_FUNCTION_CALL);

    /* Variables */
    set_color_from_tok(ctx, HL_VARIABLE_BUILTIN, TOK_VARIABLE_BUILTIN);
    set_color_from_tok(ctx, HL_VARIABLE_PARAMETER, TOK_VARIABLE_PARAMETER);

    /* Operators and punctuation */
    set_color_from_tok(ctx, HL_OPERATOR, TOK_OPERATOR);
    set_color_from_tok(ctx, HL_PUNCTUATION, TOK_PUNCTUATION);

    /* Special types */
    set_color_from_tok(ctx, HL_CONSTRUCTOR, TOK_CONSTRUCTOR);
    set_color_from_tok(ctx, HL_NAMESPACE, TOK_NAMESPACE);
    set_color_from_tok(ctx, HL_LABEL, TOK_LABEL);
    set_color_from_tok(ctx, HL_TAG, TOK_TAG);

    /* Keyword subtypes */
    set_color_from_tok(ctx, HL_KEYWORD_CONTROL, TOK_KEYWORD_CONTROL);
    set_color_from_tok(ctx, HL_KEYWORD_FUNCTION, TOK_KEYWORD_FUNCTION);
    set_color_from_tok(ctx, HL_KEYWORD_RETURN, TOK_KEYWORD_RETURN);

    /* Constants */
    set_color_from_tok(ctx, HL_CONSTANT_BUILTIN, TOK_CONSTANT_BUILTIN);
}
#endif

/* Initialize default syntax highlighting colors.
 * Colors are stored as RGB values and rendered using true color escape codes.
 * When built with linenoise, colors come from the current theme. */
void syntax_init_default_colors(editor_ctx_t *ctx) {
#ifdef LOKI_USE_LINENOISE
    /* Use colors from the current theme */
    syntax_apply_theme_colors(ctx);
#else
    /* Fallback: hardcoded colors matching original appearance */
    /* Base types (0-8) */
    /* HL_NORMAL */
    ctx->view.colors[0].r = 200; ctx->view.colors[0].g = 200; ctx->view.colors[0].b = 200;
    /* HL_NONPRINT */
    ctx->view.colors[1].r = 100; ctx->view.colors[1].g = 100; ctx->view.colors[1].b = 100;
    /* HL_COMMENT */
    ctx->view.colors[2].r = 100; ctx->view.colors[2].g = 100; ctx->view.colors[2].b = 100;
    /* HL_MLCOMMENT */
    ctx->view.colors[3].r = 100; ctx->view.colors[3].g = 100; ctx->view.colors[3].b = 100;
    /* HL_KEYWORD1 */
    ctx->view.colors[4].r = 220; ctx->view.colors[4].g = 100; ctx->view.colors[4].b = 220;
    /* HL_KEYWORD2 */
    ctx->view.colors[5].r = 100; ctx->view.colors[5].g = 220; ctx->view.colors[5].b = 220;
    /* HL_STRING */
    ctx->view.colors[6].r = 220; ctx->view.colors[6].g = 220; ctx->view.colors[6].b = 100;
    /* HL_NUMBER */
    ctx->view.colors[7].r = 200; ctx->view.colors[7].g = 100; ctx->view.colors[7].b = 200;
    /* HL_MATCH */
    ctx->view.colors[8].r = 100; ctx->view.colors[8].g = 150; ctx->view.colors[8].b = 220;

    /* Extended types (9-23) - fallback colors */
    /* HL_FUNCTION - blue/cyan */
    ctx->view.colors[9].r = 80; ctx->view.colors[9].g = 180; ctx->view.colors[9].b = 220;
    /* HL_FUNCTION_BUILTIN - brighter cyan */
    ctx->view.colors[10].r = 100; ctx->view.colors[10].g = 200; ctx->view.colors[10].b = 240;
    /* HL_FUNCTION_CALL - slightly muted cyan */
    ctx->view.colors[11].r = 120; ctx->view.colors[11].g = 180; ctx->view.colors[11].b = 200;
    /* HL_VARIABLE_BUILTIN - orange/red for self/this */
    ctx->view.colors[12].r = 220; ctx->view.colors[12].g = 120; ctx->view.colors[12].b = 100;
    /* HL_VARIABLE_PARAMETER - light orange */
    ctx->view.colors[13].r = 220; ctx->view.colors[13].g = 160; ctx->view.colors[13].b = 100;
    /* HL_OPERATOR - white/light gray */
    ctx->view.colors[14].r = 220; ctx->view.colors[14].g = 220; ctx->view.colors[14].b = 220;
    /* HL_PUNCTUATION - dimmer gray */
    ctx->view.colors[15].r = 150; ctx->view.colors[15].g = 150; ctx->view.colors[15].b = 150;
    /* HL_CONSTRUCTOR - green/teal */
    ctx->view.colors[16].r = 100; ctx->view.colors[16].g = 220; ctx->view.colors[16].b = 180;
    /* HL_NAMESPACE - purple */
    ctx->view.colors[17].r = 180; ctx->view.colors[17].g = 120; ctx->view.colors[17].b = 220;
    /* HL_LABEL - yellow/gold */
    ctx->view.colors[18].r = 220; ctx->view.colors[18].g = 200; ctx->view.colors[18].b = 100;
    /* HL_TAG - blue */
    ctx->view.colors[19].r = 100; ctx->view.colors[19].g = 150; ctx->view.colors[19].b = 220;
    /* HL_KEYWORD_CONTROL - magenta (like keyword1) */
    ctx->view.colors[20].r = 220; ctx->view.colors[20].g = 100; ctx->view.colors[20].b = 220;
    /* HL_KEYWORD_FUNCTION - magenta variant */
    ctx->view.colors[21].r = 200; ctx->view.colors[21].g = 100; ctx->view.colors[21].b = 200;
    /* HL_KEYWORD_RETURN - bright magenta */
    ctx->view.colors[22].r = 240; ctx->view.colors[22].g = 120; ctx->view.colors[22].b = 240;
    /* HL_CONSTANT_BUILTIN - cyan (like keyword2) */
    ctx->view.colors[23].r = 100; ctx->view.colors[23].g = 220; ctx->view.colors[23].b = 220;
#endif
}
