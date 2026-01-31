/* theme.c -- Theme system implementation
 *
 * Provides built-in themes and theme management functions.
 */

#include "syntax/theme.h"
#include <string.h>
#include <stddef.h>

/* Current active theme */
static const syntax_theme_t *current_theme = NULL;

/* ========================= Built-in Themes ========================= */

/* Monokai-inspired theme - vibrant colors on dark background */
const syntax_theme_t theme_monokai = {
    .name = "monokai",
    .description = "Vibrant colors inspired by Monokai",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Keywords - pinks/magentas */
        [TOK_KEYWORD]             = 197,  /* Hot pink */
        [TOK_KEYWORD_CONTROL]     = 197,
        [TOK_KEYWORD_OPERATOR]    = 197,
        [TOK_KEYWORD_FUNCTION]    = 197,
        [TOK_KEYWORD_RETURN]      = 197,
        [TOK_KEYWORD_IMPORT]      = 197,
        [TOK_KEYWORD_TYPE]        = 81,   /* Cyan for type keywords */
        [TOK_KEYWORD_MODIFIER]    = 197,

        /* Literals */
        [TOK_STRING]              = 186,  /* Yellow/tan */
        [TOK_STRING_ESCAPE]       = 141,  /* Purple for escapes */
        [TOK_STRING_REGEX]        = 186,
        [TOK_STRING_SPECIAL]      = 186,
        [TOK_NUMBER]              = 141,  /* Purple */
        [TOK_NUMBER_FLOAT]        = 141,
        [TOK_BOOLEAN]             = 141,
        [TOK_CONSTANT]            = 141,
        [TOK_CONSTANT_BUILTIN]    = 141,

        /* Comments - gray */
        [TOK_COMMENT]             = 242,
        [TOK_COMMENT_DOC]         = 242,

        /* Functions - green */
        [TOK_FUNCTION]            = 148,  /* Green */
        [TOK_FUNCTION_CALL]       = 148,
        [TOK_FUNCTION_BUILTIN]    = 81,   /* Cyan for builtins */
        [TOK_FUNCTION_METHOD]     = 148,
        [TOK_FUNCTION_MACRO]      = 148,

        /* Variables */
        [TOK_VARIABLE]            = 0,    /* Default/white */
        [TOK_VARIABLE_BUILTIN]    = 208,  /* Orange */
        [TOK_VARIABLE_PARAMETER]  = 208,  /* Orange */
        [TOK_VARIABLE_FIELD]      = 0,
        [TOK_VARIABLE_PROPERTY]   = 0,

        /* Types - cyan */
        [TOK_TYPE]                = 81,
        [TOK_TYPE_BUILTIN]        = 81,
        [TOK_TYPE_PARAMETER]      = 81,
        [TOK_TYPE_QUALIFIER]      = 197,

        /* Operators */
        [TOK_OPERATOR]            = 197,
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 0,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        /* Special */
        [TOK_CONSTRUCTOR]         = 148,
        [TOK_NAMESPACE]           = 81,
        [TOK_MODULE]              = 81,
        [TOK_TAG]                 = 197,
        [TOK_TAG_ATTRIBUTE]       = 148,
        [TOK_LABEL]               = 186,
        [TOK_PREPROCESSOR]        = 197,

        /* Errors */
        [TOK_ERROR]               = 196,
        [TOK_WARNING]             = 214,
    }
};

/* Dracula theme - purple-centric dark theme */
const syntax_theme_t theme_dracula = {
    .name = "dracula",
    .description = "Purple-centric dark theme",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Keywords - pink */
        [TOK_KEYWORD]             = 212,
        [TOK_KEYWORD_CONTROL]     = 212,
        [TOK_KEYWORD_OPERATOR]    = 212,
        [TOK_KEYWORD_FUNCTION]    = 212,
        [TOK_KEYWORD_RETURN]      = 212,
        [TOK_KEYWORD_IMPORT]      = 212,
        [TOK_KEYWORD_TYPE]        = 117,  /* Cyan */
        [TOK_KEYWORD_MODIFIER]    = 212,

        /* Literals */
        [TOK_STRING]              = 228,  /* Yellow */
        [TOK_STRING_ESCAPE]       = 212,
        [TOK_STRING_REGEX]        = 215,
        [TOK_STRING_SPECIAL]      = 228,
        [TOK_NUMBER]              = 141,  /* Purple */
        [TOK_NUMBER_FLOAT]        = 141,
        [TOK_BOOLEAN]             = 141,
        [TOK_CONSTANT]            = 141,
        [TOK_CONSTANT_BUILTIN]    = 141,

        /* Comments */
        [TOK_COMMENT]             = 103,
        [TOK_COMMENT_DOC]         = 103,

        /* Functions - green */
        [TOK_FUNCTION]            = 84,
        [TOK_FUNCTION_CALL]       = 84,
        [TOK_FUNCTION_BUILTIN]    = 117,
        [TOK_FUNCTION_METHOD]     = 84,
        [TOK_FUNCTION_MACRO]      = 84,

        /* Variables */
        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 141,
        [TOK_VARIABLE_PARAMETER]  = 215,
        [TOK_VARIABLE_FIELD]      = 0,
        [TOK_VARIABLE_PROPERTY]   = 0,

        /* Types - cyan */
        [TOK_TYPE]                = 117,
        [TOK_TYPE_BUILTIN]        = 117,
        [TOK_TYPE_PARAMETER]      = 117,
        [TOK_TYPE_QUALIFIER]      = 212,

        /* Operators */
        [TOK_OPERATOR]            = 212,
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 0,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        /* Special */
        [TOK_CONSTRUCTOR]         = 117,
        [TOK_NAMESPACE]           = 212,
        [TOK_MODULE]              = 212,
        [TOK_TAG]                 = 212,
        [TOK_TAG_ATTRIBUTE]       = 84,
        [TOK_LABEL]               = 215,
        [TOK_PREPROCESSOR]        = 212,

        /* Errors */
        [TOK_ERROR]               = 203,
        [TOK_WARNING]             = 215,
    }
};

