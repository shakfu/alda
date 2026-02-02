/**
 * @file repl.c
 * @brief Joy REPL - Interactive stack-based music composition terminal.
 */

#include "repl.h"
#include "psnd.h"
#include "loki/core.h"
#include "loki/internal.h"
#include "loki/syntax.h"
#include "loki/lua.h"
#include "loki/repl_launcher.h"
#include "loki/repl_helpers.h"
#include "shared/repl_commands.h"
#include "shared/context.h"

/* Joy library headers */
#include "joy_runtime.h"
#include "joy_parser.h"
#include "joy_midi_backend.h"
#include "joy_async.h"
#include "music_notation.h"
#include "music_context.h"
#include "midi_primitives.h"

/* REPL-owned SharedContext for multi-context support */
static SharedContext* g_joy_repl_shared = NULL;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#define usleep(us) Sleep((us) / 1000)
#else
#include <unistd.h>
#endif
#include <setjmp.h>
#include <sys/stat.h>

/* ============================================================================
 * Joy Usage and Help
 * ============================================================================ */

static void print_joy_repl_usage(const char *prog) {
    printf("Usage: %s joy [options] [file.joy]\n", prog);
    printf("\n");
    printf("Joy concatenative music language interpreter with MIDI output.\n");
    printf("If no file is provided, starts an interactive REPL.\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help        Show this help message\n");
    printf("  -v, --verbose     Enable verbose output\n");
    printf("  -l, --list        List available MIDI ports\n");
    printf("  -p, --port N      Use MIDI port N (0-based index)\n");
    printf("  --virtual NAME    Create virtual MIDI port with NAME\n");
    printf("\n");
    printf("Built-in Synth Options:\n");
    printf("  -sf, --soundfont PATH  Use built-in synth with soundfont (.sf2)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s joy                   Start interactive Joy REPL\n", prog);
    printf("  %s joy song.joy          Execute a Joy file\n", prog);
    printf("  %s joy -sf gm.sf2        REPL with built-in synth\n", prog);
    printf("  %s joy --virtual JoyOut  REPL with virtual MIDI port\n", prog);
    printf("\n");
}

static void print_joy_repl_help(void) {
    shared_print_command_help();

    printf("Joy-specific Commands:\n");
    printf("  .               Print stack\n");
    printf("\n");
    printf("Joy Syntax:\n");
    printf("  c d e f g a b   Note names (octave 4 by default)\n");
    printf("  c5 d3 e6        Notes with explicit octave\n");
    printf("  c+ c-           Sharps and flats\n");
    printf("  [c d e] play    Play notes sequentially\n");
    printf("  [c e g] chord   Play notes as chord\n");
    printf("  c major chord   Build and play C major chord\n");
    printf("  120 tempo       Set tempo to 120 BPM\n");
    printf("  80 vol          Set volume to 80%%\n");
    printf("\n");
    printf("Combinators:\n");
    printf("  [1 2 3] [2 *] map   -> [2 4 6]\n");
    printf("  [c d e] [12 +] map  -> transpose up octave\n");
    printf("  5 [c e g] times     -> repeat 5 times\n");
    printf("\n");
}

/* ============================================================================
 * Joy REPL Loop
 * ============================================================================ */

/* Stop callback for Joy REPL */
static void joy_stop_playback(void) {
    /* Stop async playback first */
    joy_async_stop();
    /* Then send panic to silence any remaining notes */
    joy_midi_panic(g_joy_repl_shared);
}

/* Forward declarations for skeleton callbacks */
static int joy_skel_process_command(void *ctx, const char *input);
static void joy_skel_eval_line(void *ctx, const char *input);
static void joy_skel_print_banner(void);

/* Note: Using repl_repl_starts_with() from loki/repl_helpers.h */

/* Process a Joy REPL command. Returns: 0=continue, 1=quit, 2=evaluate as Joy code */
static int joy_process_command(JoyContext* ctx, const char* input) {
    /* Try shared commands first */
    int result = shared_process_command(g_joy_repl_shared, input, joy_stop_playback);
    if (result == REPL_CMD_QUIT) {
        return 1; /* quit */
    }
    if (result == REPL_CMD_HANDLED) {
        return 0;
    }

    /* Handle Joy-specific commands */
    const char *cmd = input;
    if (cmd[0] == ':')
        cmd++;

    /* Help - add Joy-specific help */
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0 || strcmp(cmd, "?") == 0) {
        print_joy_repl_help();
        return 0;
    }

    /* :play file.joy - load and execute a Joy file */
    if (repl_starts_with(cmd, "play ")) {
        const char* path = cmd + 5;
        while (*path == ' ') path++;
        if (*path) {
            printf("Loading %s...\n", path);
            int load_result = joy_load_file(ctx, path);
            if (load_result != 0) {
                printf("Failed to load file: %s\n", path);
            }
        } else {
            printf("Usage: :play PATH\n");
        }
        return 0;
    }

    /* Print stack */
    if (strcmp(input, ".") == 0) {
        /* Print stack - handled by caller */
        return 2;
    }

    return 2; /* evaluate as Joy code */
}

