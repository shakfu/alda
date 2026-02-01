/* metronome.c - Tempo tap and metronome commands
 *
 * Provides:
 *   :tap       - Tap to set tempo (tap multiple times)
 *   :metronome - Toggle metronome on/off
 *
 * The metronome uses beat detection similar to live_loop, playing
 * drum sounds on beat boundaries via the shared audio backend.
 */

#include "command_impl.h"
#include "loki/link.h"
#include "shared/context.h"
#include <sys/time.h>
#include <math.h>

/* ============================================================================
 * Tempo Tap State
 * ============================================================================ */

#define TAP_MAX_SAMPLES 8       /* Number of taps to average */
#define TAP_TIMEOUT_MS  2000    /* Reset if gap exceeds 2 seconds */
#define TAP_MIN_BPM     30.0
#define TAP_MAX_BPM     300.0

static struct {
    long long timestamps[TAP_MAX_SAMPLES];  /* Millisecond timestamps */
    int count;                               /* Number of valid samples */
    int index;                               /* Next write position (circular) */
} g_tap_state = {0};

/* Get current time in milliseconds */
static long long tap_get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* Reset tap state (called on timeout or explicit reset) */
static void tap_reset(void) {
    g_tap_state.count = 0;
    g_tap_state.index = 0;
}

/* Calculate BPM from tap timestamps, returns 0 if not enough data */
static double tap_calculate_bpm(void) {
    if (g_tap_state.count < 2) return 0.0;

    /* Calculate average interval between consecutive taps */
    long long total_interval = 0;
    int intervals = 0;

    /* Find oldest and newest timestamps in circular buffer */
    int oldest_idx = (g_tap_state.index - g_tap_state.count + TAP_MAX_SAMPLES) % TAP_MAX_SAMPLES;

    for (int i = 1; i < g_tap_state.count; i++) {
        int prev_idx = (oldest_idx + i - 1) % TAP_MAX_SAMPLES;
        int curr_idx = (oldest_idx + i) % TAP_MAX_SAMPLES;
        long long interval = g_tap_state.timestamps[curr_idx] - g_tap_state.timestamps[prev_idx];
        if (interval > 0) {
            total_interval += interval;
            intervals++;
        }
    }

    if (intervals == 0) return 0.0;

    double avg_interval_ms = (double)total_interval / intervals;
    if (avg_interval_ms <= 0) return 0.0;

    double bpm = 60000.0 / avg_interval_ms;

    /* Clamp to reasonable range */
    if (bpm < TAP_MIN_BPM) bpm = TAP_MIN_BPM;
    if (bpm > TAP_MAX_BPM) bpm = TAP_MAX_BPM;

    return bpm;
}

/* ============================================================================
 * Metronome State
 * ============================================================================ */

#define METRONOME_CHANNEL   10  /* MIDI drum channel */
#define METRONOME_ACCENT    36  /* Kick drum for downbeat */
#define METRONOME_TICK      42  /* Hi-hat for subdivisions */
#define METRONOME_VEL_ACCENT 100
#define METRONOME_VEL_TICK   60

static struct {
    int enabled;          /* Metronome is running */
    int subdivisions;     /* Clicks per beat (1=quarter, 2=eighth, 4=sixteenth) */
    double quantum;       /* Beats per bar (typically 4) */
    double last_beat;     /* Last beat position we played on */
} g_metronome = {
    .enabled = 0,
    .subdivisions = 1,
    .quantum = 4.0,
    .last_beat = 0.0
};

/* ============================================================================
 * Commands
 * ============================================================================ */

/**
 * :tap - Tap tempo command
 *
 * Each invocation records a tap timestamp. After 2+ taps, calculates
 * average BPM and sets it via Link (if enabled) or context tempo.
 *
 * Usage:
 *   :tap        - Record a tap
 *   :tap reset  - Clear tap history
 */
int cmd_tap(editor_ctx_t *ctx, const char *args) {
    /* Handle reset argument */
    if (args && strncmp(args, "reset", 5) == 0) {
        tap_reset();
        editor_set_status_msg(ctx, "Tap tempo reset");
        return 1;
    }

    long long now = tap_get_time_ms();

    /* Check for timeout - reset if last tap was too long ago */
    if (g_tap_state.count > 0) {
        int last_idx = (g_tap_state.index - 1 + TAP_MAX_SAMPLES) % TAP_MAX_SAMPLES;
        long long last_time = g_tap_state.timestamps[last_idx];
        if (now - last_time > TAP_TIMEOUT_MS) {
            tap_reset();
        }
    }

    /* Record this tap */
    g_tap_state.timestamps[g_tap_state.index] = now;
    g_tap_state.index = (g_tap_state.index + 1) % TAP_MAX_SAMPLES;
    if (g_tap_state.count < TAP_MAX_SAMPLES) {
        g_tap_state.count++;
    }

    /* Calculate and set BPM if we have enough data */
    double bpm = tap_calculate_bpm();
    if (bpm > 0) {
        /* Set tempo via Link if enabled, otherwise set context tempo */
        if (loki_link_is_enabled(ctx)) {
            loki_link_set_tempo(ctx, bpm);
        } else if (ctx->model.shared) {
            ctx->model.shared->tempo = (int)round(bpm);
        }
        editor_set_status_msg(ctx, "Tempo: %.1f BPM (%d taps)", bpm, g_tap_state.count);
    } else {
        editor_set_status_msg(ctx, "Tap... (%d)", g_tap_state.count);
    }

    return 1;
}