/* Solarized Dark theme */
const syntax_theme_t theme_solarized_dark = {
    .name = "solarized-dark",
    .description = "Precision colors for dark background",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Solarized palette approximation in 256 colors:
         * base03=234, base02=235, base01=240, base00=241
         * base0=244, base1=245, base2=254, base3=230
         * yellow=136, orange=166, red=160, magenta=125
         * violet=61, blue=33, cyan=37, green=64 */

        /* Keywords - orange/red */
        [TOK_KEYWORD]             = 166,
        [TOK_KEYWORD_CONTROL]     = 166,
        [TOK_KEYWORD_OPERATOR]    = 166,
        [TOK_KEYWORD_FUNCTION]    = 166,
        [TOK_KEYWORD_RETURN]      = 166,
        [TOK_KEYWORD_IMPORT]      = 166,
        [TOK_KEYWORD_TYPE]        = 136,
        [TOK_KEYWORD_MODIFIER]    = 166,

        /* Literals */
        [TOK_STRING]              = 37,   /* Cyan */
        [TOK_STRING_ESCAPE]       = 160,
        [TOK_STRING_REGEX]        = 160,
        [TOK_STRING_SPECIAL]      = 37,
        [TOK_NUMBER]              = 125,  /* Magenta */
        [TOK_NUMBER_FLOAT]        = 125,
        [TOK_BOOLEAN]             = 136,  /* Yellow */
        [TOK_CONSTANT]            = 136,
        [TOK_CONSTANT_BUILTIN]    = 136,

        /* Comments */
        [TOK_COMMENT]             = 240,
        [TOK_COMMENT_DOC]         = 240,

        /* Functions - blue */
        [TOK_FUNCTION]            = 33,
        [TOK_FUNCTION_CALL]       = 33,
        [TOK_FUNCTION_BUILTIN]    = 33,
        [TOK_FUNCTION_METHOD]     = 33,
        [TOK_FUNCTION_MACRO]      = 166,

        /* Variables */
        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 33,
        [TOK_VARIABLE_PARAMETER]  = 0,
        [TOK_VARIABLE_FIELD]      = 0,
        [TOK_VARIABLE_PROPERTY]   = 0,

        /* Types - yellow */
        [TOK_TYPE]                = 136,
        [TOK_TYPE_BUILTIN]        = 136,
        [TOK_TYPE_PARAMETER]      = 136,
        [TOK_TYPE_QUALIFIER]      = 166,

        /* Operators */
        [TOK_OPERATOR]            = 64,   /* Green */
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 0,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        /* Special */
        [TOK_CONSTRUCTOR]         = 136,
        [TOK_NAMESPACE]           = 61,   /* Violet */
        [TOK_MODULE]              = 61,
        [TOK_TAG]                 = 33,
        [TOK_TAG_ATTRIBUTE]       = 136,
        [TOK_LABEL]               = 166,
        [TOK_PREPROCESSOR]        = 166,

        /* Errors */
        [TOK_ERROR]               = 160,
        [TOK_WARNING]             = 166,
    }
};

/* Solarized Light theme */
const syntax_theme_t theme_solarized_light = {
    .name = "solarized-light",
    .description = "Precision colors for light background",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Same colors as solarized dark - the terminal bg changes */
        [TOK_KEYWORD]             = 166,
        [TOK_KEYWORD_CONTROL]     = 166,
        [TOK_KEYWORD_OPERATOR]    = 166,
        [TOK_KEYWORD_FUNCTION]    = 166,
        [TOK_KEYWORD_RETURN]      = 166,
        [TOK_KEYWORD_IMPORT]      = 166,
        [TOK_KEYWORD_TYPE]        = 136,
        [TOK_KEYWORD_MODIFIER]    = 166,

        [TOK_STRING]              = 37,
        [TOK_STRING_ESCAPE]       = 160,
        [TOK_STRING_REGEX]        = 160,
        [TOK_STRING_SPECIAL]      = 37,
        [TOK_NUMBER]              = 125,
        [TOK_NUMBER_FLOAT]        = 125,
        [TOK_BOOLEAN]             = 136,
        [TOK_CONSTANT]            = 136,
        [TOK_CONSTANT_BUILTIN]    = 136,

        [TOK_COMMENT]             = 245,
        [TOK_COMMENT_DOC]         = 245,

        [TOK_FUNCTION]            = 33,
        [TOK_FUNCTION_CALL]       = 33,
        [TOK_FUNCTION_BUILTIN]    = 33,
        [TOK_FUNCTION_METHOD]     = 33,
        [TOK_FUNCTION_MACRO]      = 166,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 33,
        [TOK_VARIABLE_PARAMETER]  = 0,
        [TOK_VARIABLE_FIELD]      = 0,
        [TOK_VARIABLE_PROPERTY]   = 0,

        [TOK_TYPE]                = 136,
        [TOK_TYPE_BUILTIN]        = 136,
        [TOK_TYPE_PARAMETER]      = 136,
        [TOK_TYPE_QUALIFIER]      = 166,

        [TOK_OPERATOR]            = 64,
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 0,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        [TOK_CONSTRUCTOR]         = 136,
        [TOK_NAMESPACE]           = 61,
        [TOK_MODULE]              = 61,
        [TOK_TAG]                 = 33,
        [TOK_TAG_ATTRIBUTE]       = 136,
        [TOK_LABEL]               = 166,
        [TOK_PREPROCESSOR]        = 166,

        [TOK_ERROR]               = 160,
        [TOK_WARNING]             = 166,
    }
};

