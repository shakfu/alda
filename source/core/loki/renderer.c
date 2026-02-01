/* renderer.c - Renderer interface implementation
 *
 * This file contains:
 * - Terminal renderer (VT100 escape sequences)
 * - Null renderer (for testing)
 * - Helper functions
 */

#include "renderer.h"
#include "internal.h"
#include "terminal.h"
#include "selection.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* ======================= Helper Functions ================================= */

HighlightType hl_const_to_type(int hl_const) {
    switch (hl_const) {
        case HL_NORMAL:   return HL_TYPE_NORMAL;
        case HL_COMMENT:  return HL_TYPE_COMMENT;
        case HL_KEYWORD1: return HL_TYPE_KEYWORD1;
        case HL_KEYWORD2: return HL_TYPE_KEYWORD2;
        case HL_STRING:   return HL_TYPE_STRING;
        case HL_NUMBER:   return HL_TYPE_NUMBER;
        case HL_MATCH:    return HL_TYPE_MATCH;
        case HL_NONPRINT: return HL_TYPE_NONPRINT;
        default:          return HL_TYPE_NORMAL;
    }
}

/* ======================= Terminal Renderer ================================ */

typedef struct {
    struct abuf ab;     /* Output buffer */
    int cols;           /* Screen columns */
    int rows;           /* Screen rows */
} TerminalRendererData;

static void terminal_begin_frame(Renderer *r, int cols, int rows) {
    TerminalRendererData *data = (TerminalRendererData *)r->data;
    data->cols = cols;
    data->rows = rows;

    /* Reset buffer */
    data->ab.len = 0;

    /* Hide cursor and go home */
    terminal_buffer_append(&data->ab, "\x1b[?25l", 6);  /* Hide cursor */
    terminal_buffer_append(&data->ab, "\x1b[H", 3);     /* Go home */
}

static void terminal_end_frame(Renderer *r) {
    TerminalRendererData *data = (TerminalRendererData *)r->data;

    /* Show cursor */
    terminal_buffer_append(&data->ab, "\x1b[?25h", 6);

    /* Flush buffer to terminal */
    write(STDOUT_FILENO, data->ab.b, data->ab.len);
}

static void terminal_render_tabs(Renderer *r, const char **tabs, int tab_count,
                                 int active_tab, int width) {
    TerminalRendererData *data = (TerminalRendererData *)r->data;
    struct abuf *ab = &data->ab;

    if (tab_count <= 1) return;

    terminal_buffer_append(ab, "\x1b[7m", 4);  /* Reverse video */

    int col = 0;
    for (int i = 0; i < tab_count && col < width; i++) {
        const char *tab = tabs[i] ? tabs[i] : "???";
        int len = strlen(tab);
        if (len > 20) len = 20;

        if (i == active_tab) {
            terminal_buffer_append(ab, "\x1b[1m", 4);  /* Bold */
        }

        terminal_buffer_append(ab, " ", 1);
        col++;

        int take = len;
        if (col + take > width - 1) take = width - col - 1;
        if (take > 0) {
            terminal_buffer_append(ab, tab, take);
            col += take;
        }

        terminal_buffer_append(ab, " ", 1);
        col++;

        if (i == active_tab) {
            terminal_buffer_append(ab, "\x1b[22m", 5);  /* Normal intensity */
        }

        if (i < tab_count - 1 && col < width) {
            terminal_buffer_append(ab, "|", 1);
            col++;
        }
    }

    /* Pad rest of line */
    while (col < width) {
        terminal_buffer_append(ab, " ", 1);
        col++;
    }

    terminal_buffer_append(ab, "\x1b[0m", 4);  /* Reset */
    terminal_buffer_append(ab, "\r\n", 2);
}

