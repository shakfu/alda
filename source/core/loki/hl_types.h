/* hl_types.h - Syntax highlighting type constants
 *
 * This header defines all highlight type constants used by the syntax
 * highlighting subsystem. These map semantic code elements to color indices.
 *
 * The HL_* constants are indices into the editor's color array and correspond
 * to TOK_* token types defined in theme.h for theme color lookups.
 */

#ifndef LOKI_HL_TYPES_H
#define LOKI_HL_TYPES_H

/* ======================= Highlight Type Constants ========================= */

/* Base highlight types (0-8) - original vocabulary */
#define HL_NORMAL           0
#define HL_NONPRINT         1
#define HL_COMMENT          2   /* Single line comment */
#define HL_MLCOMMENT        3   /* Multi-line comment */
#define HL_KEYWORD1         4   /* Generic keywords */
#define HL_KEYWORD2         5   /* Type keywords */
#define HL_STRING           6
#define HL_NUMBER           7
#define HL_MATCH            8   /* Search match */

/* Extended highlight types (9-23) - tree-sitter vocabulary */
#define HL_FUNCTION             9   /* Function definitions */
#define HL_FUNCTION_BUILTIN     10  /* Built-in functions (print, len, etc.) */
#define HL_FUNCTION_CALL        11  /* Function calls */
#define HL_VARIABLE_BUILTIN     12  /* Built-in variables (self, this) */
#define HL_VARIABLE_PARAMETER   13  /* Function parameters */
#define HL_OPERATOR             14  /* Operators (+, -, *, /, etc.) */
#define HL_PUNCTUATION          15  /* Punctuation (brackets, delimiters) */
#define HL_CONSTRUCTOR          16  /* Constructors */
#define HL_NAMESPACE            17  /* Namespaces/modules */
#define HL_LABEL                18  /* Labels (goto targets, markers) */
#define HL_TAG                  19  /* Tags (HTML, XML) */
#define HL_KEYWORD_CONTROL      20  /* Control flow (if, else, for, while) */
#define HL_KEYWORD_FUNCTION     21  /* Function keywords (function, def, fn) */
#define HL_KEYWORD_RETURN       22  /* Return keywords (return, yield) */
#define HL_CONSTANT_BUILTIN     23  /* Built-in constants (nil, null, None) */

/* Additional extended highlight types (24-50) - full tree-sitter vocabulary */
#define HL_VARIABLE             24  /* Regular variables */
#define HL_VARIABLE_FIELD       25  /* Struct/class fields */
#define HL_VARIABLE_PROPERTY    26  /* Object properties */
#define HL_KEYWORD_OPERATOR     27  /* Operator keywords (and, or, not) */
#define HL_KEYWORD_IMPORT       28  /* Import keywords (import, require) */
#define HL_KEYWORD_TYPE         29  /* Type keywords (type, class, struct) */
#define HL_KEYWORD_MODIFIER     30  /* Modifiers (public, private, static) */
#define HL_STRING_ESCAPE        31  /* Escape sequences (\n, \t, etc.) */
#define HL_STRING_REGEX         32  /* Regex patterns */
#define HL_STRING_SPECIAL       33  /* Special strings (f-strings, etc.) */
#define HL_NUMBER_FLOAT         34  /* Floating point numbers */
#define HL_BOOLEAN              35  /* Boolean literals (true, false) */
#define HL_CONSTANT             36  /* General constants */
#define HL_COMMENT_DOC          37  /* Documentation comments */
#define HL_FUNCTION_METHOD      38  /* Method calls */
#define HL_FUNCTION_MACRO       39  /* Macros */
#define HL_TYPE                 40  /* Type names */
#define HL_TYPE_BUILTIN         41  /* Built-in types */
#define HL_TYPE_PARAMETER       42  /* Generic type parameters */
#define HL_TYPE_QUALIFIER       43  /* Type qualifiers (const, volatile) */
#define HL_PUNCTUATION_BRACKET  44  /* Brackets [ ] { } ( ) */
#define HL_PUNCTUATION_DELIMITER 45 /* Delimiters , ; . */
#define HL_MODULE               46  /* Modules */
#define HL_TAG_ATTRIBUTE        47  /* HTML/XML attributes */
#define HL_PREPROCESSOR         48  /* Preprocessor directives */
#define HL_ERROR                49  /* Error highlighting */
#define HL_WARNING              50  /* Warning highlighting */

#define HL_TYPE_COUNT           51  /* Total number of highlight types */

/* ======================= Highlight Flags ================================== */

#define HL_HIGHLIGHT_STRINGS    (1<<0)
#define HL_HIGHLIGHT_NUMBERS    (1<<1)

/* ======================= Syntax Type Constants ============================ */

/* Language-specific syntax highlighter types */
#define HL_TYPE_C               0
#define HL_TYPE_MARKDOWN        1
#define HL_TYPE_CSOUND          2

/* ======================= Code Block Languages ============================= */

/* Code block language constants (for markdown fenced blocks) */
#define CB_LANG_NONE            0
#define CB_LANG_C               1
#define CB_LANG_PYTHON          2
#define CB_LANG_LUA             3
#define CB_LANG_CYTHON          4

/* ======================= CSD Section Constants ============================ */

/* CSD section constants (for Csound .csd files) */
#define CSD_SECTION_NONE        0
#define CSD_SECTION_OPTIONS     1
#define CSD_SECTION_ORCHESTRA   2
#define CSD_SECTION_SCORE       3

#endif /* LOKI_HL_TYPES_H */