/* Gruvbox Dark theme */
const syntax_theme_t theme_gruvbox_dark = {
    .name = "gruvbox-dark",
    .description = "Retro groove dark theme",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Gruvbox palette approximation:
         * bg=235, fg=223
         * red=167, green=142, yellow=214, blue=109
         * purple=175, aqua=108, orange=208, gray=245 */

        [TOK_KEYWORD]             = 167,  /* Red */
        [TOK_KEYWORD_CONTROL]     = 167,
        [TOK_KEYWORD_OPERATOR]    = 167,
        [TOK_KEYWORD_FUNCTION]    = 167,
        [TOK_KEYWORD_RETURN]      = 167,
        [TOK_KEYWORD_IMPORT]      = 167,
        [TOK_KEYWORD_TYPE]        = 214,  /* Yellow */
        [TOK_KEYWORD_MODIFIER]    = 167,

        [TOK_STRING]              = 142,  /* Green */
        [TOK_STRING_ESCAPE]       = 208,
        [TOK_STRING_REGEX]        = 208,
        [TOK_STRING_SPECIAL]      = 142,
        [TOK_NUMBER]              = 175,  /* Purple */
        [TOK_NUMBER_FLOAT]        = 175,
        [TOK_BOOLEAN]             = 175,
        [TOK_CONSTANT]            = 175,
        [TOK_CONSTANT_BUILTIN]    = 175,

        [TOK_COMMENT]             = 245,
        [TOK_COMMENT_DOC]         = 245,

        [TOK_FUNCTION]            = 142,  /* Green */
        [TOK_FUNCTION_CALL]       = 142,
        [TOK_FUNCTION_BUILTIN]    = 108,  /* Aqua */
        [TOK_FUNCTION_METHOD]     = 142,
        [TOK_FUNCTION_MACRO]      = 108,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 208,  /* Orange */
        [TOK_VARIABLE_PARAMETER]  = 109,  /* Blue */
        [TOK_VARIABLE_FIELD]      = 0,
        [TOK_VARIABLE_PROPERTY]   = 0,

        [TOK_TYPE]                = 214,  /* Yellow */
        [TOK_TYPE_BUILTIN]        = 214,
        [TOK_TYPE_PARAMETER]      = 214,
        [TOK_TYPE_QUALIFIER]      = 167,

        [TOK_OPERATOR]            = 0,
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 0,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        [TOK_CONSTRUCTOR]         = 214,
        [TOK_NAMESPACE]           = 108,
        [TOK_MODULE]              = 108,
        [TOK_TAG]                 = 167,
        [TOK_TAG_ATTRIBUTE]       = 214,
        [TOK_LABEL]               = 208,
        [TOK_PREPROCESSOR]        = 108,

        [TOK_ERROR]               = 167,
        [TOK_WARNING]             = 208,
    }
};

/* Nord theme - Arctic, north-bluish colors */
const syntax_theme_t theme_nord = {
    .name = "nord",
    .description = "Arctic, north-bluish colors",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Nord palette approximation:
         * polar night: 236, 238, 240, 243
         * snow storm: 254, 254, 254
         * frost: 67 (blue), 73 (cyan), 79 (aqua), 110 (light blue)
         * aurora: 168 (red), 173 (orange), 179 (yellow), 108 (green), 139 (purple) */

        [TOK_KEYWORD]             = 139,  /* Purple */
        [TOK_KEYWORD_CONTROL]     = 139,
        [TOK_KEYWORD_OPERATOR]    = 139,
        [TOK_KEYWORD_FUNCTION]    = 139,
        [TOK_KEYWORD_RETURN]      = 139,
        [TOK_KEYWORD_IMPORT]      = 139,
        [TOK_KEYWORD_TYPE]        = 67,   /* Blue */
        [TOK_KEYWORD_MODIFIER]    = 139,

        [TOK_STRING]              = 108,  /* Green */
        [TOK_STRING_ESCAPE]       = 179,  /* Yellow */
        [TOK_STRING_REGEX]        = 173,
        [TOK_STRING_SPECIAL]      = 108,
        [TOK_NUMBER]              = 139,  /* Purple */
        [TOK_NUMBER_FLOAT]        = 139,
        [TOK_BOOLEAN]             = 139,
        [TOK_CONSTANT]            = 139,
        [TOK_CONSTANT_BUILTIN]    = 139,

        [TOK_COMMENT]             = 243,
        [TOK_COMMENT_DOC]         = 243,

        [TOK_FUNCTION]            = 110,  /* Light blue */
        [TOK_FUNCTION_CALL]       = 110,
        [TOK_FUNCTION_BUILTIN]    = 73,   /* Cyan */
        [TOK_FUNCTION_METHOD]     = 110,
        [TOK_FUNCTION_MACRO]      = 79,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 139,
        [TOK_VARIABLE_PARAMETER]  = 0,
        [TOK_VARIABLE_FIELD]      = 0,
        [TOK_VARIABLE_PROPERTY]   = 0,

        [TOK_TYPE]                = 67,
        [TOK_TYPE_BUILTIN]        = 67,
        [TOK_TYPE_PARAMETER]      = 67,
        [TOK_TYPE_QUALIFIER]      = 139,

        [TOK_OPERATOR]            = 73,
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 0,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        [TOK_CONSTRUCTOR]         = 67,
        [TOK_NAMESPACE]           = 67,
        [TOK_MODULE]              = 67,
        [TOK_TAG]                 = 139,
        [TOK_TAG_ATTRIBUTE]       = 110,
        [TOK_LABEL]               = 179,
        [TOK_PREPROCESSOR]        = 79,

        [TOK_ERROR]               = 168,
        [TOK_WARNING]             = 173,
    }
};