static void terminal_render_row(Renderer *r, int row_num,
                                const RenderSegment *segments, int seg_count,
                                int gutter_width, int is_empty) {
    TerminalRendererData *data = (TerminalRendererData *)r->data;
    struct abuf *ab = &data->ab;

    /* Render gutter (line number) */
    if (gutter_width > 0) {
        terminal_buffer_append(ab, "\x1b[90m", 5);  /* Dark gray */
        if (is_empty) {
            /* Empty row: show tilde */
            for (int i = 0; i < gutter_width - 1; i++)
                terminal_buffer_append(ab, " ", 1);
            terminal_buffer_append(ab, "~", 1);
        } else {
            char line_num_buf[16];
            int line_num_len = snprintf(line_num_buf, sizeof(line_num_buf),
                "%*d ", gutter_width - 1, row_num);
            terminal_buffer_append(ab, line_num_buf, line_num_len);
        }
        terminal_buffer_append(ab, "\x1b[39m", 5);  /* Reset foreground */
    } else if (is_empty) {
        terminal_buffer_append(ab, "~", 1);
    }

    /* Render segments */
    HighlightType current_type = HL_TYPE_NORMAL;
    int in_selection = 0;

    for (int i = 0; i < seg_count; i++) {
        const RenderSegment *seg = &segments[i];

        /* Handle selection */
        if (seg->selected && !in_selection) {
            terminal_buffer_append(ab, "\x1b[7m", 4);  /* Reverse video */
            in_selection = 1;
        } else if (!seg->selected && in_selection) {
            terminal_buffer_append(ab, "\x1b[27m", 5);  /* Normal video */
            in_selection = 0;
        }

        /* Handle highlight type change */
        if (seg->hl_type != current_type) {
            if (seg->hl_type == HL_TYPE_NONPRINT) {
                if (!in_selection) {
                    terminal_buffer_append(ab, "\x1b[7m", 4);
                }
            } else if (seg->hl_type == HL_TYPE_NORMAL) {
                terminal_buffer_append(ab, "\x1b[39m", 5);
            } else {
                /* Use a simple color mapping */
                const char *color_code = "\x1b[39m";
                switch (seg->hl_type) {
                    case HL_TYPE_COMMENT:  color_code = "\x1b[90m"; break;  /* Gray */
                    case HL_TYPE_KEYWORD1: color_code = "\x1b[33m"; break;  /* Yellow */
                    case HL_TYPE_KEYWORD2: color_code = "\x1b[32m"; break;  /* Green */
                    case HL_TYPE_STRING:   color_code = "\x1b[36m"; break;  /* Cyan */
                    case HL_TYPE_NUMBER:   color_code = "\x1b[35m"; break;  /* Magenta */
                    case HL_TYPE_MATCH:    color_code = "\x1b[34m"; break;  /* Blue */
                    default: break;
                }
                terminal_buffer_append(ab, color_code, strlen(color_code));
            }
            current_type = seg->hl_type;
        }

        /* Render text */
        if (seg->hl_type == HL_TYPE_NONPRINT && seg->len == 1) {
            /* Non-printable: show as ^X or ? */
            char sym = (seg->text[0] <= 26) ? '@' + seg->text[0] : '?';
            terminal_buffer_append(ab, &sym, 1);
            terminal_buffer_append(ab, "\x1b[0m", 4);  /* Reset */
            current_type = HL_TYPE_NORMAL;
        } else {
            terminal_buffer_append(ab, seg->text, seg->len);
        }
    }

    /* Reset and end line */
    terminal_buffer_append(ab, "\x1b[39m", 5);  /* Reset foreground */
    if (in_selection) {
        terminal_buffer_append(ab, "\x1b[27m", 5);  /* Normal video */
    }
    terminal_buffer_append(ab, "\x1b[0K", 4);   /* Clear to end of line */
    terminal_buffer_append(ab, "\r\n", 2);
}

