/**
 * @file repl_linenoise.c
 * @brief Linenoise integration layer for REPL with tree-sitter syntax highlighting.
 *
 * Lua-only version for psnd.
 */

#include "repl_linenoise.h"

#ifdef LOKI_USE_LINENOISE

#include <syntax/lua.h>
#include <stdlib.h>
#include <string.h>

/* Global linenoise context */
static linenoise_context_t *g_ln_ctx = NULL;
static ReplLanguage g_current_lang = REPL_LANG_NONE;

/* Initialize highlighters for the current language */
static int init_highlighter(ReplLanguage lang) {
    switch (lang) {
        case REPL_LANG_LUA:
            return lua_highlight_init();
        case REPL_LANG_NONE:
        default:
            return 0;
    }
}

/* Free highlighters for the current language */
static void free_highlighter(ReplLanguage lang) {
    switch (lang) {
        case REPL_LANG_LUA:
            lua_highlight_free();
            break;
        case REPL_LANG_NONE:
        default:
            break;
    }
}

/* Get the highlight callback for the current language */
static linenoise_highlight_cb_t *get_highlight_callback(ReplLanguage lang) {
    switch (lang) {
        case REPL_LANG_LUA:
            return lua_highlight_callback;
        case REPL_LANG_NONE:
        default:
            return NULL;
    }
}

int repl_linenoise_init(ReplLanguage lang) {
    /* Clean up any existing context */
    if (g_ln_ctx) {
        repl_linenoise_cleanup();
    }

    /* Create new linenoise context */
    g_ln_ctx = linenoise_context_create();
    if (!g_ln_ctx) {
        return -1;
    }

    /* Initialize highlighter for the language */
    if (init_highlighter(lang) != 0) {
        linenoise_context_destroy(g_ln_ctx);
        g_ln_ctx = NULL;
        return -1;
    }

    /* Set highlight callback */
    linenoise_highlight_cb_t *cb = get_highlight_callback(lang);
    if (cb) {
        linenoise_set_highlight_callback(g_ln_ctx, cb);
    }

    g_current_lang = lang;
    return 0;
}

void repl_linenoise_cleanup(void) {
    if (g_ln_ctx) {
        free_highlighter(g_current_lang);
        linenoise_context_destroy(g_ln_ctx);
        g_ln_ctx = NULL;
    }
    g_current_lang = REPL_LANG_NONE;
}

linenoise_context_t *repl_linenoise_get_context(void) {
    return g_ln_ctx;
}

void repl_linenoise_set_language(ReplLanguage lang) {
    if (lang == g_current_lang) {
        return;
    }

    /* Free old highlighter */
    free_highlighter(g_current_lang);

    /* Initialize new highlighter */
    if (init_highlighter(lang) == 0 && g_ln_ctx) {
        linenoise_highlight_cb_t *cb = get_highlight_callback(lang);
        linenoise_set_highlight_callback(g_ln_ctx, cb);
    }

    g_current_lang = lang;
}

ReplLanguage repl_linenoise_get_language(void) {
    return g_current_lang;
}

#endif /* LOKI_USE_LINENOISE */