/* One Dark (Atom) theme */
const syntax_theme_t theme_one_dark = {
    .name = "one-dark",
    .description = "Atom One Dark theme",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* One Dark palette approximation:
         * hue-1: 180 (cyan), hue-2: 114 (green), hue-3: 179 (orange)
         * hue-4: 39 (blue), hue-5: 204 (red), hue-5-2: 210 (light red)
         * hue-6: 173 (orange), hue-6-2: 179 (yellow)
         * purple: 176, fg: 252 */

        [TOK_KEYWORD]             = 176,  /* Purple */
        [TOK_KEYWORD_CONTROL]     = 176,
        [TOK_KEYWORD_OPERATOR]    = 176,
        [TOK_KEYWORD_FUNCTION]    = 176,
        [TOK_KEYWORD_RETURN]      = 176,
        [TOK_KEYWORD_IMPORT]      = 176,
        [TOK_KEYWORD_TYPE]        = 180,  /* Cyan */
        [TOK_KEYWORD_MODIFIER]    = 176,

        [TOK_STRING]              = 114,  /* Green */
        [TOK_STRING_ESCAPE]       = 180,
        [TOK_STRING_REGEX]        = 180,
        [TOK_STRING_SPECIAL]      = 114,
        [TOK_NUMBER]              = 173,  /* Orange */
        [TOK_NUMBER_FLOAT]        = 173,
        [TOK_BOOLEAN]             = 173,
        [TOK_CONSTANT]            = 173,
        [TOK_CONSTANT_BUILTIN]    = 173,

        [TOK_COMMENT]             = 242,
        [TOK_COMMENT_DOC]         = 242,

        [TOK_FUNCTION]            = 39,   /* Blue */
        [TOK_FUNCTION_CALL]       = 39,
        [TOK_FUNCTION_BUILTIN]    = 180,
        [TOK_FUNCTION_METHOD]     = 39,
        [TOK_FUNCTION_MACRO]      = 180,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 204,  /* Red */
        [TOK_VARIABLE_PARAMETER]  = 173,
        [TOK_VARIABLE_FIELD]      = 204,
        [TOK_VARIABLE_PROPERTY]   = 204,

        [TOK_TYPE]                = 180,  /* Cyan */
        [TOK_TYPE_BUILTIN]        = 180,
        [TOK_TYPE_PARAMETER]      = 180,
        [TOK_TYPE_QUALIFIER]      = 176,

        [TOK_OPERATOR]            = 180,
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 0,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        [TOK_CONSTRUCTOR]         = 179,
        [TOK_NAMESPACE]           = 179,
        [TOK_MODULE]              = 179,
        [TOK_TAG]                 = 204,
        [TOK_TAG_ATTRIBUTE]       = 173,
        [TOK_LABEL]               = 179,
        [TOK_PREPROCESSOR]        = 176,

        [TOK_ERROR]               = 204,
        [TOK_WARNING]             = 179,
    }
};

/* Basic 16-color theme for terminals without 256-color support */
const syntax_theme_t theme_basic16 = {
    .name = "basic16",
    .description = "Simple 16-color theme for basic terminals",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Using basic ANSI colors 1-15 */
        [TOK_KEYWORD]             = 5,    /* Magenta */
        [TOK_KEYWORD_CONTROL]     = 5,
        [TOK_KEYWORD_OPERATOR]    = 5,
        [TOK_KEYWORD_FUNCTION]    = 5,
        [TOK_KEYWORD_RETURN]      = 5,
        [TOK_KEYWORD_IMPORT]      = 5,
        [TOK_KEYWORD_TYPE]        = 3,    /* Yellow */
        [TOK_KEYWORD_MODIFIER]    = 5,

        [TOK_STRING]              = 2,    /* Green */
        [TOK_STRING_ESCAPE]       = 6,    /* Cyan */
        [TOK_STRING_REGEX]        = 6,
        [TOK_STRING_SPECIAL]      = 2,
        [TOK_NUMBER]              = 3,    /* Yellow */
        [TOK_NUMBER_FLOAT]        = 3,
        [TOK_BOOLEAN]             = 3,
        [TOK_CONSTANT]            = 3,
        [TOK_CONSTANT_BUILTIN]    = 3,

        [TOK_COMMENT]             = 8,    /* Bright black (gray) */
        [TOK_COMMENT_DOC]         = 8,

        [TOK_FUNCTION]            = 4,    /* Blue */
        [TOK_FUNCTION_CALL]       = 4,
        [TOK_FUNCTION_BUILTIN]    = 6,    /* Cyan */
        [TOK_FUNCTION_METHOD]     = 4,
        [TOK_FUNCTION_MACRO]      = 6,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 1,    /* Red */
        [TOK_VARIABLE_PARAMETER]  = 6,
        [TOK_VARIABLE_FIELD]      = 0,
        [TOK_VARIABLE_PROPERTY]   = 0,

        [TOK_TYPE]                = 6,    /* Cyan */
        [TOK_TYPE_BUILTIN]        = 6,
        [TOK_TYPE_PARAMETER]      = 6,
        [TOK_TYPE_QUALIFIER]      = 5,

        [TOK_OPERATOR]            = 0,
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 0,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        [TOK_CONSTRUCTOR]         = 3,
        [TOK_NAMESPACE]           = 6,
        [TOK_MODULE]              = 6,
        [TOK_TAG]                 = 1,
        [TOK_TAG_ATTRIBUTE]       = 3,
        [TOK_LABEL]               = 3,
        [TOK_PREPROCESSOR]        = 5,

        [TOK_ERROR]               = 1,    /* Red */
        [TOK_WARNING]             = 3,    /* Yellow */
    }
};