/* Tab completion callback for Joy REPL */
static char **joy_completion_callback(const char *prefix, int *count, void *user_data) {
    JoyContext *ctx = (JoyContext *)user_data;
    if (!ctx || !ctx->dictionary) return NULL;

    return joy_dict_get_completions(ctx->dictionary, prefix, count, REPL_COMPLETIONS_MAX);
}

/* ============================================================================
 * REPL Skeleton Callbacks
 * ============================================================================ */

/* Global for error recovery - needed across skeleton callbacks */
static jmp_buf g_joy_error_recovery;

/* Skeleton callback: process command */
static int joy_skel_process_command(void *ctx, const char *input) {
    return joy_process_command((JoyContext *)ctx, input);
}

/* Skeleton callback: evaluate line */
static void joy_skel_eval_line(void *ctx, const char *input) {
    JoyContext *joy_ctx = (JoyContext *)ctx;

    /* Set up error recovery point */
    if (setjmp(g_joy_error_recovery) != 0) {
        /* Error occurred during eval - just return */
        return;
    }

    joy_ctx->error_jmp = &g_joy_error_recovery;
    joy_set_current_context(joy_ctx);

    /* Evaluate Joy code */
    joy_eval_line(joy_ctx, input);
}

/* Skeleton callback: print banner */
static void joy_skel_print_banner(void) {
    printf("Joy REPL %s (type help for help, quit to exit)\n", PSND_VERSION);
}

static void joy_repl_loop(JoyContext *ctx, editor_ctx_t *syntax_ctx) {
    /* Set up context for error recovery */
    ctx->error_jmp = &g_joy_error_recovery;
    joy_set_current_context(ctx);

    /* Configure the REPL skeleton */
    ReplSkeletonConfig config = {
        .process_command = joy_skel_process_command,
        .eval_line = joy_skel_eval_line,
        .print_banner = joy_skel_print_banner,
        .on_iteration = shared_repl_link_check,
        .completion_fn = joy_completion_callback,
        .completion_user_data = ctx,
        .prompt = "joy> ",
        .lang_name = "joy",
        .syntax_lang = REPL_SKEL_LANG_JOY,
        .lang_ctx = ctx
    };

    repl_skeleton_run(&config, syntax_ctx);
}

/* ============================================================================
 * Shared REPL Launcher Callbacks
 * ============================================================================ */

/* List MIDI ports */
static void joy_cb_list_ports(void) {
    SharedContext temp_ctx;
    if (shared_context_init(&temp_ctx) == 0) {
        joy_midi_list_ports(&temp_ctx);
        shared_context_cleanup(&temp_ctx);
    }
}