static void terminal_render_status(Renderer *r, const StatusInfo *info, int width) {
    TerminalRendererData *data = (TerminalRendererData *)r->data;
    struct abuf *ab = &data->ab;

    terminal_buffer_append(ab, "\x1b[0K", 4);   /* Clear line */
    terminal_buffer_append(ab, "\x1b[7m", 4);   /* Reverse video */

    char status[128], rstatus[128];

    /* Build left status */
    int len = snprintf(status, sizeof(status), " %s%s  %.20s - %d lines %s",
        info->lang ? info->lang : "",
        info->mode ? info->mode : "NORMAL",
        info->filename ? info->filename : "[No Name]",
        info->numrows,
        info->dirty ? "(modified)" : "");

    /* Build right status with playback info */
    char playback_info[64] = "";
    if (info->playing || info->metronome_active) {
        if (info->tempo > 0 && info->bar > 0) {
            /* Show bar.beat and tempo */
            snprintf(playback_info, sizeof(playback_info), "%d.%d %.0fBPM ",
                info->bar, info->beat_in_bar, info->tempo);
        } else if (info->tempo > 0) {
            /* Just tempo */
            snprintf(playback_info, sizeof(playback_info), "%.0fBPM ", info->tempo);
        }
    }

    const char *playing_tag = "";
    if (info->playing && info->metronome_active) {
        playing_tag = "[PLAY+MET] ";
    } else if (info->playing) {
        playing_tag = "[PLAYING] ";
    } else if (info->metronome_active) {
        playing_tag = "[METRONOME] ";
    }

    /* Link peers indicator */
    char peers_info[16] = "";
    if (info->link_active && info->link_peers > 0) {
        snprintf(peers_info, sizeof(peers_info), "[%dP] ", info->link_peers);
    }

    int rlen = snprintf(rstatus, sizeof(rstatus), "%s%s%s%d/%d",
        peers_info, playback_info, playing_tag,
        info->current_row, info->numrows);

    if (len > width) len = width;
    terminal_buffer_append(ab, status, len);

    /* Pad and right-align */
    while (len < width) {
        if (width - len == rlen) {
            terminal_buffer_append(ab, rstatus, rlen);
            break;
        } else {
            terminal_buffer_append(ab, " ", 1);
            len++;
        }
    }

    terminal_buffer_append(ab, "\x1b[0m\r\n", 6);  /* Reset and newline */
}

static void terminal_render_message(Renderer *r, const char *message, int width) {
    TerminalRendererData *data = (TerminalRendererData *)r->data;
    struct abuf *ab = &data->ab;

    terminal_buffer_append(ab, "\x1b[0K", 4);  /* Clear line */

    if (message && *message) {
        int msglen = strlen(message);
        if (msglen > width) msglen = width;
        terminal_buffer_append(ab, message, msglen);
    }
}

static void terminal_render_repl(Renderer *r, const ReplInfo *info, int width) {
    TerminalRendererData *data = (TerminalRendererData *)r->data;
    struct abuf *ab = &data->ab;

    terminal_buffer_append(ab, "\r\n", 2);

    /* Render log lines */
    int start = info->log_count - info->max_display_lines;
    if (start < 0) start = 0;
    int rendered = 0;

    for (int i = start; i < info->log_count; i++) {
        const char *line = info->log_lines[i] ? info->log_lines[i] : "";
        int take = strlen(line);
        if (take > width) take = width;
        terminal_buffer_append(ab, "\x1b[0K", 4);
        if (take > 0) terminal_buffer_append(ab, line, take);
        terminal_buffer_append(ab, "\r\n", 2);
        rendered++;
    }

    /* Pad remaining lines */
    while (rendered < info->max_display_lines) {
        terminal_buffer_append(ab, "\x1b[0K\r\n", 6);
        rendered++;
    }

    /* Render prompt and input */
    terminal_buffer_append(ab, "\x1b[0K", 4);
    if (info->prompt) {
        terminal_buffer_append(ab, info->prompt, strlen(info->prompt));
    }

    int prompt_len = info->prompt ? strlen(info->prompt) : 0;
    int available = width - prompt_len;
    if (available < 0) available = 0;
    if (available > 0 && info->input_len > 0) {
        int shown = info->input_len;
        if (shown > available) shown = available;
        terminal_buffer_append(ab, info->input, shown);
    }
}