/* Catppuccin Mocha - Soothing pastel theme */
const syntax_theme_t theme_catppuccin = {
    .name = "catppuccin",
    .description = "Soothing pastel theme (Mocha variant)",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Catppuccin Mocha palette approximation:
         * rosewater=224, flamingo=210, pink=212, mauve=141
         * red=203, maroon=174, peach=209, yellow=222
         * green=120, teal=116, sky=117, sapphire=75
         * blue=111, lavender=147, text=254, subtext=249 */

        [TOK_KEYWORD]             = 141,  /* Mauve */
        [TOK_KEYWORD_CONTROL]     = 141,
        [TOK_KEYWORD_OPERATOR]    = 141,
        [TOK_KEYWORD_FUNCTION]    = 141,
        [TOK_KEYWORD_RETURN]      = 141,
        [TOK_KEYWORD_IMPORT]      = 141,
        [TOK_KEYWORD_TYPE]        = 222,  /* Yellow */
        [TOK_KEYWORD_MODIFIER]    = 141,

        [TOK_STRING]              = 120,  /* Green */
        [TOK_STRING_ESCAPE]       = 212,  /* Pink */
        [TOK_STRING_REGEX]        = 209,
        [TOK_STRING_SPECIAL]      = 120,
        [TOK_NUMBER]              = 209,  /* Peach */
        [TOK_NUMBER_FLOAT]        = 209,
        [TOK_BOOLEAN]             = 209,
        [TOK_CONSTANT]            = 209,
        [TOK_CONSTANT_BUILTIN]    = 209,

        [TOK_COMMENT]             = 243,  /* Overlay0 */
        [TOK_COMMENT_DOC]         = 243,

        [TOK_FUNCTION]            = 111,  /* Blue */
        [TOK_FUNCTION_CALL]       = 111,
        [TOK_FUNCTION_BUILTIN]    = 116,  /* Teal */
        [TOK_FUNCTION_METHOD]     = 111,
        [TOK_FUNCTION_MACRO]      = 141,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 203,  /* Red */
        [TOK_VARIABLE_PARAMETER]  = 174,  /* Maroon */
        [TOK_VARIABLE_FIELD]      = 147,  /* Lavender */
        [TOK_VARIABLE_PROPERTY]   = 147,

        [TOK_TYPE]                = 222,  /* Yellow */
        [TOK_TYPE_BUILTIN]        = 222,
        [TOK_TYPE_PARAMETER]      = 222,
        [TOK_TYPE_QUALIFIER]      = 141,

        [TOK_OPERATOR]            = 117,  /* Sky */
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 249,
        [TOK_PUNCTUATION_DELIMITER] = 249,

        [TOK_CONSTRUCTOR]         = 209,
        [TOK_NAMESPACE]           = 111,
        [TOK_MODULE]              = 111,
        [TOK_TAG]                 = 141,
        [TOK_TAG_ATTRIBUTE]       = 222,
        [TOK_LABEL]               = 75,   /* Sapphire */
        [TOK_PREPROCESSOR]        = 212,

        [TOK_ERROR]               = 203,
        [TOK_WARNING]             = 222,
    }
};

/* Tokyo Night - Dark theme inspired by Tokyo city lights */
const syntax_theme_t theme_tokyo_night = {
    .name = "tokyo-night",
    .description = "Dark theme inspired by Tokyo city lights",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Tokyo Night palette approximation:
         * red=203, orange=215, yellow=224, green=120
         * cyan=116, blue=111, magenta=176, purple=141
         * fg=252, comment=60 */

        [TOK_KEYWORD]             = 176,  /* Magenta */
        [TOK_KEYWORD_CONTROL]     = 176,
        [TOK_KEYWORD_OPERATOR]    = 176,
        [TOK_KEYWORD_FUNCTION]    = 176,
        [TOK_KEYWORD_RETURN]      = 176,
        [TOK_KEYWORD_IMPORT]      = 176,
        [TOK_KEYWORD_TYPE]        = 111,  /* Blue */
        [TOK_KEYWORD_MODIFIER]    = 176,

        [TOK_STRING]              = 120,  /* Green */
        [TOK_STRING_ESCAPE]       = 116,
        [TOK_STRING_REGEX]        = 215,
        [TOK_STRING_SPECIAL]      = 120,
        [TOK_NUMBER]              = 215,  /* Orange */
        [TOK_NUMBER_FLOAT]        = 215,
        [TOK_BOOLEAN]             = 215,
        [TOK_CONSTANT]            = 215,
        [TOK_CONSTANT_BUILTIN]    = 215,

        [TOK_COMMENT]             = 60,   /* Dark blue-gray */
        [TOK_COMMENT_DOC]         = 60,

        [TOK_FUNCTION]            = 111,  /* Blue */
        [TOK_FUNCTION_CALL]       = 111,
        [TOK_FUNCTION_BUILTIN]    = 116,  /* Cyan */
        [TOK_FUNCTION_METHOD]     = 111,
        [TOK_FUNCTION_MACRO]      = 176,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 203,  /* Red */
        [TOK_VARIABLE_PARAMETER]  = 224,  /* Yellow */
        [TOK_VARIABLE_FIELD]      = 116,
        [TOK_VARIABLE_PROPERTY]   = 116,

        [TOK_TYPE]                = 116,  /* Cyan */
        [TOK_TYPE_BUILTIN]        = 116,
        [TOK_TYPE_PARAMETER]      = 116,
        [TOK_TYPE_QUALIFIER]      = 176,

        [TOK_OPERATOR]            = 116,
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 60,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        [TOK_CONSTRUCTOR]         = 116,
        [TOK_NAMESPACE]           = 111,
        [TOK_MODULE]              = 111,
        [TOK_TAG]                 = 203,
        [TOK_TAG_ATTRIBUTE]       = 120,
        [TOK_LABEL]               = 111,
        [TOK_PREPROCESSOR]        = 176,

        [TOK_ERROR]               = 203,
        [TOK_WARNING]             = 224,
    }
};