/* Initialize Joy context and MIDI/audio */
static void *joy_cb_init(const SharedReplArgs *args) {
    /* Initialize Joy context */
    JoyContext *ctx = joy_context_new();
    if (!ctx) {
        fprintf(stderr, "Error: Failed to create Joy context\n");
        return NULL;
    }

    /* Register primitives */
    joy_register_primitives(ctx);
    music_notation_init(ctx);
    joy_midi_register_primitives(ctx);

    /* Set parser dictionary for DEFINE support */
    joy_set_parser_dict(ctx->dictionary);

    /*
     * Create REPL-owned SharedContext and pass to Joy.
     * This ensures multiple REPL instances don't stomp each other's context.
     */
    g_joy_repl_shared = (SharedContext*)malloc(sizeof(SharedContext));
    if (!g_joy_repl_shared) {
        fprintf(stderr, "Error: Failed to allocate shared context\n");
        music_notation_cleanup(ctx);
        joy_context_free(ctx);
        return NULL;
    }

    if (shared_context_init(g_joy_repl_shared) != 0) {
        fprintf(stderr, "Error: Failed to initialize shared context\n");
        free(g_joy_repl_shared);
        g_joy_repl_shared = NULL;
        music_notation_cleanup(ctx);
        joy_context_free(ctx);
        return NULL;
    }

    /* Link SharedContext to MusicContext so primitives can access it */
    MusicContext* mctx = music_get_context(ctx);
    if (mctx) {
        music_context_set_shared(mctx, g_joy_repl_shared);
    }

    /* Setup output */
    if (args->soundfont_path) {
        /* Use built-in synth */
        if (joy_tsf_load_soundfont(args->soundfont_path) != 0) {
            fprintf(stderr, "Error: Failed to load soundfont: %s\n", args->soundfont_path);
            shared_context_cleanup(g_joy_repl_shared);
            free(g_joy_repl_shared);
            g_joy_repl_shared = NULL;
            music_notation_cleanup(ctx);
            joy_context_free(ctx);
            return NULL;
        }
        if (joy_tsf_enable(g_joy_repl_shared) != 0) {
            fprintf(stderr, "Error: Failed to enable built-in synth\n");
            shared_context_cleanup(g_joy_repl_shared);
            free(g_joy_repl_shared);
            g_joy_repl_shared = NULL;
            music_notation_cleanup(ctx);
            joy_context_free(ctx);
            return NULL;
        }
        if (args->verbose) {
            printf("Using built-in synth: %s\n", args->soundfont_path);
        }
    } else {
        /* Setup MIDI output */
        int midi_opened = 0;

        if (args->virtual_name) {
            if (joy_midi_open_virtual(g_joy_repl_shared, args->virtual_name) == 0) {
                midi_opened = 1;
                if (args->verbose) {
                    printf("Created virtual MIDI output: %s\n", args->virtual_name);
                }
            }
        } else if (args->port_index >= 0) {
            if (joy_midi_open_port(g_joy_repl_shared, args->port_index) == 0) {
                midi_opened = 1;
            }
        } else {
            /* Try to open a virtual port by default */
            if (joy_midi_open_virtual(g_joy_repl_shared, PSND_MIDI_PORT_NAME) == 0) {
                midi_opened = 1;
                if (args->verbose) {
                    printf("Created virtual MIDI output: " PSND_MIDI_PORT_NAME "\n");
                }
            }
        }

        if (!midi_opened) {
            fprintf(stderr, "Warning: No MIDI output available\n");
            fprintf(stderr, "Hint: Use -sf <soundfont.sf2> for built-in synth\n");
        }
    }

    /* Initialize async playback system */
    if (joy_async_init() != 0) {
        fprintf(stderr, "Warning: Failed to initialize async playback\n");
        /* Non-fatal - continue with sync playback fallback */
    }

    /* Initialize Link callbacks for REPL notifications */
    shared_repl_link_init_callbacks(g_joy_repl_shared);

    return ctx;
}

/* Cleanup Joy context and MIDI/audio */
static void joy_cb_cleanup(void *lang_ctx) {
    JoyContext *ctx = (JoyContext *)lang_ctx;

    /* Cleanup Link callbacks */
    shared_repl_link_cleanup_callbacks();

    /* Wait for async playback to finish (with timeout) */
    if (joy_async_is_playing()) {
        joy_async_wait(5000);  /* Wait up to 5 seconds */
    }

    /* Cleanup async playback system */
    joy_async_cleanup();

    /* Wait for audio buffer to drain */
    if (joy_tsf_is_enabled(g_joy_repl_shared)) {
        usleep(300000);  /* 300ms for audio tail */
    }

    /* Send panic and cleanup backends */
    joy_midi_panic(g_joy_repl_shared);
    joy_csound_cleanup(g_joy_repl_shared);
    joy_link_cleanup();

    /* Free REPL-owned SharedContext */
    if (g_joy_repl_shared) {
        shared_context_cleanup(g_joy_repl_shared);
        free(g_joy_repl_shared);
        g_joy_repl_shared = NULL;
    }

    music_notation_cleanup(ctx);
    joy_context_free(ctx);
}

/* Execute a Joy file */
static int joy_cb_exec_file(void *lang_ctx, const char *path, int verbose) {
    JoyContext *ctx = (JoyContext *)lang_ctx;
    (void)verbose;

    int result = joy_load_file(ctx, path);
    if (result != 0) {
        fprintf(stderr, "Error: Failed to execute file\n");
    }
    return result;
}

/* Run the Joy REPL loop */
static void joy_cb_repl_loop(void *lang_ctx, editor_ctx_t *syntax_ctx) {
    JoyContext *ctx = (JoyContext *)lang_ctx;
    joy_repl_loop(ctx, syntax_ctx);
}

/* Joy shared REPL callbacks */
static const SharedReplCallbacks joy_repl_callbacks = {
    .name = "joy",
    .file_ext = ".joy",
    .prog_name = PSND_NAME,
    .print_usage = print_joy_repl_usage,
    .list_ports = joy_cb_list_ports,
    .init = joy_cb_init,
    .cleanup = joy_cb_cleanup,
    .exec_file = joy_cb_exec_file,
    .repl_loop = joy_cb_repl_loop,
};

/* ============================================================================
 * Joy REPL Main Entry Point
 * ============================================================================ */

int joy_repl_main(int argc, char **argv) {
    return shared_lang_repl_main(&joy_repl_callbacks, argc, argv);
}

/* ============================================================================
 * Joy Play Main Entry Point (headless file execution)
 * ============================================================================ */

int joy_play_main(int argc, char **argv) {
    return shared_lang_play_main(&joy_repl_callbacks, argc, argv);
}
