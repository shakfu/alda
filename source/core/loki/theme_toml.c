/* theme_toml.c - TOML theme loading
 *
 * Loads theme definitions from .psnd/themes directory.
 * Falls back to built-in C themes if TOML theme not found.
 */

#include "theme_toml.h"
#include "internal.h"
#include "psnd.h"
#include "syntax.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define access _access
#define F_OK 0

/* Minimal dirent compatibility for Windows */
struct dirent { char d_name[MAX_PATH]; };
typedef struct { HANDLE hFind; WIN32_FIND_DATAA ffd; struct dirent ent; int first; } DIR;
static DIR *opendir(const char *path) {
    DIR *d = (DIR*)malloc(sizeof(DIR));
    if (!d) return NULL;
    char search[MAX_PATH];
    snprintf(search, MAX_PATH, "%s\\*", path);
    d->hFind = FindFirstFileA(search, &d->ffd);
    if (d->hFind == INVALID_HANDLE_VALUE) { free(d); return NULL; }
    d->first = 1;
    return d;
}
static struct dirent *readdir(DIR *d) {
    if (!d) return NULL;
    if (d->first) { d->first = 0; }
    else if (!FindNextFileA(d->hFind, &d->ffd)) return NULL;
    strncpy(d->ent.d_name, d->ffd.cFileName, MAX_PATH - 1);
    d->ent.d_name[MAX_PATH - 1] = '\0';
    return &d->ent;
}
static void closedir(DIR *d) { if (d) { FindClose(d->hFind); free(d); } }
#else
#include <unistd.h>
#include <dirent.h>
#endif

#include <toml.h>

/* Maximum themes we can track */
#define MAX_TOML_THEMES 64

/* Static storage for theme names */
static char *g_theme_names[MAX_TOML_THEMES + 1];
static int g_theme_count = 0;
static int g_themes_scanned = 0;

/* Map of color names to HL_* constants */
static const struct {
    const char *name;
    int hl_type;
} color_map[] = {
    /* Base types (0-8) */
    {"normal",    HL_NORMAL},
    {"nonprint",  HL_NONPRINT},
    {"comment",   HL_COMMENT},
    {"mlcomment", HL_MLCOMMENT},
    {"keyword1",  HL_KEYWORD1},
    {"keyword2",  HL_KEYWORD2},
    {"string",    HL_STRING},
    {"number",    HL_NUMBER},
    {"match",     HL_MATCH},

    /* Functions */
    {"function",         HL_FUNCTION},
    {"function_builtin", HL_FUNCTION_BUILTIN},
    {"function_call",    HL_FUNCTION_CALL},
    {"function_method",  HL_FUNCTION_METHOD},
    {"function_macro",   HL_FUNCTION_MACRO},

    /* Variables */
    {"variable",           HL_VARIABLE},
    {"variable_builtin",   HL_VARIABLE_BUILTIN},
    {"variable_parameter", HL_VARIABLE_PARAMETER},
    {"variable_field",     HL_VARIABLE_FIELD},
    {"variable_property",  HL_VARIABLE_PROPERTY},

    /* Keywords */
    {"keyword_control",   HL_KEYWORD_CONTROL},
    {"keyword_function",  HL_KEYWORD_FUNCTION},
    {"keyword_return",    HL_KEYWORD_RETURN},
    {"keyword_operator",  HL_KEYWORD_OPERATOR},
    {"keyword_import",    HL_KEYWORD_IMPORT},
    {"keyword_type",      HL_KEYWORD_TYPE},
    {"keyword_modifier",  HL_KEYWORD_MODIFIER},

    /* Strings */
    {"string_escape",  HL_STRING_ESCAPE},
    {"string_regex",   HL_STRING_REGEX},
    {"string_special", HL_STRING_SPECIAL},

    /* Numbers */
    {"number_float", HL_NUMBER_FLOAT},

    /* Literals */
    {"boolean",          HL_BOOLEAN},
    {"constant",         HL_CONSTANT},
    {"constant_builtin", HL_CONSTANT_BUILTIN},

    /* Comments */
    {"comment_doc", HL_COMMENT_DOC},

    /* Types */
    {"type",           HL_TYPE},
    {"type_builtin",   HL_TYPE_BUILTIN},
    {"type_parameter", HL_TYPE_PARAMETER},
    {"type_qualifier", HL_TYPE_QUALIFIER},

    /* Operators/Punctuation */
    {"operator",              HL_OPERATOR},
    {"punctuation",           HL_PUNCTUATION},
    {"punctuation_bracket",   HL_PUNCTUATION_BRACKET},
    {"punctuation_delimiter", HL_PUNCTUATION_DELIMITER},

    /* Special */
    {"constructor",    HL_CONSTRUCTOR},
    {"namespace",      HL_NAMESPACE},
    {"module",         HL_MODULE},
    {"label",          HL_LABEL},
    {"tag",            HL_TAG},
    {"tag_attribute",  HL_TAG_ATTRIBUTE},
    {"preprocessor",   HL_PREPROCESSOR},

    /* Diagnostics */
    {"error",   HL_ERROR},
    {"warning", HL_WARNING},

    {NULL, -1}
};