static void terminal_render_picker(Renderer *r, const PickerInfo *info, int width, int height) {
    TerminalRendererData *data = (TerminalRendererData *)r->data;
    struct abuf *ab = &data->ab;

    /* Clear screen and go home */
    terminal_buffer_append(ab, "\x1b[H", 3);
    terminal_buffer_append(ab, "\x1b[2J", 4);

    /* Render title bar (reverse video) */
    terminal_buffer_append(ab, "\x1b[7m", 4);
    if (info->title) {
        int title_len = strlen(info->title);
        int padding = (width - title_len) / 2;
        for (int i = 0; i < padding; i++) {
            terminal_buffer_append(ab, " ", 1);
        }
        if (title_len > width) title_len = width;
        terminal_buffer_append(ab, info->title, title_len);
        for (int i = padding + title_len; i < width; i++) {
            terminal_buffer_append(ab, " ", 1);
        }
    } else {
        for (int i = 0; i < width; i++) {
            terminal_buffer_append(ab, " ", 1);
        }
    }
    terminal_buffer_append(ab, "\x1b[0m", 4);
    terminal_buffer_append(ab, "\r\n", 2);

    /* Render items */
    int visible = info->visible_rows;
    if (visible > height - 3) visible = height - 3;
    if (visible < 1) visible = 1;

    for (int i = 0; i < visible; i++) {
        int item_idx = info->scroll_offset + i;
        int is_selected = (item_idx == info->selected_index);

        /* Clear line */
        terminal_buffer_append(ab, "\x1b[0K", 4);

        if (item_idx < info->item_count) {
            const char *item = info->items[item_idx];
            int item_len = item ? strlen(item) : 0;

            /* Selection indicator */
            if (is_selected) {
                terminal_buffer_append(ab, "\x1b[7m", 4);  /* Reverse video */
                terminal_buffer_append(ab, "> ", 2);
            } else {
                terminal_buffer_append(ab, "  ", 2);
            }

            /* Item text (truncate if needed) */
            int max_item_width = width - 3;
            if (item_len > max_item_width) item_len = max_item_width;
            if (item && item_len > 0) {
                terminal_buffer_append(ab, item, item_len);
            }

            /* Pad to full width for reverse video */
            if (is_selected) {
                for (int j = item_len + 2; j < width; j++) {
                    terminal_buffer_append(ab, " ", 1);
                }
                terminal_buffer_append(ab, "\x1b[0m", 4);  /* Reset */
            }
        }

        terminal_buffer_append(ab, "\r\n", 2);
    }

    /* Fill remaining rows with blank lines */
    int rows_used = 1 + visible;  /* Title + items */
    for (int i = rows_used; i < height - 2; i++) {
        terminal_buffer_append(ab, "\x1b[0K\r\n", 6);
    }

    /* Render status bar */
    terminal_buffer_append(ab, "\x1b[7m", 4);
    char status[80];
    int status_len = snprintf(status, sizeof(status),
        " %d/%d | j/k:move ENTER:select ESC:cancel",
        info->selected_index + 1, info->item_count);
    if (status_len > width) status_len = width;
    terminal_buffer_append(ab, status, status_len);
    for (int i = status_len; i < width; i++) {
        terminal_buffer_append(ab, " ", 1);
    }
    terminal_buffer_append(ab, "\x1b[0m", 4);
}

static void terminal_set_cursor(Renderer *r, int row, int col) {
    TerminalRendererData *data = (TerminalRendererData *)r->data;
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
    terminal_buffer_append(&data->ab, buf, len);
}

static void terminal_show_cursor(Renderer *r) {
    TerminalRendererData *data = (TerminalRendererData *)r->data;
    terminal_buffer_append(&data->ab, "\x1b[?25h", 6);
}

static void terminal_hide_cursor(Renderer *r) {
    TerminalRendererData *data = (TerminalRendererData *)r->data;
    terminal_buffer_append(&data->ab, "\x1b[?25l", 6);
}