/* Everforest - Green-based nature theme */
const syntax_theme_t theme_everforest = {
    .name = "everforest",
    .description = "Green-based nature theme",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Everforest Dark palette approximation:
         * bg=235, fg=223
         * red=167, orange=208, yellow=214, green=142
         * aqua=108, blue=109, purple=175, gray=245 */

        [TOK_KEYWORD]             = 167,  /* Red */
        [TOK_KEYWORD_CONTROL]     = 167,
        [TOK_KEYWORD_OPERATOR]    = 167,
        [TOK_KEYWORD_FUNCTION]    = 167,
        [TOK_KEYWORD_RETURN]      = 167,
        [TOK_KEYWORD_IMPORT]      = 167,
        [TOK_KEYWORD_TYPE]        = 214,  /* Yellow */
        [TOK_KEYWORD_MODIFIER]    = 167,

        [TOK_STRING]              = 142,  /* Green */
        [TOK_STRING_ESCAPE]       = 108,  /* Aqua */
        [TOK_STRING_REGEX]        = 208,
        [TOK_STRING_SPECIAL]      = 142,
        [TOK_NUMBER]              = 175,  /* Purple */
        [TOK_NUMBER_FLOAT]        = 175,
        [TOK_BOOLEAN]             = 175,
        [TOK_CONSTANT]            = 175,
        [TOK_CONSTANT_BUILTIN]    = 175,

        [TOK_COMMENT]             = 245,  /* Gray */
        [TOK_COMMENT_DOC]         = 245,

        [TOK_FUNCTION]            = 142,  /* Green */
        [TOK_FUNCTION_CALL]       = 142,
        [TOK_FUNCTION_BUILTIN]    = 108,  /* Aqua */
        [TOK_FUNCTION_METHOD]     = 142,
        [TOK_FUNCTION_MACRO]      = 108,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 167,
        [TOK_VARIABLE_PARAMETER]  = 109,  /* Blue */
        [TOK_VARIABLE_FIELD]      = 0,
        [TOK_VARIABLE_PROPERTY]   = 0,

        [TOK_TYPE]                = 214,  /* Yellow */
        [TOK_TYPE_BUILTIN]        = 214,
        [TOK_TYPE_PARAMETER]      = 214,
        [TOK_TYPE_QUALIFIER]      = 167,

        [TOK_OPERATOR]            = 208,  /* Orange */
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 0,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        [TOK_CONSTRUCTOR]         = 214,
        [TOK_NAMESPACE]           = 108,
        [TOK_MODULE]              = 108,
        [TOK_TAG]                 = 208,
        [TOK_TAG_ATTRIBUTE]       = 142,
        [TOK_LABEL]               = 214,
        [TOK_PREPROCESSOR]        = 108,

        [TOK_ERROR]               = 167,
        [TOK_WARNING]             = 214,
    }
};

/* Kanagawa - Inspired by Hokusai's The Great Wave */
const syntax_theme_t theme_kanagawa = {
    .name = "kanagawa",
    .description = "Inspired by Hokusai's The Great Wave",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Kanagawa palette approximation:
         * fujiWhite=254, oldWhite=223, sumiInk=235
         * waveBlue=67, springBlue=110, crystalBlue=75
         * springGreen=114, boatYellow=222, carpYellow=228
         * sakuraPink=218, peachRed=203, surimiOrange=209
         * roninYellow=214, autumnGreen=107, autumnRed=167
         * waveAqua=73, dragonBlue=109 */

        [TOK_KEYWORD]             = 176,  /* Oniviolet */
        [TOK_KEYWORD_CONTROL]     = 176,
        [TOK_KEYWORD_OPERATOR]    = 203,  /* Peach red */
        [TOK_KEYWORD_FUNCTION]    = 176,
        [TOK_KEYWORD_RETURN]      = 176,
        [TOK_KEYWORD_IMPORT]      = 176,
        [TOK_KEYWORD_TYPE]        = 73,   /* Wave aqua */
        [TOK_KEYWORD_MODIFIER]    = 176,

        [TOK_STRING]              = 114,  /* Spring green */
        [TOK_STRING_ESCAPE]       = 209,  /* Surimi orange */
        [TOK_STRING_REGEX]        = 214,
        [TOK_STRING_SPECIAL]      = 114,
        [TOK_NUMBER]              = 218,  /* Sakura pink */
        [TOK_NUMBER_FLOAT]        = 218,
        [TOK_BOOLEAN]             = 209,
        [TOK_CONSTANT]            = 209,
        [TOK_CONSTANT_BUILTIN]    = 209,

        [TOK_COMMENT]             = 102,  /* Fuji gray */
        [TOK_COMMENT_DOC]         = 102,

        [TOK_FUNCTION]            = 75,   /* Crystal blue */
        [TOK_FUNCTION_CALL]       = 75,
        [TOK_FUNCTION_BUILTIN]    = 73,   /* Wave aqua */
        [TOK_FUNCTION_METHOD]     = 75,
        [TOK_FUNCTION_MACRO]      = 209,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 203,  /* Peach red */
        [TOK_VARIABLE_PARAMETER]  = 228,  /* Carp yellow */
        [TOK_VARIABLE_FIELD]      = 110,  /* Spring blue */
        [TOK_VARIABLE_PROPERTY]   = 110,

        [TOK_TYPE]                = 73,   /* Wave aqua */
        [TOK_TYPE_BUILTIN]        = 73,
        [TOK_TYPE_PARAMETER]      = 73,
        [TOK_TYPE_QUALIFIER]      = 176,

        [TOK_OPERATOR]            = 203,  /* Peach red */
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 102,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        [TOK_CONSTRUCTOR]         = 228,
        [TOK_NAMESPACE]           = 67,   /* Wave blue */
        [TOK_MODULE]              = 67,
        [TOK_TAG]                 = 167,  /* Autumn red */
        [TOK_TAG_ATTRIBUTE]       = 107,  /* Autumn green */
        [TOK_LABEL]               = 222,  /* Boat yellow */
        [TOK_PREPROCESSOR]        = 176,

        [TOK_ERROR]               = 203,
        [TOK_WARNING]             = 214,
    }
};