/* Get HL_* constant for a color name */
static int color_name_to_hl(const char *name) {
    for (int i = 0; color_map[i].name; i++) {
        if (strcmp(color_map[i].name, name) == 0) {
            return color_map[i].hl_type;
        }
    }
    return -1;
}

/* Parse RGB array [r, g, b] and apply to context */
static void apply_color_array(editor_ctx_t *ctx, int hl_type, toml_array_t *arr) {
    if (!arr) return;
    if (toml_array_nelem(arr) < 3) return;

    toml_datum_t r = toml_int_at(arr, 0);
    toml_datum_t g = toml_int_at(arr, 1);
    toml_datum_t b = toml_int_at(arr, 2);

    if (r.ok && g.ok && b.ok) {
        int rv = (int)r.u.i;
        int gv = (int)g.u.i;
        int bv = (int)b.u.i;

        if (rv >= 0 && rv <= 255 && gv >= 0 && gv <= 255 && bv >= 0 && bv <= 255) {
            ctx->view.colors[hl_type].r = rv;
            ctx->view.colors[hl_type].g = gv;
            ctx->view.colors[hl_type].b = bv;
        }
    }
}

/* Parse [colors] section and apply to context */
static void apply_colors_section(editor_ctx_t *ctx, toml_table_t *colors) {
    if (!colors) return;

    int i = 0;
    const char *key;
    while ((key = toml_key_in(colors, i++)) != NULL) {
        int hl_type = color_name_to_hl(key);
        if (hl_type < 0) continue;

        toml_array_t *arr = toml_array_in(colors, key);
        if (arr) {
            apply_color_array(ctx, hl_type, arr);
        }
    }
}

/* Build path to theme file */
static int build_theme_path(char *path, size_t path_size, const char *name,
                           const char *base_dir) {
    int len = snprintf(path, path_size, "%s/" PSND_CONFIG_DIR "/themes/%s.toml",
                       base_dir, name);
    return (len > 0 && (size_t)len < path_size) ? 0 : -1;
}

/* Load a TOML theme by name */
int theme_toml_load(editor_ctx_t *ctx, const char *name) {
    if (!ctx || !name || !name[0]) return 0;

    char path[1024];
    FILE *fp = NULL;

    /* Try project-local first */
    if (build_theme_path(path, sizeof(path), name, ".") == 0) {
        fp = fopen(path, "r");
    }

    /* Try home directory */
    if (!fp) {
        const char *home = getenv("HOME");
        if (home && home[0]) {
            if (build_theme_path(path, sizeof(path), name, home) == 0) {
                fp = fopen(path, "r");
            }
        }
    }

    if (!fp) return 0;

    char errbuf[256];
    toml_table_t *root = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!root) {
        fprintf(stderr, "Theme parse error (%s): %s\n", path, errbuf);
        return 0;
    }

    /* Parse [colors] section */
    toml_table_t *colors = toml_table_in(root, "colors");
    apply_colors_section(ctx, colors);

    toml_free(root);
    return 1;
}

/* Free theme name list */
static void free_theme_names(void) {
    for (int i = 0; i < g_theme_count; i++) {
        free(g_theme_names[i]);
        g_theme_names[i] = NULL;
    }
    g_theme_count = 0;
}

/* Add a theme name to the list (without .toml extension) */
static void add_theme_name(const char *filename) {
    if (g_theme_count >= MAX_TOML_THEMES) return;

    /* Strip .toml extension */
    size_t len = strlen(filename);
    if (len <= 5) return;
    if (strcmp(filename + len - 5, ".toml") != 0) return;

    char *name = malloc(len - 4);  /* len - 5 + 1 for null */
    if (!name) return;

    memcpy(name, filename, len - 5);
    name[len - 5] = '\0';

    /* Check for duplicates */
    for (int i = 0; i < g_theme_count; i++) {
        if (strcmp(g_theme_names[i], name) == 0) {
            free(name);
            return;
        }
    }

    g_theme_names[g_theme_count++] = name;
}

/* Scan a directory for .toml theme files */
static void scan_themes_dir(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && g_theme_count < MAX_TOML_THEMES) {
        if (entry->d_name[0] == '.') continue;

        size_t len = strlen(entry->d_name);
        if (len > 5 && strcmp(entry->d_name + len - 5, ".toml") == 0) {
            add_theme_name(entry->d_name);
        }
    }

    closedir(dir);
}

/* Scan for available TOML themes */
void theme_toml_scan(void) {
    if (g_themes_scanned) {
        free_theme_names();
    }

    char path[1024];

    /* Scan project-local themes */
    snprintf(path, sizeof(path), PSND_CONFIG_DIR "/themes");
    scan_themes_dir(path);

    /* Scan home directory themes */
    const char *home = getenv("HOME");
    if (home && home[0]) {
        snprintf(path, sizeof(path), "%s/" PSND_CONFIG_DIR "/themes", home);
        scan_themes_dir(path);
    }

    g_theme_names[g_theme_count] = NULL;
    g_themes_scanned = 1;
}

/* List available TOML themes */
const char **theme_toml_list(void) {
    if (!g_themes_scanned) {
        theme_toml_scan();
    }
    return (const char **)g_theme_names;
}
