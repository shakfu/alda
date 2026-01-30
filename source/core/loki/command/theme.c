/* theme.c - Theme switching command
 *
 * Provides :theme command to switch syntax highlighting themes.
 * Only available when built with linenoise (WITH_LINENOISE).
 */

#include "command_impl.h"
#include "../syntax.h"

#ifdef LOKI_USE_LINENOISE
#include <syntax/theme.h>
#endif

int cmd_theme(editor_ctx_t *ctx, const char *args) {
#ifdef LOKI_USE_LINENOISE
    /* No args: list available themes */
    if (!args || !args[0]) {
        const char **names = theme_list();
        const syntax_theme_t *current = theme_get();

        editor_set_status_msg(ctx, "Themes: ");

        /* Build theme list string */
        char buf[512] = "Themes: ";
        size_t pos = strlen(buf);

        for (int i = 0; names[i]; i++) {
            int is_current = (current && strcmp(current->name, names[i]) == 0);
            int len = snprintf(buf + pos, sizeof(buf) - pos, "%s%s%s%s",
                               i > 0 ? ", " : "",
                               is_current ? "[" : "",
                               names[i],
                               is_current ? "]" : "");
            if (len > 0 && pos + len < sizeof(buf)) {
                pos += len;
            }
        }

        editor_set_status_msg(ctx, "%s", buf);
        return 1;
    }

    /* Find and set the theme */
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