/* Rose Pine - Elegant, soft theme */
const syntax_theme_t theme_rose_pine = {
    .name = "rose-pine",
    .description = "Elegant, soft theme",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Rose Pine palette approximation:
         * base=235, surface=236, overlay=238
         * muted=242, subtle=245, text=254
         * love=204, gold=179, rose=211
         * pine=73, foam=116, iris=141 */

        [TOK_KEYWORD]             = 73,   /* Pine */
        [TOK_KEYWORD_CONTROL]     = 73,
        [TOK_KEYWORD_OPERATOR]    = 73,
        [TOK_KEYWORD_FUNCTION]    = 73,
        [TOK_KEYWORD_RETURN]      = 73,
        [TOK_KEYWORD_IMPORT]      = 73,
        [TOK_KEYWORD_TYPE]        = 116,  /* Foam */
        [TOK_KEYWORD_MODIFIER]    = 73,

        [TOK_STRING]              = 179,  /* Gold */
        [TOK_STRING_ESCAPE]       = 204,  /* Love */
        [TOK_STRING_REGEX]        = 211,
        [TOK_STRING_SPECIAL]      = 179,
        [TOK_NUMBER]              = 211,  /* Rose */
        [TOK_NUMBER_FLOAT]        = 211,
        [TOK_BOOLEAN]             = 211,
        [TOK_CONSTANT]            = 211,
        [TOK_CONSTANT_BUILTIN]    = 211,

        [TOK_COMMENT]             = 242,  /* Muted */
        [TOK_COMMENT_DOC]         = 245,  /* Subtle */

        [TOK_FUNCTION]            = 211,  /* Rose */
        [TOK_FUNCTION_CALL]       = 211,
        [TOK_FUNCTION_BUILTIN]    = 116,  /* Foam */
        [TOK_FUNCTION_METHOD]     = 211,
        [TOK_FUNCTION_MACRO]      = 141,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 204,  /* Love */
        [TOK_VARIABLE_PARAMETER]  = 141,  /* Iris */
        [TOK_VARIABLE_FIELD]      = 116,
        [TOK_VARIABLE_PROPERTY]   = 116,

        [TOK_TYPE]                = 116,  /* Foam */
        [TOK_TYPE_BUILTIN]        = 116,
        [TOK_TYPE_PARAMETER]      = 116,
        [TOK_TYPE_QUALIFIER]      = 73,

        [TOK_OPERATOR]            = 245,  /* Subtle */
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 242,
        [TOK_PUNCTUATION_DELIMITER] = 242,

        [TOK_CONSTRUCTOR]         = 116,
        [TOK_NAMESPACE]           = 141,  /* Iris */
        [TOK_MODULE]              = 141,
        [TOK_TAG]                 = 204,
        [TOK_TAG_ATTRIBUTE]       = 179,
        [TOK_LABEL]               = 179,
        [TOK_PREPROCESSOR]        = 141,

        [TOK_ERROR]               = 204,
        [TOK_WARNING]             = 179,
    }
};

/* Palenight - Material palenight theme */
const syntax_theme_t theme_palenight = {
    .name = "palenight",
    .description = "Material palenight theme",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Material Palenight palette approximation:
         * bg=236, fg=254
         * red=204, orange=209, yellow=222, green=114
         * cyan=116, blue=111, purple=176, pink=212 */

        [TOK_KEYWORD]             = 176,  /* Purple */
        [TOK_KEYWORD_CONTROL]     = 176,
        [TOK_KEYWORD_OPERATOR]    = 116,  /* Cyan */
        [TOK_KEYWORD_FUNCTION]    = 176,
        [TOK_KEYWORD_RETURN]      = 176,
        [TOK_KEYWORD_IMPORT]      = 176,
        [TOK_KEYWORD_TYPE]        = 222,  /* Yellow */
        [TOK_KEYWORD_MODIFIER]    = 176,

        [TOK_STRING]              = 114,  /* Green */
        [TOK_STRING_ESCAPE]       = 116,
        [TOK_STRING_REGEX]        = 204,
        [TOK_STRING_SPECIAL]      = 114,
        [TOK_NUMBER]              = 209,  /* Orange */
        [TOK_NUMBER_FLOAT]        = 209,
        [TOK_BOOLEAN]             = 209,
        [TOK_CONSTANT]            = 209,
        [TOK_CONSTANT_BUILTIN]    = 209,

        [TOK_COMMENT]             = 60,   /* Gray-blue */
        [TOK_COMMENT_DOC]         = 60,

        [TOK_FUNCTION]            = 111,  /* Blue */
        [TOK_FUNCTION_CALL]       = 111,
        [TOK_FUNCTION_BUILTIN]    = 116,  /* Cyan */
        [TOK_FUNCTION_METHOD]     = 111,
        [TOK_FUNCTION_MACRO]      = 116,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 204,  /* Red */
        [TOK_VARIABLE_PARAMETER]  = 209,
        [TOK_VARIABLE_FIELD]      = 204,
        [TOK_VARIABLE_PROPERTY]   = 204,

        [TOK_TYPE]                = 222,  /* Yellow */
        [TOK_TYPE_BUILTIN]        = 222,
        [TOK_TYPE_PARAMETER]      = 222,
        [TOK_TYPE_QUALIFIER]      = 176,

        [TOK_OPERATOR]            = 116,
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 0,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        [TOK_CONSTRUCTOR]         = 222,
        [TOK_NAMESPACE]           = 222,
        [TOK_MODULE]              = 222,
        [TOK_TAG]                 = 204,
        [TOK_TAG_ATTRIBUTE]       = 176,
        [TOK_LABEL]               = 212,  /* Pink */
        [TOK_PREPROCESSOR]        = 116,

        [TOK_ERROR]               = 204,
        [TOK_WARNING]             = 222,
    }
};

