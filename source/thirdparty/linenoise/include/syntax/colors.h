/* colors.h -- 256-color palette for syntax highlighting
 *
 * This header defines color constants for tree-sitter syntax highlighting.
 * Colors use the 256-color ANSI palette:
 *   0-7:     Standard colors (black, red, green, yellow, blue, magenta, cyan, white)
 *   8-15:    Bright colors
 *   16-231:  216 colors in a 6x6x6 RGB cube
 *   232-255: 24 grayscale shades
 *
 * Color value 0 is reserved for "default/no color" in linenoise.
 */

#ifndef SYNTAX_COLORS_H
#define SYNTAX_COLORS_H

/* Special value meaning "use default terminal color" */
#define SYN_COLOR_DEFAULT       0

/* Standard colors (0-7, but we use 1-7 since 0 is default) */
#define SYN_COLOR_BLACK         16   /* Use 16 instead of 0 for actual black */
#define SYN_COLOR_RED           1
#define SYN_COLOR_GREEN         2
#define SYN_COLOR_YELLOW        3
#define SYN_COLOR_BLUE          4
#define SYN_COLOR_MAGENTA       5
#define SYN_COLOR_CYAN          6
#define SYN_COLOR_WHITE         7

/* Bright colors (8-15) */
#define SYN_COLOR_BRIGHT_BLACK  8
#define SYN_COLOR_BRIGHT_RED    9
#define SYN_COLOR_BRIGHT_GREEN  10
#define SYN_COLOR_BRIGHT_YELLOW 11
#define SYN_COLOR_BRIGHT_BLUE   12
#define SYN_COLOR_BRIGHT_MAGENTA 13
#define SYN_COLOR_BRIGHT_CYAN   14
#define SYN_COLOR_BRIGHT_WHITE  15

/* Extended 256-color palette - curated for syntax highlighting */

/* Keywords - purples/magentas */
#define SYN_KEYWORD             171  /* Light purple */
#define SYN_KEYWORD_CONTROL     134  /* Medium purple - if/else/for/while */
#define SYN_KEYWORD_OPERATOR    177  /* Light magenta - and/or/not */
#define SYN_KEYWORD_FUNCTION    141  /* Soft purple - function/def/fn */
#define SYN_KEYWORD_RETURN      176  /* Pink-ish - return/yield */
#define SYN_KEYWORD_IMPORT      139  /* Dusty purple - import/require/use */
#define SYN_KEYWORD_TYPE        133  /* Deep purple - type keywords */

/* Strings - greens */
#define SYN_STRING              114  /* Soft green */
#define SYN_STRING_ESCAPE       220  /* Gold/yellow for escape sequences */
#define SYN_STRING_REGEX        209  /* Orange for regex */
#define SYN_STRING_SPECIAL      157  /* Light green for special strings */

/* Numbers - oranges/yellows */
#define SYN_NUMBER              215  /* Orange */
#define SYN_NUMBER_FLOAT        216  /* Lighter orange for floats */

/* Comments - grays */
#define SYN_COMMENT             102  /* Medium gray */
#define SYN_COMMENT_DOC         109  /* Slightly blue-gray for doc comments */

/* Functions - blues */
#define SYN_FUNCTION            39   /* Bright blue */
#define SYN_FUNCTION_CALL       75   /* Light blue */
#define SYN_FUNCTION_BUILTIN    81   /* Cyan-blue for builtins */
#define SYN_FUNCTION_METHOD     74   /* Teal for methods */

/* Variables and identifiers */
#define SYN_VARIABLE            252  /* Light gray - normal variables */
#define SYN_VARIABLE_BUILTIN    37   /* Teal - self/this */
#define SYN_VARIABLE_PARAMETER  180  /* Tan/beige for parameters */
#define SYN_VARIABLE_FIELD      152  /* Light cyan for fields */

/* Types - teals/cyans */
#define SYN_TYPE                37   /* Teal */
#define SYN_TYPE_BUILTIN        73   /* Cyan for built-in types */
#define SYN_TYPE_PARAMETER      80   /* Light teal for type params */

/* Constants */
#define SYN_CONSTANT            220  /* Gold */
#define SYN_CONSTANT_BUILTIN    214  /* Orange for nil/null/None */
#define SYN_BOOLEAN             214  /* Orange for true/false */

/* Operators and punctuation */
#define SYN_OPERATOR            255  /* White */
#define SYN_PUNCTUATION         245  /* Light gray */
#define SYN_PUNCTUATION_BRACKET 250  /* Slightly brighter for brackets */
#define SYN_PUNCTUATION_DELIM   243  /* Dimmer for delimiters */

/* Special */
#define SYN_CONSTRUCTOR         179  /* Tan for constructors/tables */
#define SYN_NAMESPACE           37   /* Teal for namespaces/modules */
#define SYN_TAG                 167  /* Red-ish for tags (HTML/XML) */
#define SYN_ATTRIBUTE           143  /* Olive for attributes */
#define SYN_PROPERTY            152  /* Light cyan for properties */
#define SYN_LABEL               185  /* Yellow-green for labels */

/* Errors and warnings */
#define SYN_ERROR               196  /* Bright red */
#define SYN_WARNING             214  /* Orange */

/* Diffs */
#define SYN_DIFF_ADD            114  /* Green */
#define SYN_DIFF_DELETE         203  /* Red */
#define SYN_DIFF_CHANGE         215  /* Orange */

#endif /* SYNTAX_COLORS_H */