/**
 * :metronome - Toggle metronome
 *
 * Usage:
 *   :metronome         - Toggle on/off
 *   :metronome on      - Turn on
 *   :metronome off     - Turn off
 *   :metronome 2       - Set subdivisions (1=quarter, 2=eighth, 4=sixteenth)
 */
int cmd_metronome(editor_ctx_t *ctx, const char *args) {
    /* Parse arguments */
    if (args && args[0]) {
        if (strcmp(args, "on") == 0) {
            g_metronome.enabled = 1;
        } else if (strcmp(args, "off") == 0) {
            g_metronome.enabled = 0;
        } else {
            /* Try to parse as subdivision number */
            int subdiv = atoi(args);
            if (subdiv >= 1 && subdiv <= 8) {
                g_metronome.subdivisions = subdiv;
                g_metronome.enabled = 1;
            } else {
                editor_set_status_msg(ctx, "Usage: :metronome [on|off|1-8]");
                return 0;
            }
        }
    } else {
        /* Toggle */
        g_metronome.enabled = !g_metronome.enabled;
    }

    /* Reset beat tracking when enabling */
    if (g_metronome.enabled) {
        if (loki_link_is_enabled(ctx)) {
            g_metronome.last_beat = loki_link_get_beat(ctx, g_metronome.quantum);
        } else {
            g_metronome.last_beat = 0.0;
        }
    }

    if (g_metronome.enabled) {
        const char *subdiv_name = "";
        switch (g_metronome.subdivisions) {
            case 1: subdiv_name = "quarter"; break;
            case 2: subdiv_name = "eighth"; break;
            case 4: subdiv_name = "sixteenth"; break;
            default: subdiv_name = ""; break;
        }
        if (subdiv_name[0]) {
            editor_set_status_msg(ctx, "Metronome ON (%s notes)", subdiv_name);
        } else {
            editor_set_status_msg(ctx, "Metronome ON (1/%d)", g_metronome.subdivisions);
        }
    } else {
        editor_set_status_msg(ctx, "Metronome OFF");
    }

    return 1;
}

/* ============================================================================
 * Main Loop Integration
 * ============================================================================ */

/**
 * Check if metronome is enabled.
 * Used by main loop to decide whether to call metronome_tick().
 */
int metronome_is_enabled(void) {
    return g_metronome.enabled;
}

/**
 * Tick function called from editor main loop.
 * Plays metronome clicks on beat boundaries.
 *
 * @param ctx Editor context (for Link and shared audio access)
 */
void metronome_tick(editor_ctx_t *ctx) {
    if (!g_metronome.enabled || !ctx) return;

    /* Need Link for beat-synced metronome */
    if (!loki_link_is_enabled(ctx)) {
        /* Could implement time-based fallback here, but for now require Link */
        return;
    }

    SharedContext *shared = ctx->model.shared;
    if (!shared) return;

    /* Get current beat position */
    double beat_unit = 1.0 / g_metronome.subdivisions;
    double current_beat = loki_link_get_beat(ctx, g_metronome.quantum);

    /* Detect beat boundary crossing */
    double current_tick = floor(current_beat / beat_unit);
    double last_tick = floor(g_metronome.last_beat / beat_unit);

    if (current_tick > last_tick) {
        /* Determine if this is a downbeat (start of bar) */
        double phase = loki_link_get_phase(ctx, g_metronome.quantum);
        int is_downbeat = (phase < beat_unit);

        /* Also check for beat 1 (accent on quarter note) */
        int beat_in_bar = (int)floor(phase);
        int is_quarter_accent = (g_metronome.subdivisions > 1) &&
                                (fmod(current_beat, 1.0) < beat_unit);

        int pitch, velocity;
        if (is_downbeat) {
            /* Bar downbeat - loudest accent */
            pitch = METRONOME_ACCENT;
            velocity = METRONOME_VEL_ACCENT;
        } else if (is_quarter_accent && beat_in_bar == 0) {
            /* Quarter note accent */
            pitch = METRONOME_ACCENT;
            velocity = METRONOME_VEL_ACCENT - 20;
        } else {
            /* Subdivision tick */
            pitch = METRONOME_TICK;
            velocity = METRONOME_VEL_TICK;
        }

        /* Send the click */
        shared_send_note_on(shared, METRONOME_CHANNEL, pitch, velocity);

        /* Note off after very short duration (handled by synth decay) */
        /* For drum sounds, note-off is often ignored, but send anyway */
        shared_send_note_off(shared, METRONOME_CHANNEL, pitch);
    }

    g_metronome.last_beat = current_beat;
}

/**
 * Stop metronome (called from :stop command).
 */
void metronome_stop(void) {
    g_metronome.enabled = 0;
}
