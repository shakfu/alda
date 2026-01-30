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
    "basic16",
    NULL
};

unsigned char theme_color(syntax_token_t token) {
    const syntax_theme_t *theme = current_theme ? current_theme : &theme_monokai;

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
