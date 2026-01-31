/* lang_toml.c - TOML language definition loading
 *
 * Loads language definitions from .psnd/languages/ directory.
 */

#include "lang_toml.h"
#include "languages.h"
#include "internal.h"
#include "psnd.h"

#include <toml.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

/* Maximum keywords per language */
#define MAX_KEYWORDS 512
#define MAX_EXTENSIONS 16

/* Parse a TOML array of strings into a NULL-terminated char** array */
static char **parse_string_array(toml_array_t *arr, int max_count) {
    if (!arr) return NULL;

    int count = toml_array_nelem(arr);
    if (count <= 0) return NULL;
    if (count > max_count) count = max_count;

    char **result = calloc(count + 1, sizeof(char *));
    if (!result) return NULL;

    int actual = 0;
    for (int i = 0; i < count; i++) {
        toml_datum_t val = toml_string_at(arr, i);
        if (val.ok) {
            result[actual++] = val.u.s;  /* Takes ownership of string */
        }
    }
    result[actual] = NULL;

    if (actual == 0) {
        free(result);
        return NULL;
    }

    return result;
}

/* Parse keywords with optional type suffix (keyword2 marked with |) */
static char **parse_keywords(toml_array_t *primary, toml_array_t *secondary) {
    int primary_count = primary ? toml_array_nelem(primary) : 0;
    int secondary_count = secondary ? toml_array_nelem(secondary) : 0;
    int total = primary_count + secondary_count;

    if (total <= 0) return NULL;
    if (total > MAX_KEYWORDS) total = MAX_KEYWORDS;

    char **result = calloc(total + 1, sizeof(char *));
    if (!result) return NULL;

    int idx = 0;

    /* Add primary keywords (keyword1) */
    for (int i = 0; i < primary_count && idx < MAX_KEYWORDS; i++) {
        toml_datum_t val = toml_string_at(primary, i);
        if (val.ok) {
            result[idx++] = val.u.s;
        }
    }

    /* Add secondary keywords (keyword2) with | suffix */
    for (int i = 0; i < secondary_count && idx < MAX_KEYWORDS; i++) {
        toml_datum_t val = toml_string_at(secondary, i);
        if (val.ok) {
            /* Append | to mark as keyword2 */
            size_t len = strlen(val.u.s);
            char *kw2 = malloc(len + 2);
            if (kw2) {
                memcpy(kw2, val.u.s, len);
                kw2[len] = '|';
                kw2[len + 1] = '\0';
                result[idx++] = kw2;
            }
            free(val.u.s);  /* Free original */
        }
    }

    result[idx] = NULL;

    if (idx == 0) {
        free(result);
        return NULL;
    }

    return result;
}

int lang_toml_load(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;

    char errbuf[256];
    toml_table_t *conf = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!conf) {
        fprintf(stderr, "lang_toml: parse error in %s: %s\n", path, errbuf);
        return 0;
    }

    /* Allocate language struct */
    struct t_editor_syntax *lang = calloc(1, sizeof(struct t_editor_syntax));
    if (!lang) {
        toml_free(conf);
        return 0;
    }

    /* Parse [language] section */
    toml_table_t *language = toml_table_in(conf, "language");
    if (!language) {
        fprintf(stderr, "lang_toml: missing [language] section in %s\n", path);
        free(lang);
        toml_free(conf);
        return 0;
    }

    /* Get extensions (required) */
    toml_array_t *ext_arr = toml_array_in(language, "extensions");
    lang->filematch = parse_string_array(ext_arr, MAX_EXTENSIONS);
    if (!lang->filematch) {
        fprintf(stderr, "lang_toml: missing extensions in %s\n", path);
        free(lang);
        toml_free(conf);
        return 0;
    }

    /* Get line comment (optional) */
    toml_datum_t line_comment = toml_string_in(language, "line_comment");
    if (line_comment.ok) {
        strncpy(lang->singleline_comment_start, line_comment.u.s,
                sizeof(lang->singleline_comment_start) - 1);
        lang->singleline_comment_start[sizeof(lang->singleline_comment_start) - 1] = '\0';
        free(line_comment.u.s);
    } else {
        lang->singleline_comment_start[0] = '\0';
    }

    /* Get multiline comments (optional) */
    toml_datum_t ml_start = toml_string_in(language, "multiline_comment_start");
    toml_datum_t ml_end = toml_string_in(language, "multiline_comment_end");
    if (ml_start.ok) {
        strncpy(lang->multiline_comment_start, ml_start.u.s,
                sizeof(lang->multiline_comment_start) - 1);
        lang->multiline_comment_start[sizeof(lang->multiline_comment_start) - 1] = '\0';
        free(ml_start.u.s);
    } else {
        lang->multiline_comment_start[0] = '\0';
    }
    if (ml_end.ok) {
        strncpy(lang->multiline_comment_end, ml_end.u.s,
                sizeof(lang->multiline_comment_end) - 1);
        lang->multiline_comment_end[sizeof(lang->multiline_comment_end) - 1] = '\0';
        free(ml_end.u.s);
    } else {
        lang->multiline_comment_end[0] = '\0';
    }

    /* Get separators (optional, default to common set) */
    toml_datum_t sep = toml_string_in(language, "separators");
    if (sep.ok) {
        lang->separators = sep.u.s;
    } else {
        lang->separators = strdup(",.()+-/*=~%<>[]{}:;");
    }

    /* Parse [highlighting] section */
    toml_table_t *highlighting = toml_table_in(conf, "highlighting");
    if (highlighting) {
        toml_datum_t strings = toml_bool_in(highlighting, "strings");
        toml_datum_t numbers = toml_bool_in(highlighting, "numbers");

        if (strings.ok && strings.u.b) {
            lang->flags |= HL_HIGHLIGHT_STRINGS;
        }
        if (numbers.ok && numbers.u.b) {
            lang->flags |= HL_HIGHLIGHT_NUMBERS;
        }
    }

    /* Parse [keywords] section */
    toml_table_t *keywords = toml_table_in(conf, "keywords");
    if (keywords) {
        toml_array_t *primary = toml_array_in(keywords, "primary");
        toml_array_t *secondary = toml_array_in(keywords, "secondary");
        lang->keywords = parse_keywords(primary, secondary);
    }

    /* Default type */
    lang->type = HL_TYPE_C;

    /* Register the language */
    int result = add_dynamic_language(lang);
    if (result != 0) {
        free_dynamic_language(lang);
        toml_free(conf);
        return 0;
    }

    toml_free(conf);
    return 1;
}

/* Scan a directory for .toml files and load them */
static int scan_directory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) return 0;

    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (entry->d_name[0] == '.') continue;

        /* Check for .toml extension */
        size_t len = strlen(entry->d_name);
        if (len < 6) continue;
        if (strcmp(entry->d_name + len - 5, ".toml") != 0) continue;

        /* Build full path */
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        /* Load the language */
        if (lang_toml_load(path)) {
            count++;
        }
    }

    closedir(dir);
    return count;
}

int lang_toml_load_all(void) {
    int count = 0;
    char path[1024];

    /* First, try project-local .psnd/languages/ */
    if (access(".psnd/languages", F_OK) == 0) {
        count += scan_directory(".psnd/languages");
    }

    /* Then, try home directory ~/.psnd/languages/ */
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }

    if (home) {
        snprintf(path, sizeof(path), "%s/.psnd/languages", home);
        if (access(path, F_OK) == 0) {
            count += scan_directory(path);
        }
    }

    return count;
}
