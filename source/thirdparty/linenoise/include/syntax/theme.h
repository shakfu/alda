/* theme.h -- Theme system for syntax highlighting
 *
 * Defines theme structures and built-in themes for syntax highlighting.
 * Themes map semantic token types to 256-color palette values.
 */

#ifndef SYNTAX_THEME_H
#define SYNTAX_THEME_H

/* Token types for syntax highlighting.
 * These are semantic categories that map to tree-sitter capture names. */
typedef enum {
    TOK_DEFAULT = 0,

    /* Keywords */
    TOK_KEYWORD,
    TOK_KEYWORD_CONTROL,      /* if, else, for, while, switch, case */
    TOK_KEYWORD_OPERATOR,     /* and, or, not */
    TOK_KEYWORD_FUNCTION,     /* function, def, fn, lambda */
    TOK_KEYWORD_RETURN,       /* return, yield */
    TOK_KEYWORD_IMPORT,       /* import, require, use, include */
    TOK_KEYWORD_TYPE,         /* type, class, struct, enum, interface */
    TOK_KEYWORD_MODIFIER,     /* public, private, static, const, mut */

    /* Literals */
    TOK_STRING,
    TOK_STRING_ESCAPE,
    TOK_STRING_REGEX,
    TOK_STRING_SPECIAL,
    TOK_NUMBER,
    TOK_NUMBER_FLOAT,
    TOK_BOOLEAN,
    TOK_CONSTANT,
    TOK_CONSTANT_BUILTIN,     /* nil, null, None, undefined */

    /* Comments */
    TOK_COMMENT,
    TOK_COMMENT_DOC,

    /* Functions */
    TOK_FUNCTION,
    TOK_FUNCTION_CALL,
    TOK_FUNCTION_BUILTIN,
    TOK_FUNCTION_METHOD,
    TOK_FUNCTION_MACRO,

    /* Variables */
    TOK_VARIABLE,
    TOK_VARIABLE_BUILTIN,     /* self, this */
    TOK_VARIABLE_PARAMETER,
    TOK_VARIABLE_FIELD,
    TOK_VARIABLE_PROPERTY,

    /* Types */
    TOK_TYPE,
    TOK_TYPE_BUILTIN,
    TOK_TYPE_PARAMETER,
    TOK_TYPE_QUALIFIER,

    /* Operators and punctuation */
    TOK_OPERATOR,
    TOK_PUNCTUATION,
    TOK_PUNCTUATION_BRACKET,
    TOK_PUNCTUATION_DELIMITER,

    /* Special */
    TOK_CONSTRUCTOR,
    TOK_NAMESPACE,
    TOK_MODULE,
    TOK_TAG,
    TOK_TAG_ATTRIBUTE,
    TOK_LABEL,
    TOK_PREPROCESSOR,

    /* Errors */
    TOK_ERROR,
    TOK_WARNING,

    /* Must be last */
    TOK_COUNT
} syntax_token_t;

/* Theme structure - maps token types to colors */
typedef struct {
    const char *name;
    const char *description;
    unsigned char colors[TOK_COUNT];
} syntax_theme_t;

/* Get color for a token type from the current theme */
unsigned char theme_color(syntax_token_t token);

/* Set the current theme */
void theme_set(const syntax_theme_t *theme);

/* Get the current theme */
const syntax_theme_t *theme_get(void);

/* Get a built-in theme by name. Returns NULL if not found. */
const syntax_theme_t *theme_find(const char *name);

/* Get list of available theme names (NULL-terminated array) */
const char **theme_list(void);

/* ========================= Built-in Themes ========================= */

/* Monokai-inspired dark theme */
extern const syntax_theme_t theme_monokai;

/* Solarized Dark theme */
extern const syntax_theme_t theme_solarized_dark;

/* Solarized Light theme */
extern const syntax_theme_t theme_solarized_light;

/* Dracula theme */
extern const syntax_theme_t theme_dracula;

/* Gruvbox Dark theme */
extern const syntax_theme_t theme_gruvbox_dark;

/* Nord theme */
extern const syntax_theme_t theme_nord;

/* One Dark (Atom) theme */
extern const syntax_theme_t theme_one_dark;

/* Simple 16-color theme for basic terminals */
extern const syntax_theme_t theme_basic16;

/* Default theme (alias for monokai) */
extern const syntax_theme_t theme_default;

#endif /* SYNTAX_THEME_H */
