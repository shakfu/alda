/* test_repl_line_editor.c - Unit tests for REPL line editor helpers
 *
 * Focus is repl_extract_completion_prefix(), the shared word-extraction step
 * used by the tab-completion paths.
 *
 * Regression context: the linenoise completion adapter previously copied the
 * word into a REPL_MAX_INPUT_LENGTH (1024) stack buffer while linenoise hands
 * the callback a LINENOISE_MAX_LINE (4096) buffer. The length guard covered the
 * memcpy but not the NUL terminator, so a word of 1024 or more characters wrote
 * up to ~3KB past the buffer. The tests below pin the boundary.
 */

#include "test_framework.h"
#include "loki/repl.h"
#include <string.h>
#include <stdlib.h>

/* Guard bytes let us assert the helper never writes past out_size. */
#define GUARD_LEN 64
#define GUARD_BYTE 0xAB

typedef struct {
    char *block;      /* out_size bytes + GUARD_LEN trailing guard */
    char *out;
    size_t out_size;
} GuardedBuf;

static void guarded_init(GuardedBuf *g, size_t out_size) {
    g->out_size = out_size;
    g->block = malloc(out_size + GUARD_LEN);
    memset(g->block, GUARD_BYTE, out_size + GUARD_LEN);
    g->out = g->block;
}

static void guarded_free(GuardedBuf *g) {
    free(g->block);
    g->block = NULL;
    g->out = NULL;
}

/* Returns 1 if the trailing guard region is intact. */
static int guard_intact(const GuardedBuf *g) {
    for (size_t i = 0; i < GUARD_LEN; i++) {
        if ((unsigned char)g->block[g->out_size + i] != GUARD_BYTE) return 0;
    }
    return 1;
}

/* Build a string of `n` copies of `c`. Caller frees. */
static char *make_word(size_t n, char c) {
    char *s = malloc(n + 1);
    memset(s, c, n);
    s[n] = '\0';
    return s;
}

/* ========================================================================== */
/* Basic extraction                                                            */
/* ========================================================================== */

TEST(extract_simple_word) {
    char out[64];
    int start = -1;
    int len = repl_extract_completion_prefix("hello", 5, out, sizeof(out), &start);
    ASSERT_EQ(len, 5);
    ASSERT_STR_EQ("hello", out);
    ASSERT_EQ(start, 0);
}

TEST(extract_last_word_only) {
    char out[64];
    int start = -1;
    int len = repl_extract_completion_prefix("foo bar baz", 11, out, sizeof(out), &start);
    ASSERT_EQ(len, 3);
    ASSERT_STR_EQ("baz", out);
    ASSERT_EQ(start, 8);
}

TEST(extract_stops_at_tab_and_newline) {
    char out[64];
    int start = -1;
    int len = repl_extract_completion_prefix("foo\tbar", 7, out, sizeof(out), &start);
    ASSERT_EQ(len, 3);
    ASSERT_STR_EQ("bar", out);
    ASSERT_EQ(start, 4);

    len = repl_extract_completion_prefix("foo\nqux", 7, out, sizeof(out), &start);
    ASSERT_EQ(len, 3);
    ASSERT_STR_EQ("qux", out);
    ASSERT_EQ(start, 4);
}

TEST(extract_empty_word_after_space) {
    char out[64];
    int start = -1;
    int len = repl_extract_completion_prefix("foo ", 4, out, sizeof(out), &start);
    ASSERT_EQ(len, 0);
    ASSERT_STR_EQ("", out);
    ASSERT_EQ(start, 4);
}

TEST(extract_empty_buffer) {
    char out[64];
    int start = -1;
    int len = repl_extract_completion_prefix("", 0, out, sizeof(out), &start);
    ASSERT_EQ(len, 0);
    ASSERT_STR_EQ("", out);
    ASSERT_EQ(start, 0);
}

TEST(extract_honors_pos_before_end_of_buffer) {
    /* Cursor mid-line: only the word up to pos is considered. */
    char out[64];
    int start = -1;
    int len = repl_extract_completion_prefix("abcdef ghi", 4, out, sizeof(out), &start);
    ASSERT_EQ(len, 4);
    ASSERT_STR_EQ("abcd", out);
    ASSERT_EQ(start, 0);
}

TEST(extract_word_start_optional) {
    char out[64];
    int len = repl_extract_completion_prefix("foo bar", 7, out, sizeof(out), NULL);
    ASSERT_EQ(len, 3);
    ASSERT_STR_EQ("bar", out);
}

/* ========================================================================== */
/* Boundary conditions - the overflow regression                               */
/* ========================================================================== */

TEST(extract_word_exactly_fills_buffer_minus_terminator) {
    /* out_size - 1 characters plus the NUL is the largest word that fits. */
    GuardedBuf g;
    guarded_init(&g, 32);
    char *word = make_word(31, 'x');

    int start = -1;
    int len = repl_extract_completion_prefix(word, 31, g.out, g.out_size, &start);

    ASSERT_EQ(len, 31);
    ASSERT_EQ(start, 0);
    ASSERT_EQ((int)g.out[31], 0);
    ASSERT_TRUE(guard_intact(&g));

    free(word);
    guarded_free(&g);
}

