/**
 * test_view_theme.c - Tests for tracker theme system
 */

#include "test_framework.h"
#include "tracker_view.h"
#include <stdlib.h>
#include <string.h>

/* Global test stats required by test framework */
test_stats_t test_stats;

/*============================================================================
 * Theme Get Tests
 *============================================================================*/

TEST(theme_get_default) {
    const TrackerTheme* theme = tracker_theme_get("default");

    ASSERT_NOT_NULL(theme);
    ASSERT_STR_EQ(theme->name, "default");
    ASSERT_STR_EQ(theme->author, "psnd");
}

TEST(theme_get_retro) {
    const TrackerTheme* theme = tracker_theme_get("retro");

    ASSERT_NOT_NULL(theme);
    ASSERT_STR_EQ(theme->name, "retro");
    ASSERT_STR_EQ(theme->author, "psnd");
}

TEST(theme_get_null_returns_default) {
    const TrackerTheme* theme = tracker_theme_get(NULL);

    ASSERT_NOT_NULL(theme);
    ASSERT_STR_EQ(theme->name, "default");
}

TEST(theme_get_unknown_returns_null) {
    const TrackerTheme* theme = tracker_theme_get("nonexistent");

    ASSERT_NULL(theme);
}

TEST(theme_get_empty_string_returns_null) {
    const TrackerTheme* theme = tracker_theme_get("");

    ASSERT_NULL(theme);
}

TEST(theme_get_case_sensitive) {
    /* Theme names are case-sensitive */
    ASSERT_NULL(tracker_theme_get("Default"));
    ASSERT_NULL(tracker_theme_get("DEFAULT"));
    ASSERT_NULL(tracker_theme_get("Retro"));
    ASSERT_NULL(tracker_theme_get("RETRO"));
}

/*============================================================================
 * Theme List Tests
 *============================================================================*/

TEST(theme_list_returns_names) {
    int count = 0;
    const char** names = tracker_theme_list(&count);

    ASSERT_NOT_NULL(names);
    ASSERT_GTE(count, 2);  /* At least default and retro */
}