/* Ayu Dark - Warm dark theme */
const syntax_theme_t theme_ayu_dark = {
    .name = "ayu-dark",
    .description = "Warm dark theme",
    .colors = {
        [TOK_DEFAULT]             = 0,

        /* Ayu Dark palette approximation:
         * bg=234, fg=252
         * red=203, orange=209, yellow=222, green=150
         * cyan=80, blue=74, purple=176, accent=209 */

        [TOK_KEYWORD]             = 209,  /* Orange (accent) */
        [TOK_KEYWORD_CONTROL]     = 209,
        [TOK_KEYWORD_OPERATOR]    = 209,
        [TOK_KEYWORD_FUNCTION]    = 209,
        [TOK_KEYWORD_RETURN]      = 209,
        [TOK_KEYWORD_IMPORT]      = 209,
        [TOK_KEYWORD_TYPE]        = 80,   /* Cyan */
        [TOK_KEYWORD_MODIFIER]    = 209,

        [TOK_STRING]              = 150,  /* Green */
        [TOK_STRING_ESCAPE]       = 80,
        [TOK_STRING_REGEX]        = 150,
        [TOK_STRING_SPECIAL]      = 150,
        [TOK_NUMBER]              = 176,  /* Purple */
        [TOK_NUMBER_FLOAT]        = 176,
        [TOK_BOOLEAN]             = 209,
        [TOK_CONSTANT]            = 176,
        [TOK_CONSTANT_BUILTIN]    = 209,

        [TOK_COMMENT]             = 59,   /* Gray */
        [TOK_COMMENT_DOC]         = 59,

        [TOK_FUNCTION]            = 222,  /* Yellow */
        [TOK_FUNCTION_CALL]       = 222,
        [TOK_FUNCTION_BUILTIN]    = 80,   /* Cyan */
        [TOK_FUNCTION_METHOD]     = 222,
        [TOK_FUNCTION_MACRO]      = 209,

        [TOK_VARIABLE]            = 0,
        [TOK_VARIABLE_BUILTIN]    = 203,  /* Red */
        [TOK_VARIABLE_PARAMETER]  = 74,   /* Blue */
        [TOK_VARIABLE_FIELD]      = 0,
        [TOK_VARIABLE_PROPERTY]   = 0,

        [TOK_TYPE]                = 80,   /* Cyan */
        [TOK_TYPE_BUILTIN]        = 80,
        [TOK_TYPE_PARAMETER]      = 80,
        [TOK_TYPE_QUALIFIER]      = 209,

        [TOK_OPERATOR]            = 209,
        [TOK_PUNCTUATION]         = 0,
        [TOK_PUNCTUATION_BRACKET] = 59,
        [TOK_PUNCTUATION_DELIMITER] = 0,

        [TOK_CONSTRUCTOR]         = 80,
        [TOK_NAMESPACE]           = 74,   /* Blue */
        [TOK_MODULE]              = 74,
        [TOK_TAG]                 = 209,
        [TOK_TAG_ATTRIBUTE]       = 80,
        [TOK_LABEL]               = 222,
        [TOK_PREPROCESSOR]        = 209,

        [TOK_ERROR]               = 203,
        [TOK_WARNING]             = 222,
    }
};

/* Default theme alias */
const syntax_theme_t theme_default = {
    .name = "default",
    .description = "Default theme (Monokai)",
    .colors = { 0 }  /* Will use monokai via fallback */
};

/* ========================= Theme Management ========================= */

/* List of all built-in themes for lookup */
static const syntax_theme_t *builtin_themes[] = {
    &theme_monokai,
    &theme_dracula,
    &theme_solarized_dark,
    &theme_solarized_light,
    &theme_gruvbox_dark,
    &theme_nord,
    &theme_one_dark,
    &theme_catppuccin,
    &theme_tokyo_night,
    &theme_everforest,
    &theme_kanagawa,
    &theme_rose_pine,
    &theme_palenight,
    &theme_ayu_dark,
    &theme_basic16,
    NULL
};

/* Theme names for listing */
static const char *theme_names[] = {
    "monokai",
    "dracula",
    "solarized-dark",
    "solarized-light",
    "gruvbox-dark",
    "nord",
    "one-dark",
    "catppuccin",
    "tokyo-night",
    "everforest",
    "kanagawa",
    "rose-pine",
    "palenight",
    "ayu-dark",
    "basic16",
    NULL
};

unsigned char theme_color(syntax_token_t token) {
    const syntax_theme_t *theme = current_theme ? current_theme : &theme_nord;

    if (token < 0 || token >= TOK_COUNT) {
        return 0;
    }

    return theme->colors[token];
}

void theme_set(const syntax_theme_t *theme) {
    current_theme = theme;
}

const syntax_theme_t *theme_get(void) {
    return current_theme ? current_theme : &theme_monokai;
}

const syntax_theme_t *theme_find(const char *name) {
    int i;

    if (name == NULL) {
        return NULL;
    }

    /* Check for "default" alias */
    if (strcmp(name, "default") == 0) {
        return &theme_monokai;
    }

    for (i = 0; builtin_themes[i] != NULL; i++) {
        if (strcmp(builtin_themes[i]->name, name) == 0) {
            return builtin_themes[i];
        }
    }

    return NULL;
}

const char **theme_list(void) {
    return theme_names;
}