static int terminal_clipboard_copy(Renderer *r, const char *text, size_t len) {
    (void)r;  /* Terminal renderer uses stdout directly */

    /* Base64 encode */
    char *encoded = base64_encode(text, len);
    if (!encoded) return -1;

    /* Send OSC 52 sequence: ESC]52;c;<base64>BEL */
    printf("\033]52;c;%s\007", encoded);
    fflush(stdout);
    free(encoded);

    return 0;
}

static void terminal_destroy(Renderer *r) {
    if (r) {
        TerminalRendererData *data = (TerminalRendererData *)r->data;
        if (data) {
            terminal_buffer_free(&data->ab);
            free(data);
        }
        free(r);
    }
}

Renderer *terminal_renderer_create(void) {
    Renderer *r = calloc(1, sizeof(Renderer));
    if (!r) return NULL;

    TerminalRendererData *data = calloc(1, sizeof(TerminalRendererData));
    if (!data) {
        free(r);
        return NULL;
    }

    data->ab = (struct abuf)ABUF_INIT;

    r->data = data;
    r->begin_frame = terminal_begin_frame;
    r->end_frame = terminal_end_frame;
    r->render_tabs = terminal_render_tabs;
    r->render_row = terminal_render_row;
    r->render_status = terminal_render_status;
    r->render_message = terminal_render_message;
    r->render_repl = terminal_render_repl;
    r->render_picker = terminal_render_picker;
    r->set_cursor = terminal_set_cursor;
    r->show_cursor = terminal_show_cursor;
    r->hide_cursor = terminal_hide_cursor;
    r->clipboard_copy = terminal_clipboard_copy;
    r->destroy = terminal_destroy;

    return r;
}

/* ======================= Null Renderer ==================================== */

static void null_begin_frame(Renderer *r, int cols, int rows) {
    (void)r; (void)cols; (void)rows;
}

static void null_end_frame(Renderer *r) {
    (void)r;
}

static void null_render_tabs(Renderer *r, const char **tabs, int tab_count,
                             int active_tab, int width) {
    (void)r; (void)tabs; (void)tab_count; (void)active_tab; (void)width;
}

static void null_render_row(Renderer *r, int row_num,
                            const RenderSegment *segments, int seg_count,
                            int gutter_width, int is_empty) {
    (void)r; (void)row_num; (void)segments; (void)seg_count;
    (void)gutter_width; (void)is_empty;
}

static void null_render_status(Renderer *r, const StatusInfo *info, int width) {
    (void)r; (void)info; (void)width;
}

static void null_render_message(Renderer *r, const char *message, int width) {
    (void)r; (void)message; (void)width;
}

static void null_render_repl(Renderer *r, const ReplInfo *info, int width) {
    (void)r; (void)info; (void)width;
}

static void null_render_picker(Renderer *r, const PickerInfo *info, int width, int height) {
    (void)r; (void)info; (void)width; (void)height;
}

static void null_set_cursor(Renderer *r, int row, int col) {
    (void)r; (void)row; (void)col;
}

static void null_show_cursor(Renderer *r) {
    (void)r;
}

static void null_hide_cursor(Renderer *r) {
    (void)r;
}

static int null_clipboard_copy(Renderer *r, const char *text, size_t len) {
    (void)r; (void)text; (void)len;
    return 0;  /* Silently succeed */
}

static void null_destroy(Renderer *r) {
    if (r) free(r);
}

Renderer *null_renderer_create(void) {
    Renderer *r = calloc(1, sizeof(Renderer));
    if (!r) return NULL;

    r->data = NULL;
    r->begin_frame = null_begin_frame;
    r->end_frame = null_end_frame;
    r->render_tabs = null_render_tabs;
    r->render_row = null_render_row;
    r->render_status = null_render_status;
    r->render_message = null_render_message;
    r->render_repl = null_render_repl;
    r->render_picker = null_render_picker;
    r->set_cursor = null_set_cursor;
    r->show_cursor = null_show_cursor;
    r->hide_cursor = null_hide_cursor;
    r->clipboard_copy = null_clipboard_copy;
    r->destroy = null_destroy;

    return r;
}
