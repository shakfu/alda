/* theme.c - Theme switching command
 *
 * Provides :theme command to switch syntax highlighting themes.
 * Priority: TOML themes > Lua themes > C built-ins.
 * Only available when built with linenoise (WITH_LINENOISE).
 */

#include "command_impl.h"
#include "../syntax.h"
#include "../theme_toml.h"

#ifdef LOKI_USE_LINENOISE
#include <syntax/theme.h>
#include "lua.h"
#include "lauxlib.h"
#endif

#ifdef LOKI_USE_LINENOISE
/* Try to load a Lua theme by calling theme.load(name).
 * Returns 1 if successful, 0 if theme not found or load failed. */
static int try_load_lua_theme(editor_ctx_t *ctx, const char *name) {
    lua_State *L = ctx->lua_host ? ctx->lua_host->L : NULL;
    if (!L) return 0;

    /* Get the 'theme' global table */
    lua_getglobal(L, "theme");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }

    /* Get theme.load function */
    lua_getfield(L, -1, "load");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return 0;
    }

    /* Call theme.load(name) */
    lua_pushstring(L, name);
    if (lua_pcall(L, 1, 2, 0) != LUA_OK) {
        /* Lua error */
        lua_pop(L, 2);  /* Pop error message and theme table */
        return 0;
    }

    /* theme.load returns: true on success, or nil + error_message on failure */
    int success = lua_toboolean(L, -2);
    lua_pop(L, 3);  /* Pop result, error/nil, and theme table */

    return success;
}
#endif

/* Helper to apply theme colors to all buffers */
static void apply_to_all_buffers(editor_ctx_t *ctx) {
    int buffer_ids[MAX_BUFFERS];
    int count = buffer_get_list(buffer_ids);
    for (int i = 0; i < count; i++) {
        editor_ctx_t *buf_ctx = buffer_get(buffer_ids[i]);
        if (buf_ctx && buf_ctx != ctx) {
            /* Copy colors from current ctx to other buffers */
            memcpy(buf_ctx->view.colors, ctx->view.colors,
                   sizeof(ctx->view.colors));
            buf_ctx->model.dirty++;
        }
    }
}

int cmd_theme(editor_ctx_t *ctx, const char *args) {
#ifdef LOKI_USE_LINENOISE
    /* No args: list available themes */
    if (!args || !args[0]) {
        const char **names = theme_list();

        /* Build theme list string */
        char buf[512] = "Themes: ";
        size_t pos = strlen(buf);

        for (int i = 0; names[i]; i++) {
            int len = snprintf(buf + pos, sizeof(buf) - pos, "%s%s",
                               i > 0 ? ", " : "",
                               names[i]);
            if (len > 0 && pos + len < sizeof(buf)) {
                pos += len;
            }
        }

        /* Add note about TOML/Lua themes */
        int note_len = snprintf(buf + pos, sizeof(buf) - pos,
                                " (+ .psnd/themes/)");
        if (note_len > 0 && pos + note_len < sizeof(buf)) {
            pos += note_len;
        }

        editor_set_status_msg(ctx, "%s", buf);
        return 1;
    }

    /* First, try loading as a TOML theme (preferred) */
    if (theme_toml_load(ctx, args)) {
        apply_to_all_buffers(ctx);
        editor_set_status_msg(ctx, "Theme set to: %s (TOML)", args);
        return 1;
    }

    /* Second, try loading as a Lua theme */
    if (try_load_lua_theme(ctx, args)) {
        apply_to_all_buffers(ctx);
        editor_set_status_msg(ctx, "Theme set to: %s (Lua)", args);
        return 1;
    }

    /* Fall back to C built-in theme */
    const syntax_theme_t *theme = theme_find(args);
    if (!theme) {
        editor_set_status_msg(ctx, "Unknown theme: %s", args);
        return 0;
    }

    theme_set(theme);

    /* Apply theme colors to all open buffers */
    int buffer_ids[MAX_BUFFERS];
    int count = buffer_get_list(buffer_ids);
    for (int i = 0; i < count; i++) {
        editor_ctx_t *buf_ctx = buffer_get(buffer_ids[i]);
        if (buf_ctx) {
            syntax_apply_theme_colors(buf_ctx);
            buf_ctx->model.dirty++;
        }
    }

    editor_set_status_msg(ctx, "Theme set to: %s", theme->name);
    return 1;
#else
    (void)args;
    editor_set_status_msg(ctx, "Theme support requires linenoise (build with -DWITH_LINENOISE=ON)");
    return 0;
#endif
}