TEST(extract_word_exactly_out_size_is_rejected) {
    /* The regression: word_len == out_size passed the old memcpy guard but the
     * terminator still landed at out[out_size]. Must be rejected outright. */
    GuardedBuf g;
    guarded_init(&g, 32);
    char *word = make_word(32, 'x');

    int len = repl_extract_completion_prefix(word, 32, g.out, g.out_size, NULL);

    ASSERT_EQ(len, -1);
    ASSERT_TRUE(guard_intact(&g));

    free(word);
    guarded_free(&g);
}

TEST(extract_word_far_larger_than_buffer_is_rejected) {
    /* Mirrors the real case: a LINENOISE_MAX_LINE-sized word against a
     * REPL_MAX_INPUT_LENGTH buffer. */
    GuardedBuf g;
    guarded_init(&g, REPL_MAX_INPUT_LENGTH);
    char *word = make_word(4095, 'x');

    int len = repl_extract_completion_prefix(word, 4095, g.out,
                                             g.out_size, NULL);

    ASSERT_EQ(len, -1);
    ASSERT_TRUE(guard_intact(&g));

    free(word);
    guarded_free(&g);
}

TEST(extract_long_word_rejected_even_with_leading_space) {
    /* start > 0 must not change the verdict; the word itself is what must fit. */
    GuardedBuf g;
    guarded_init(&g, REPL_MAX_INPUT_LENGTH);

    size_t wordlen = 2000;
    char *line = malloc(wordlen + 8);
    memcpy(line, "cmd ", 4);
    memset(line + 4, 'y', wordlen);
    line[4 + wordlen] = '\0';

    int len = repl_extract_completion_prefix(line, (int)(4 + wordlen), g.out,
                                             g.out_size, NULL);

    ASSERT_EQ(len, -1);
    ASSERT_TRUE(guard_intact(&g));

    free(line);
    guarded_free(&g);
}

TEST(extract_long_line_short_trailing_word_succeeds) {
    /* A long line is fine so long as the trailing word fits. */
    GuardedBuf g;
    guarded_init(&g, REPL_MAX_INPUT_LENGTH);

    size_t filler = 3000;
    char *line = malloc(filler + 8);
    memset(line, 'z', filler);
    memcpy(line + filler, " ab", 3);
    line[filler + 3] = '\0';

    int start = -1;
    int len = repl_extract_completion_prefix(line, (int)(filler + 3), g.out,
                                             g.out_size, &start);

    ASSERT_EQ(len, 2);
    ASSERT_STR_EQ("ab", g.out);
    ASSERT_EQ(start, (int)filler + 1);
    ASSERT_TRUE(guard_intact(&g));

    free(line);
    guarded_free(&g);
}

TEST(extract_single_byte_buffer_only_fits_empty_word) {
    GuardedBuf g;
    guarded_init(&g, 1);

    /* Empty trailing word fits: just the terminator. */
    int len = repl_extract_completion_prefix("a ", 2, g.out, g.out_size, NULL);
    ASSERT_EQ(len, 0);
    ASSERT_EQ((int)g.out[0], 0);
    ASSERT_TRUE(guard_intact(&g));

    /* One character does not. */
    len = repl_extract_completion_prefix("ab", 2, g.out, g.out_size, NULL);
    ASSERT_EQ(len, -1);
    ASSERT_TRUE(guard_intact(&g));

    guarded_free(&g);
}

/* ========================================================================== */
/* Invalid arguments                                                           */
/* ========================================================================== */

TEST(extract_rejects_null_buffer) {
    char out[64];
    ASSERT_EQ(repl_extract_completion_prefix(NULL, 0, out, sizeof(out), NULL), -1);
}

TEST(extract_rejects_null_out) {
    ASSERT_EQ(repl_extract_completion_prefix("foo", 3, NULL, 64, NULL), -1);
}

TEST(extract_rejects_zero_out_size) {
    char out[64];
    ASSERT_EQ(repl_extract_completion_prefix("foo", 3, out, 0, NULL), -1);
}

TEST(extract_rejects_negative_pos) {
    char out[64];
    ASSERT_EQ(repl_extract_completion_prefix("foo", -1, out, sizeof(out), NULL), -1);
}

/* ========================================================================== */

BEGIN_TEST_SUITE("REPL Line Editor")
    RUN_TEST(extract_simple_word);
    RUN_TEST(extract_last_word_only);
    RUN_TEST(extract_stops_at_tab_and_newline);
    RUN_TEST(extract_empty_word_after_space);
    RUN_TEST(extract_empty_buffer);
    RUN_TEST(extract_honors_pos_before_end_of_buffer);
    RUN_TEST(extract_word_start_optional);

    /* Boundary conditions - the overflow regression */
    RUN_TEST(extract_word_exactly_fills_buffer_minus_terminator);
    RUN_TEST(extract_word_exactly_out_size_is_rejected);
    RUN_TEST(extract_word_far_larger_than_buffer_is_rejected);
    RUN_TEST(extract_long_word_rejected_even_with_leading_space);
    RUN_TEST(extract_long_line_short_trailing_word_succeeds);
    RUN_TEST(extract_single_byte_buffer_only_fits_empty_word);

    /* Invalid arguments */
    RUN_TEST(extract_rejects_null_buffer);
    RUN_TEST(extract_rejects_null_out);
    RUN_TEST(extract_rejects_zero_out_size);
    RUN_TEST(extract_rejects_negative_pos);
END_TEST_SUITE()
