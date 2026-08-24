/**
 * @file test_examples.c
 * @brief Parses and interprets every bundled Alda example.
 *
 * The examples/ directory ships with psnd and is the closest thing the project
 * has to a real-world corpus, but nothing used to exercise it. Nine of the forty
 * files were unparseable by the very parser that shipped alongside them, and a
 * tenth parsed but could not be interpreted because it declares more parts than
 * the context allowed. Both gaps were found by differential-testing against the
 * aldakit implementation; the parser cases are covered construct-by-construct in
 * test_parser.c.
 *
 * This test is the backstop: if a grammar or interpreter change breaks a real
 * score, it fails here even when no unit test happens to cover that construct.
 * It runs the full pipeline - parse, interpret, schedule - and checks that the
 * score actually produced events, so a file that parses into silence is a
 * failure rather than a pass.
 */

#include "test_framework.h"
#include <alda/ast.h>
#include <alda/context.h>
#include <alda/interpreter.h>
#include <alda/parser.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ALDA_EXAMPLES_DIR
#error "ALDA_EXAMPLES_DIR must be defined by the build"
#endif

#define MAX_EXAMPLES 256
#define MAX_NAME     256
/* Sized so "<dir>/<name>" can never be truncated. */
#define MAX_PATH_LEN (sizeof(ALDA_EXAMPLES_DIR) + MAX_NAME + 2)

static char g_names[MAX_EXAMPLES][MAX_NAME];

/* Join the examples directory and a filename. Built with explicit lengths
 * rather than snprintf: the compiler cannot see a bound on `name` through the
 * pointer and warns about truncation that MAX_PATH_LEN already rules out. */
static void example_path(char *out, size_t out_size, const char *name) {
    const size_t dir_len = sizeof(ALDA_EXAMPLES_DIR) - 1;
    size_t name_len = strnlen(name, MAX_NAME);

    if (dir_len + 1 + name_len + 1 > out_size) {
        out[0] = '\0';
        return;
    }

    memcpy(out, ALDA_EXAMPLES_DIR, dir_len);
    out[dir_len] = '/';
    memcpy(out + dir_len + 1, name, name_len);
    out[dir_len + 1 + name_len] = '\0';
}

/* A handful of examples are documentation or attribute demos that legitimately
 * schedule no note events. They must still parse and interpret cleanly; they
 * are only exempt from the "produced some notes" check. */
static int may_be_silent(const char *name) {
    static const char *exempt[] = { NULL };
    for (int i = 0; exempt[i]; i++) {
        if (strcmp(name, exempt[i]) == 0) return 1;
    }
    return 0;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long size = ftell(f);
    if (size < 0) { fclose(f); return NULL; }
    rewind(f);

    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t got = fread(buf, 1, (size_t)size, f);
    buf[got] = '\0';
    fclose(f);
    return buf;
}

static int has_alda_suffix(const char *name) {
    size_t n = strlen(name);
    return n > 5 && strcmp(name + n - 5, ".alda") == 0;
}

/* Collect the example filenames so the two tests below iterate the same set. */
static int list_examples(char names[][MAX_NAME], int max) {
    DIR *dir = opendir(ALDA_EXAMPLES_DIR);
    if (!dir) return -1;

    int n = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && n < max) {
        if (!has_alda_suffix(entry->d_name)) continue;

        /* Copy with an explicit bound: d_name is not a fixed-size array on
         * every platform, so snprintf here draws a truncation warning. */
        size_t len = strlen(entry->d_name);
        if (len >= MAX_NAME) continue;  /* Too long to be one of ours */
        memcpy(names[n], entry->d_name, len + 1);
        n++;
    }
    closedir(dir);
    return n;
}

TEST(examples_all_parse) {
    int total = list_examples(g_names, MAX_EXAMPLES);
    ASSERT_TRUE(total > 0);

    int failed = 0;
    for (int i = 0; i < total; i++) {
        char path[MAX_PATH_LEN];
        example_path(path, sizeof(path), g_names[i]);

        char *source = read_file(path);
        if (!source) {
            printf("    cannot read %s\n", g_names[i]);
            failed++;
            continue;
        }

        char *error = NULL;
        AldaNode *ast = alda_parse(source, g_names[i], &error);
        if (!ast || error) {
            failed++;
            printf("    %s: %s\n", g_names[i], error ? error : "(no AST)");
        }

        free(error);
        if (ast) alda_ast_free(ast);
        free(source);
    }

    printf("    (%d/%d examples parsed)\n", total - failed, total);
    ASSERT_EQ(failed, 0);
}

TEST(examples_all_interpret) {
    int total = list_examples(g_names, MAX_EXAMPLES);
    ASSERT_TRUE(total > 0);

    /* AldaContext is around 150KB - too large to sit on the stack. */
    AldaContext *ctx = malloc(sizeof(AldaContext));
    ASSERT_NOT_NULL(ctx);

    int failed = 0;
    int silent = 0;
    long total_events = 0;

    for (int i = 0; i < total; i++) {
        char path[MAX_PATH_LEN];
        example_path(path, sizeof(path), g_names[i]);

        alda_context_init(ctx);
        alda_set_no_sleep(ctx, 1);  /* Schedule events without playing them */

        if (alda_interpret_file(ctx, path) < 0) {
            failed++;
            printf("    %s: interpretation failed\n", g_names[i]);
        } else {
            total_events += ctx->event_count;
            if (ctx->event_count == 0 && !may_be_silent(g_names[i])) {
                silent++;
                printf("    %s: parsed and interpreted but scheduled no events\n",
                       g_names[i]);
            }
        }

        alda_context_cleanup(ctx);
    }

    free(ctx);

    printf("    (%d/%d examples interpreted, %ld events scheduled)\n",
           total - failed, total, total_events);
    ASSERT_EQ(failed, 0);
    ASSERT_EQ(silent, 0);
}

BEGIN_TEST_SUITE("Alda Examples")
    RUN_TEST(examples_all_parse);
    RUN_TEST(examples_all_interpret);
END_TEST_SUITE()