TEST(theme_list_contains_default) {
    int count = 0;
    const char** names = tracker_theme_list(&count);

    bool found = false;
    for (int i = 0; i < count; i++) {
        if (strcmp(names[i], "default") == 0) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
}

TEST(theme_list_contains_retro) {
    int count = 0;
    const char** names = tracker_theme_list(&count);

    bool found = false;
    for (int i = 0; i < count; i++) {
        if (strcmp(names[i], "retro") == 0) {
            found = true;
            break;
        }
    }
    ASSERT_TRUE(found);
}

TEST(theme_list_null_count) {
    /* Should not crash when count is NULL */
    const char** names = tracker_theme_list(NULL);
    ASSERT_NOT_NULL(names);
}

TEST(theme_list_all_valid) {
    int count = 0;
    const char** names = tracker_theme_list(&count);

    /* Each name should retrieve a valid theme */
    for (int i = 0; i < count; i++) {
        const TrackerTheme* theme = tracker_theme_get(names[i]);
        ASSERT_NOT_NULL(theme);
        ASSERT_STR_EQ(theme->name, names[i]);
    }
}

/*============================================================================
 * Theme Init Default Tests
 *============================================================================*/

TEST(theme_init_default_basic) {
    TrackerTheme theme;
    memset(&theme, 0, sizeof(theme));

    tracker_theme_init_default(&theme);

    ASSERT_NOT_NULL(theme.name);
    ASSERT_STR_EQ(theme.name, "default");
    ASSERT_NOT_NULL(theme.author);
    ASSERT_STR_EQ(theme.author, "psnd");
}

TEST(theme_init_default_null_safe) {
    /* Should not crash */
    tracker_theme_init_default(NULL);
}

TEST(theme_init_default_styles) {
    TrackerTheme theme;
    tracker_theme_init_default(&theme);

    /* Check some style properties */
    ASSERT_EQ(theme.default_style.fg.type, TRACKER_COLOR_INDEXED);
    ASSERT_EQ(theme.default_style.bg.type, TRACKER_COLOR_INDEXED);

    ASSERT_EQ(theme.cursor.fg.type, TRACKER_COLOR_INDEXED);
    ASSERT_EQ(theme.cursor.bg.type, TRACKER_COLOR_INDEXED);
}

TEST(theme_init_default_border_chars) {
    TrackerTheme theme;
    tracker_theme_init_default(&theme);

    ASSERT_NOT_NULL(theme.border_h);
    ASSERT_NOT_NULL(theme.border_v);
    ASSERT_NOT_NULL(theme.note_off_marker);
    ASSERT_NOT_NULL(theme.continuation_marker);
    ASSERT_NOT_NULL(theme.empty_cell);
}

TEST(theme_init_default_velocity_styles) {
    TrackerTheme theme;
    tracker_theme_init_default(&theme);

    /* 4 velocity levels */
    for (int i = 0; i < 4; i++) {
        ASSERT_EQ(theme.note_velocity[i].fg.type, TRACKER_COLOR_INDEXED);
    }
}

/*============================================================================
 * Theme Clone Tests
 *============================================================================*/

TEST(theme_clone_default) {
    const TrackerTheme* original = tracker_theme_get("default");
    TrackerTheme* clone = tracker_theme_clone(original);

    ASSERT_NOT_NULL(clone);
    ASSERT_TRUE(clone != original);  /* Different memory */

    /* Values should match */
    ASSERT_STR_EQ(clone->name, original->name);
    ASSERT_STR_EQ(clone->author, original->author);

    /* Strings should be deep copied (different pointers) */
    ASSERT_TRUE(clone->name != original->name);
    ASSERT_TRUE(clone->author != original->author);

    tracker_theme_free(clone);
}

TEST(theme_clone_retro) {
    const TrackerTheme* original = tracker_theme_get("retro");
    TrackerTheme* clone = tracker_theme_clone(original);

    ASSERT_NOT_NULL(clone);
    ASSERT_STR_EQ(clone->name, "retro");

    tracker_theme_free(clone);
}

TEST(theme_clone_null_returns_null) {
    TrackerTheme* clone = tracker_theme_clone(NULL);
    ASSERT_NULL(clone);
}

TEST(theme_clone_copies_styles) {
    const TrackerTheme* original = tracker_theme_get("default");
    TrackerTheme* clone = tracker_theme_clone(original);

    /* Check style values are copied */
    ASSERT_EQ(clone->default_style.fg.type, original->default_style.fg.type);
    ASSERT_EQ(clone->default_style.bg.type, original->default_style.bg.type);
    ASSERT_EQ(clone->default_style.attr, original->default_style.attr);

    ASSERT_EQ(clone->cursor.fg.type, original->cursor.fg.type);
    ASSERT_EQ(clone->cursor.bg.type, original->cursor.bg.type);

    tracker_theme_free(clone);
}

TEST(theme_clone_copies_border_chars) {
    const TrackerTheme* original = tracker_theme_get("default");
    TrackerTheme* clone = tracker_theme_clone(original);

    ASSERT_STR_EQ(clone->border_h, original->border_h);
    ASSERT_STR_EQ(clone->border_v, original->border_v);
    ASSERT_STR_EQ(clone->note_off_marker, original->note_off_marker);
    ASSERT_STR_EQ(clone->empty_cell, original->empty_cell);

    /* Should be deep copies */
    ASSERT_TRUE(clone->border_h != original->border_h);
    ASSERT_TRUE(clone->border_v != original->border_v);

    tracker_theme_free(clone);
}

TEST(theme_clone_copies_colors) {
    const TrackerTheme* original = tracker_theme_get("retro");
    TrackerTheme* clone = tracker_theme_clone(original);

    /* Retro theme uses RGB colors */
    ASSERT_EQ(clone->default_style.fg.type, TRACKER_COLOR_RGB);
    ASSERT_EQ(clone->default_style.fg.value.rgb.r,
              original->default_style.fg.value.rgb.r);
    ASSERT_EQ(clone->default_style.fg.value.rgb.g,
              original->default_style.fg.value.rgb.g);
    ASSERT_EQ(clone->default_style.fg.value.rgb.b,
              original->default_style.fg.value.rgb.b);

    tracker_theme_free(clone);
}

TEST(theme_clone_modifiable) {
    const TrackerTheme* original = tracker_theme_get("default");
    TrackerTheme* clone = tracker_theme_clone(original);

    /* Modifying clone should not affect original */
    clone->default_style.attr = TRACKER_ATTR_BOLD;

    ASSERT_EQ(clone->default_style.attr, TRACKER_ATTR_BOLD);
    ASSERT_EQ(original->default_style.attr, TRACKER_ATTR_NONE);

    tracker_theme_free(clone);
}

/*============================================================================
 * Theme Free Tests
 *============================================================================*/

TEST(theme_free_null_safe) {
    /* Should not crash */
    tracker_theme_free(NULL);
}

TEST(theme_free_cloned_theme) {
    const TrackerTheme* original = tracker_theme_get("default");
    TrackerTheme* clone = tracker_theme_clone(original);

    /* Should not crash */
    tracker_theme_free(clone);
}

TEST(theme_free_builtin_safe) {
    /* Freeing a built-in theme should be safe (no-op) */
    const TrackerTheme* builtin = tracker_theme_get("default");

    /* This should not crash or free static memory */
    tracker_theme_free((TrackerTheme*)builtin);

    /* Theme should still be accessible */
    const TrackerTheme* still_valid = tracker_theme_get("default");
    ASSERT_NOT_NULL(still_valid);
    ASSERT_STR_EQ(still_valid->name, "default");
}

/*============================================================================
 * Default Theme Properties Tests
 *============================================================================*/

TEST(theme_default_indexed_colors) {
    const TrackerTheme* theme = tracker_theme_get("default");

    /* Default theme uses indexed colors */
    ASSERT_EQ(theme->default_style.fg.type, TRACKER_COLOR_INDEXED);
    ASSERT_EQ(theme->default_style.bg.type, TRACKER_COLOR_INDEXED);
    ASSERT_EQ(theme->header_style.fg.type, TRACKER_COLOR_INDEXED);
    ASSERT_EQ(theme->cursor.fg.type, TRACKER_COLOR_INDEXED);
    ASSERT_EQ(theme->cell_note.fg.type, TRACKER_COLOR_INDEXED);
}

TEST(theme_default_white_on_black) {
    const TrackerTheme* theme = tracker_theme_get("default");

    /* Default is white text on black background */
    ASSERT_EQ(theme->default_style.fg.value.index, 7);  /* white */
    ASSERT_EQ(theme->default_style.bg.value.index, 0);  /* black */
}

TEST(theme_default_cursor_inverted) {
    const TrackerTheme* theme = tracker_theme_get("default");

    /* Cursor is inverted (black on white) */
    ASSERT_EQ(theme->cursor.fg.value.index, 0);  /* black */
    ASSERT_EQ(theme->cursor.bg.value.index, 7);  /* white */
}

TEST(theme_default_error_red) {
    const TrackerTheme* theme = tracker_theme_get("default");

    /* Error style is red */
    ASSERT_EQ(theme->error_style.fg.value.index, 1);  /* red */
    ASSERT_EQ(theme->error_style.attr, TRACKER_ATTR_BOLD);
}

TEST(theme_default_message_green) {
    const TrackerTheme* theme = tracker_theme_get("default");

    /* Message style is green */
    ASSERT_EQ(theme->message_style.fg.value.index, 2);  /* green */
}

TEST(theme_default_playing_row_green) {
    const TrackerTheme* theme = tracker_theme_get("default");

    /* Playing row has green background */
    ASSERT_EQ(theme->playing_row.bg.value.index, 2);  /* green */
}

TEST(theme_default_ascii_borders) {
    const TrackerTheme* theme = tracker_theme_get("default");

    /* Default uses ASCII border characters */
    ASSERT_STR_EQ(theme->border_h, "-");
    ASSERT_STR_EQ(theme->border_v, "|");
    ASSERT_STR_EQ(theme->border_corner_tl, "+");
}

TEST(theme_default_markers) {
    const TrackerTheme* theme = tracker_theme_get("default");

    ASSERT_STR_EQ(theme->note_off_marker, "===");
    ASSERT_STR_EQ(theme->continuation_marker, "...");
    ASSERT_STR_EQ(theme->empty_cell, "---");
}

/*============================================================================
 * Retro Theme Properties Tests
 *============================================================================*/

TEST(theme_retro_rgb_colors) {
    const TrackerTheme* theme = tracker_theme_get("retro");

    /* Retro theme uses RGB colors */
    ASSERT_EQ(theme->default_style.fg.type, TRACKER_COLOR_RGB);
    ASSERT_EQ(theme->default_style.bg.type, TRACKER_COLOR_RGB);
}

TEST(theme_retro_blue_background) {
    const TrackerTheme* theme = tracker_theme_get("retro");

    /* Classic blue background (like FastTracker) */
    ASSERT_EQ(theme->default_style.bg.value.rgb.r, 0);
    ASSERT_EQ(theme->default_style.bg.value.rgb.g, 0);
    ASSERT_EQ(theme->default_style.bg.value.rgb.b, 85);
}

TEST(theme_retro_header_yellow) {
    const TrackerTheme* theme = tracker_theme_get("retro");

    /* Yellow header */
    ASSERT_EQ(theme->header_style.fg.value.rgb.r, 255);
    ASSERT_EQ(theme->header_style.fg.value.rgb.g, 255);
    ASSERT_EQ(theme->header_style.fg.value.rgb.b, 85);
}

TEST(theme_retro_ascii_borders) {
    const TrackerTheme* theme = tracker_theme_get("retro");

    /* Also uses ASCII borders */
    ASSERT_STR_EQ(theme->border_h, "-");
    ASSERT_STR_EQ(theme->border_v, "|");
}

TEST(theme_retro_different_empty_cell) {
    const TrackerTheme* theme = tracker_theme_get("retro");

    /* Retro uses "..." for empty cells */
    ASSERT_STR_EQ(theme->empty_cell, "...");
}

/*============================================================================
 * Color Helper Tests
 *============================================================================*/

TEST(color_default_helper) {
    TrackerColor c = tracker_color_default();
    ASSERT_EQ(c.type, TRACKER_COLOR_DEFAULT);
}

TEST(color_indexed_helper) {
    TrackerColor c = tracker_color_indexed(42);
    ASSERT_EQ(c.type, TRACKER_COLOR_INDEXED);
    ASSERT_EQ(c.value.index, 42);
}

TEST(color_rgb_helper) {
    TrackerColor c = tracker_color_rgb(128, 64, 32);
    ASSERT_EQ(c.type, TRACKER_COLOR_RGB);
    ASSERT_EQ(c.value.rgb.r, 128);
    ASSERT_EQ(c.value.rgb.g, 64);
    ASSERT_EQ(c.value.rgb.b, 32);
}

TEST(color_hex_helper) {
    TrackerColor c = tracker_color_hex(0xFF8040);
    ASSERT_EQ(c.type, TRACKER_COLOR_RGB);
    ASSERT_EQ(c.value.rgb.r, 0xFF);
    ASSERT_EQ(c.value.rgb.g, 0x80);
    ASSERT_EQ(c.value.rgb.b, 0x40);
}

TEST(color_hex_black) {
    TrackerColor c = tracker_color_hex(0x000000);
    ASSERT_EQ(c.value.rgb.r, 0);
    ASSERT_EQ(c.value.rgb.g, 0);
    ASSERT_EQ(c.value.rgb.b, 0);
}

TEST(color_hex_white) {
    TrackerColor c = tracker_color_hex(0xFFFFFF);
    ASSERT_EQ(c.value.rgb.r, 255);
    ASSERT_EQ(c.value.rgb.g, 255);
    ASSERT_EQ(c.value.rgb.b, 255);
}

/*============================================================================
 * Style Helper Tests
 *============================================================================*/

TEST(style_helper) {
    TrackerStyle s = tracker_style(
        tracker_color_indexed(1),
        tracker_color_indexed(2),
        TRACKER_ATTR_BOLD | TRACKER_ATTR_UNDERLINE
    );

    ASSERT_EQ(s.fg.type, TRACKER_COLOR_INDEXED);
    ASSERT_EQ(s.fg.value.index, 1);
    ASSERT_EQ(s.bg.type, TRACKER_COLOR_INDEXED);
    ASSERT_EQ(s.bg.value.index, 2);
    ASSERT_EQ(s.attr, TRACKER_ATTR_BOLD | TRACKER_ATTR_UNDERLINE);
}

/*============================================================================
 * Text Attribute Tests
 *============================================================================*/

TEST(attr_none) {
    ASSERT_EQ(TRACKER_ATTR_NONE, 0);
}

TEST(attr_flags_distinct) {
    /* Each attribute should be a distinct bit */
    ASSERT_TRUE((TRACKER_ATTR_BOLD & TRACKER_ATTR_DIM) == 0);
    ASSERT_TRUE((TRACKER_ATTR_BOLD & TRACKER_ATTR_ITALIC) == 0);
    ASSERT_TRUE((TRACKER_ATTR_BOLD & TRACKER_ATTR_UNDERLINE) == 0);
    ASSERT_TRUE((TRACKER_ATTR_DIM & TRACKER_ATTR_ITALIC) == 0);
}

TEST(attr_combinable) {
    uint8_t combined = TRACKER_ATTR_BOLD | TRACKER_ATTR_UNDERLINE;
    ASSERT_TRUE(combined & TRACKER_ATTR_BOLD);
    ASSERT_TRUE(combined & TRACKER_ATTR_UNDERLINE);
    ASSERT_FALSE(combined & TRACKER_ATTR_DIM);
}

/*============================================================================
 * Main
 *============================================================================*/

BEGIN_TEST_SUITE("tracker_view_theme")
    /* Theme get */
    RUN_TEST(theme_get_default);
    RUN_TEST(theme_get_retro);
    RUN_TEST(theme_get_null_returns_default);
    RUN_TEST(theme_get_unknown_returns_null);
    RUN_TEST(theme_get_empty_string_returns_null);
    RUN_TEST(theme_get_case_sensitive);

    /* Theme list */
    RUN_TEST(theme_list_returns_names);
    RUN_TEST(theme_list_contains_default);
    RUN_TEST(theme_list_contains_retro);
    RUN_TEST(theme_list_null_count);
    RUN_TEST(theme_list_all_valid);

    /* Theme init default */
    RUN_TEST(theme_init_default_basic);
    RUN_TEST(theme_init_default_null_safe);
    RUN_TEST(theme_init_default_styles);
    RUN_TEST(theme_init_default_border_chars);
    RUN_TEST(theme_init_default_velocity_styles);

    /* Theme clone */
    RUN_TEST(theme_clone_default);
    RUN_TEST(theme_clone_retro);
    RUN_TEST(theme_clone_null_returns_null);
    RUN_TEST(theme_clone_copies_styles);
    RUN_TEST(theme_clone_copies_border_chars);
    RUN_TEST(theme_clone_copies_colors);
    RUN_TEST(theme_clone_modifiable);

    /* Theme free */
    RUN_TEST(theme_free_null_safe);
    RUN_TEST(theme_free_cloned_theme);
    RUN_TEST(theme_free_builtin_safe);

    /* Default theme properties */
    RUN_TEST(theme_default_indexed_colors);
    RUN_TEST(theme_default_white_on_black);
    RUN_TEST(theme_default_cursor_inverted);
    RUN_TEST(theme_default_error_red);
    RUN_TEST(theme_default_message_green);
    RUN_TEST(theme_default_playing_row_green);
    RUN_TEST(theme_default_ascii_borders);
    RUN_TEST(theme_default_markers);

    /* Retro theme properties */
    RUN_TEST(theme_retro_rgb_colors);
    RUN_TEST(theme_retro_blue_background);
    RUN_TEST(theme_retro_header_yellow);
    RUN_TEST(theme_retro_ascii_borders);
    RUN_TEST(theme_retro_different_empty_cell);

    /* Color helpers */
    RUN_TEST(color_default_helper);
    RUN_TEST(color_indexed_helper);
    RUN_TEST(color_rgb_helper);
    RUN_TEST(color_hex_helper);
    RUN_TEST(color_hex_black);
    RUN_TEST(color_hex_white);

    /* Style helper */
    RUN_TEST(style_helper);

    /* Text attributes */
    RUN_TEST(attr_none);
    RUN_TEST(attr_flags_distinct);
    RUN_TEST(attr_combinable);
END_TEST_SUITE()
