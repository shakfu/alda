/**
 * @file test_music_theory.c
 * @brief Tests for music theory utilities.
 *
 * Tests verify:
 * - Pitch parsing (note name to MIDI number)
 * - Pitch to name conversion
 * - Chord building
 * - Scale building and membership
 * - Scale degree calculation
 * - Scale quantization
 * - Microtonal functions (cents to note/bend)
 * - Dynamics parsing
 * - Duration calculation
 */

#include "test_framework.h"
#include "music/music_theory.h"

/* ============================================================================
 * Pitch Parsing Tests
 * ============================================================================ */

TEST(parse_pitch_middle_c) {
    ASSERT_EQ(music_parse_pitch("C4"), 60);
    ASSERT_EQ(music_parse_pitch("c4"), 60);  /* case insensitive */
}

TEST(parse_pitch_all_naturals) {
    ASSERT_EQ(music_parse_pitch("C4"), 60);
    ASSERT_EQ(music_parse_pitch("D4"), 62);
    ASSERT_EQ(music_parse_pitch("E4"), 64);
    ASSERT_EQ(music_parse_pitch("F4"), 65);
    ASSERT_EQ(music_parse_pitch("G4"), 67);
    ASSERT_EQ(music_parse_pitch("A4"), 69);
    ASSERT_EQ(music_parse_pitch("B4"), 71);
}

TEST(parse_pitch_sharps) {
    ASSERT_EQ(music_parse_pitch("C#4"), 61);
    ASSERT_EQ(music_parse_pitch("Cs4"), 61);  /* alternate sharp notation */
    ASSERT_EQ(music_parse_pitch("CS4"), 61);  /* case insensitive */
    ASSERT_EQ(music_parse_pitch("F#4"), 66);
    ASSERT_EQ(music_parse_pitch("G#4"), 68);
}

TEST(parse_pitch_flats) {
    ASSERT_EQ(music_parse_pitch("Db4"), 61);
    ASSERT_EQ(music_parse_pitch("DB4"), 61);  /* case insensitive */
    ASSERT_EQ(music_parse_pitch("Eb4"), 63);
    ASSERT_EQ(music_parse_pitch("Bb4"), 70);
}

TEST(parse_pitch_octaves) {
    ASSERT_EQ(music_parse_pitch("C0"), 12);
    ASSERT_EQ(music_parse_pitch("C1"), 24);
    ASSERT_EQ(music_parse_pitch("C2"), 36);
    ASSERT_EQ(music_parse_pitch("C3"), 48);
    ASSERT_EQ(music_parse_pitch("C5"), 72);
    ASSERT_EQ(music_parse_pitch("C6"), 84);
    ASSERT_EQ(music_parse_pitch("C7"), 96);
    ASSERT_EQ(music_parse_pitch("C8"), 108);
}

TEST(parse_pitch_negative_octave) {
    ASSERT_EQ(music_parse_pitch("C-1"), 0);   /* lowest MIDI note */
    ASSERT_EQ(music_parse_pitch("G-1"), 7);
    ASSERT_EQ(music_parse_pitch("B-1"), 11);
}

TEST(parse_pitch_highest_notes) {
    ASSERT_EQ(music_parse_pitch("G9"), 127);  /* highest MIDI note */
    ASSERT_EQ(music_parse_pitch("F#9"), 126);
    ASSERT_EQ(music_parse_pitch("C9"), 120);
}

TEST(parse_pitch_invalid) {
    ASSERT_EQ(music_parse_pitch(NULL), -1);
    ASSERT_EQ(music_parse_pitch(""), -1);
    ASSERT_EQ(music_parse_pitch("X4"), -1);   /* invalid note letter */
    ASSERT_EQ(music_parse_pitch("C"), -1);    /* missing octave */
    ASSERT_EQ(music_parse_pitch("C#"), -1);   /* missing octave */
    ASSERT_EQ(music_parse_pitch("4"), -1);    /* missing note */
    ASSERT_EQ(music_parse_pitch("C10"), -1);  /* octave too high */
    ASSERT_EQ(music_parse_pitch("C-2"), -1);  /* octave too low */
    ASSERT_EQ(music_parse_pitch("G#9"), -1);  /* would exceed 127 */
}

TEST(parse_pitch_enharmonic) {
    /* Enharmonic equivalents should produce same MIDI number */
    ASSERT_EQ(music_parse_pitch("C#4"), music_parse_pitch("Db4"));
    ASSERT_EQ(music_parse_pitch("D#4"), music_parse_pitch("Eb4"));
    ASSERT_EQ(music_parse_pitch("F#4"), music_parse_pitch("Gb4"));
    ASSERT_EQ(music_parse_pitch("G#4"), music_parse_pitch("Ab4"));
    ASSERT_EQ(music_parse_pitch("A#4"), music_parse_pitch("Bb4"));
}

/* ============================================================================
 * Pitch To Name Tests
 * ============================================================================ */

TEST(pitch_to_name_middle_c) {
    char buf[8];
    ASSERT_NOT_NULL(music_pitch_to_name(60, buf, sizeof(buf), 1));
    ASSERT_STR_EQ(buf, "C4");
}

TEST(pitch_to_name_sharps) {
    char buf[8];
    music_pitch_to_name(61, buf, sizeof(buf), 1);  /* use sharps */
    ASSERT_STR_EQ(buf, "C#4");

    music_pitch_to_name(66, buf, sizeof(buf), 1);
    ASSERT_STR_EQ(buf, "F#4");
}

TEST(pitch_to_name_flats) {
    char buf[8];
    music_pitch_to_name(61, buf, sizeof(buf), 0);  /* use flats */
    ASSERT_STR_EQ(buf, "Db4");

    music_pitch_to_name(63, buf, sizeof(buf), 0);
    ASSERT_STR_EQ(buf, "Eb4");
}

TEST(pitch_to_name_octaves) {
    char buf[8];

    music_pitch_to_name(0, buf, sizeof(buf), 1);
    ASSERT_STR_EQ(buf, "C-1");

    music_pitch_to_name(12, buf, sizeof(buf), 1);
    ASSERT_STR_EQ(buf, "C0");

    music_pitch_to_name(127, buf, sizeof(buf), 1);
    ASSERT_STR_EQ(buf, "G9");
}

TEST(pitch_to_name_roundtrip) {
    char buf[8];
    /* Verify roundtrip: parse -> name -> parse */
    for (int pitch = 0; pitch <= 127; pitch++) {
        music_pitch_to_name(pitch, buf, sizeof(buf), 1);
        int parsed = music_parse_pitch(buf);
        ASSERT_EQ(parsed, pitch);
    }
}

TEST(pitch_to_name_invalid) {
    char buf[8];
    ASSERT_NULL(music_pitch_to_name(-1, buf, sizeof(buf), 1));
    ASSERT_NULL(music_pitch_to_name(128, buf, sizeof(buf), 1));
    ASSERT_NULL(music_pitch_to_name(60, NULL, 8, 1));
    ASSERT_NULL(music_pitch_to_name(60, buf, 3, 1));  /* buffer too small */
}

/* ============================================================================
 * Chord Building Tests
 * ============================================================================ */

TEST(build_chord_major) {
    int pitches[4];
    int count = music_build_chord(60, CHORD_MAJOR, CHORD_TRIAD_SIZE, pitches);
    ASSERT_EQ(count, 3);
    ASSERT_EQ(pitches[0], 60);  /* C4 */
    ASSERT_EQ(pitches[1], 64);  /* E4 */
    ASSERT_EQ(pitches[2], 67);  /* G4 */
}

TEST(build_chord_minor) {
    int pitches[4];
    int count = music_build_chord(60, CHORD_MINOR, CHORD_TRIAD_SIZE, pitches);
    ASSERT_EQ(count, 3);
    ASSERT_EQ(pitches[0], 60);  /* C4 */
    ASSERT_EQ(pitches[1], 63);  /* Eb4 */
    ASSERT_EQ(pitches[2], 67);  /* G4 */
}

TEST(build_chord_diminished) {
    int pitches[4];
    int count = music_build_chord(60, CHORD_DIM, CHORD_TRIAD_SIZE, pitches);
    ASSERT_EQ(count, 3);
    ASSERT_EQ(pitches[0], 60);  /* C4 */
    ASSERT_EQ(pitches[1], 63);  /* Eb4 */
    ASSERT_EQ(pitches[2], 66);  /* Gb4 */
}

TEST(build_chord_augmented) {
    int pitches[4];
    int count = music_build_chord(60, CHORD_AUG, CHORD_TRIAD_SIZE, pitches);
    ASSERT_EQ(count, 3);
    ASSERT_EQ(pitches[0], 60);  /* C4 */
    ASSERT_EQ(pitches[1], 64);  /* E4 */
    ASSERT_EQ(pitches[2], 68);  /* G#4 */
}

TEST(build_chord_dom7) {
    int pitches[4];
    int count = music_build_chord(60, CHORD_DOM7, CHORD_7TH_SIZE, pitches);
    ASSERT_EQ(count, 4);
    ASSERT_EQ(pitches[0], 60);  /* C4 */
    ASSERT_EQ(pitches[1], 64);  /* E4 */
    ASSERT_EQ(pitches[2], 67);  /* G4 */
    ASSERT_EQ(pitches[3], 70);  /* Bb4 */
}

TEST(build_chord_maj7) {
    int pitches[4];
    int count = music_build_chord(60, CHORD_MAJ7, CHORD_7TH_SIZE, pitches);
    ASSERT_EQ(count, 4);
    ASSERT_EQ(pitches[0], 60);  /* C4 */
    ASSERT_EQ(pitches[1], 64);  /* E4 */
    ASSERT_EQ(pitches[2], 67);  /* G4 */
    ASSERT_EQ(pitches[3], 71);  /* B4 */
}

TEST(build_chord_sus2) {
    int pitches[4];
    int count = music_build_chord(60, CHORD_SUS2, CHORD_TRIAD_SIZE, pitches);
    ASSERT_EQ(count, 3);
    ASSERT_EQ(pitches[0], 60);  /* C4 */
    ASSERT_EQ(pitches[1], 62);  /* D4 */
    ASSERT_EQ(pitches[2], 67);  /* G4 */
}

TEST(build_chord_sus4) {
    int pitches[4];
    int count = music_build_chord(60, CHORD_SUS4, CHORD_TRIAD_SIZE, pitches);
    ASSERT_EQ(count, 3);
    ASSERT_EQ(pitches[0], 60);  /* C4 */
    ASSERT_EQ(pitches[1], 65);  /* F4 */
    ASSERT_EQ(pitches[2], 67);  /* G4 */
}

TEST(build_chord_high_root) {
    /* Chord near top of MIDI range - some notes may be clipped */
    int pitches[4];
    int count = music_build_chord(120, CHORD_MAJOR, CHORD_TRIAD_SIZE, pitches);
    /* 120 + 7 = 127, but 120 + 11 would exceed 127 if we used maj7 */
    ASSERT_EQ(count, 3);
    ASSERT_EQ(pitches[0], 120);
    ASSERT_EQ(pitches[1], 124);
    ASSERT_EQ(pitches[2], 127);
}

TEST(build_chord_clipping) {
    /* Root so high that some chord tones exceed MIDI range */
    int pitches[4];
    int count = music_build_chord(125, CHORD_MAJOR, CHORD_TRIAD_SIZE, pitches);
    /* 125 + 4 = 129 > 127, so only root (125) and possibly one more */
    ASSERT_TRUE(count < 3);
    ASSERT_EQ(pitches[0], 125);
}

TEST(build_chord_invalid) {
    int pitches[4];
    ASSERT_EQ(music_build_chord(-1, CHORD_MAJOR, 3, pitches), 0);
    ASSERT_EQ(music_build_chord(128, CHORD_MAJOR, 3, pitches), 0);
    ASSERT_EQ(music_build_chord(60, NULL, 3, pitches), 0);
    ASSERT_EQ(music_build_chord(60, CHORD_MAJOR, 3, NULL), 0);
    ASSERT_EQ(music_build_chord(60, CHORD_MAJOR, 0, pitches), 0);
}

/* ============================================================================
 * Scale Building Tests
 * ============================================================================ */

TEST(build_scale_major) {
    int pitches[8];
    int count = music_build_scale(60, SCALE_MAJOR, SCALE_DIATONIC_SIZE, pitches);
    ASSERT_EQ(count, 7);
    ASSERT_EQ(pitches[0], 60);  /* C */
    ASSERT_EQ(pitches[1], 62);  /* D */
    ASSERT_EQ(pitches[2], 64);  /* E */
    ASSERT_EQ(pitches[3], 65);  /* F */
    ASSERT_EQ(pitches[4], 67);  /* G */
    ASSERT_EQ(pitches[5], 69);  /* A */
    ASSERT_EQ(pitches[6], 71);  /* B */
}

TEST(build_scale_minor) {
    int pitches[8];
    int count = music_build_scale(60, SCALE_MINOR, SCALE_DIATONIC_SIZE, pitches);
    ASSERT_EQ(count, 7);
    ASSERT_EQ(pitches[0], 60);  /* C */
    ASSERT_EQ(pitches[1], 62);  /* D */
    ASSERT_EQ(pitches[2], 63);  /* Eb */
    ASSERT_EQ(pitches[3], 65);  /* F */
    ASSERT_EQ(pitches[4], 67);  /* G */
    ASSERT_EQ(pitches[5], 68);  /* Ab */
    ASSERT_EQ(pitches[6], 70);  /* Bb */
}

TEST(build_scale_pentatonic_major) {
    int pitches[8];
    int count = music_build_scale(60, SCALE_PENTATONIC_MAJOR, SCALE_PENTATONIC_SIZE, pitches);
    ASSERT_EQ(count, 5);
    ASSERT_EQ(pitches[0], 60);  /* C */
    ASSERT_EQ(pitches[1], 62);  /* D */
    ASSERT_EQ(pitches[2], 64);  /* E */
    ASSERT_EQ(pitches[3], 67);  /* G */
    ASSERT_EQ(pitches[4], 69);  /* A */
}

TEST(build_scale_blues) {
    int pitches[8];
    int count = music_build_scale(60, SCALE_BLUES, SCALE_BLUES_SIZE, pitches);
    ASSERT_EQ(count, 6);
    ASSERT_EQ(pitches[0], 60);  /* C */
    ASSERT_EQ(pitches[1], 63);  /* Eb */
    ASSERT_EQ(pitches[2], 65);  /* F */
    ASSERT_EQ(pitches[3], 66);  /* Gb (blue note) */
    ASSERT_EQ(pitches[4], 67);  /* G */
    ASSERT_EQ(pitches[5], 70);  /* Bb */
}

TEST(build_scale_chromatic) {
    int pitches[16];
    int count = music_build_scale(60, SCALE_CHROMATIC, SCALE_CHROMATIC_SIZE, pitches);
    ASSERT_EQ(count, 12);
    for (int i = 0; i < 12; i++) {
        ASSERT_EQ(pitches[i], 60 + i);
    }
}

TEST(build_scale_invalid) {
    int pitches[8];
    ASSERT_EQ(music_build_scale(-1, SCALE_MAJOR, 7, pitches), 0);
    ASSERT_EQ(music_build_scale(128, SCALE_MAJOR, 7, pitches), 0);
    ASSERT_EQ(music_build_scale(60, NULL, 7, pitches), 0);
    ASSERT_EQ(music_build_scale(60, SCALE_MAJOR, 7, NULL), 0);
    ASSERT_EQ(music_build_scale(60, SCALE_MAJOR, 0, pitches), 0);
}

/* ============================================================================
 * Scale Degree Tests
 * ============================================================================ */

TEST(scale_degree_basic) {
    /* C major scale degrees */
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 1), 60);  /* root = C */
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 2), 62);  /* 2nd = D */
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 3), 64);  /* 3rd = E */
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 4), 65);  /* 4th = F */
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 5), 67);  /* 5th = G */
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 6), 69);  /* 6th = A */
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 7), 71);  /* 7th = B */
}

TEST(scale_degree_extended) {
    /* Degrees beyond octave */
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 8), 72);   /* 8th = C5 (octave) */
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 9), 74);   /* 9th = D5 */
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 10), 76);  /* 10th = E5 */
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 15), 84);  /* two octaves up */
}

TEST(scale_degree_minor) {
    /* A minor scale degrees (same as C major but from A) */
    ASSERT_EQ(music_scale_degree(69, SCALE_MINOR, 7, 1), 69);  /* A */
    ASSERT_EQ(music_scale_degree(69, SCALE_MINOR, 7, 3), 72);  /* C (minor 3rd) */
    ASSERT_EQ(music_scale_degree(69, SCALE_MINOR, 7, 5), 76);  /* E */
}

TEST(scale_degree_invalid) {
    ASSERT_EQ(music_scale_degree(60, NULL, 7, 1), -1);
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 0, 1), -1);
    ASSERT_EQ(music_scale_degree(60, SCALE_MAJOR, 7, 0), -1);
    ASSERT_EQ(music_scale_degree(-1, SCALE_MAJOR, 7, 1), -1);
    ASSERT_EQ(music_scale_degree(128, SCALE_MAJOR, 7, 1), -1);
}

/* ============================================================================
 * Scale Membership Tests
 * ============================================================================ */

TEST(in_scale_c_major) {
    /* Notes in C major */
    ASSERT_TRUE(music_in_scale(60, 60, SCALE_MAJOR, 7));  /* C */
    ASSERT_TRUE(music_in_scale(62, 60, SCALE_MAJOR, 7));  /* D */
    ASSERT_TRUE(music_in_scale(64, 60, SCALE_MAJOR, 7));  /* E */
    ASSERT_TRUE(music_in_scale(65, 60, SCALE_MAJOR, 7));  /* F */
    ASSERT_TRUE(music_in_scale(67, 60, SCALE_MAJOR, 7));  /* G */
    ASSERT_TRUE(music_in_scale(69, 60, SCALE_MAJOR, 7));  /* A */
    ASSERT_TRUE(music_in_scale(71, 60, SCALE_MAJOR, 7));  /* B */

    /* Notes NOT in C major */
    ASSERT_FALSE(music_in_scale(61, 60, SCALE_MAJOR, 7));  /* C# */
    ASSERT_FALSE(music_in_scale(63, 60, SCALE_MAJOR, 7));  /* Eb */
    ASSERT_FALSE(music_in_scale(66, 60, SCALE_MAJOR, 7));  /* F# */
    ASSERT_FALSE(music_in_scale(68, 60, SCALE_MAJOR, 7));  /* G# */
    ASSERT_FALSE(music_in_scale(70, 60, SCALE_MAJOR, 7));  /* Bb */
}

TEST(in_scale_octave_invariant) {
    /* Same pitch class in different octaves */
    ASSERT_TRUE(music_in_scale(48, 60, SCALE_MAJOR, 7));   /* C3 */
    ASSERT_TRUE(music_in_scale(72, 60, SCALE_MAJOR, 7));   /* C5 */
    ASSERT_TRUE(music_in_scale(84, 60, SCALE_MAJOR, 7));   /* C6 */

    ASSERT_FALSE(music_in_scale(49, 60, SCALE_MAJOR, 7));  /* C#3 */
    ASSERT_FALSE(music_in_scale(73, 60, SCALE_MAJOR, 7));  /* C#5 */
}

TEST(in_scale_different_roots) {
    /* G major: G A B C D E F# */
    ASSERT_TRUE(music_in_scale(67, 67, SCALE_MAJOR, 7));   /* G */
    ASSERT_TRUE(music_in_scale(66, 67, SCALE_MAJOR, 7));   /* F# */
    ASSERT_FALSE(music_in_scale(65, 67, SCALE_MAJOR, 7));  /* F natural not in G major */
}

TEST(in_scale_invalid) {
    ASSERT_FALSE(music_in_scale(60, 60, NULL, 7));
    ASSERT_FALSE(music_in_scale(60, 60, SCALE_MAJOR, 0));
    ASSERT_FALSE(music_in_scale(-1, 60, SCALE_MAJOR, 7));
    ASSERT_FALSE(music_in_scale(128, 60, SCALE_MAJOR, 7));
    ASSERT_FALSE(music_in_scale(60, -1, SCALE_MAJOR, 7));
    ASSERT_FALSE(music_in_scale(60, 128, SCALE_MAJOR, 7));
}

/* ============================================================================
 * Scale Quantization Tests
 * ============================================================================ */

TEST(quantize_already_in_scale) {
    /* Notes already in scale should stay unchanged */
    ASSERT_EQ(music_quantize_to_scale(60, 60, SCALE_MAJOR, 7), 60);
    ASSERT_EQ(music_quantize_to_scale(62, 60, SCALE_MAJOR, 7), 62);
    ASSERT_EQ(music_quantize_to_scale(64, 60, SCALE_MAJOR, 7), 64);
}

TEST(quantize_to_nearest) {
    /* C# (61) should quantize to C (60) or D (62) - nearest wins */
    int result = music_quantize_to_scale(61, 60, SCALE_MAJOR, 7);
    ASSERT_TRUE(result == 60 || result == 62);

    /* Eb (63) should quantize to D (62) or E (64) */
    result = music_quantize_to_scale(63, 60, SCALE_MAJOR, 7);
    ASSERT_TRUE(result == 62 || result == 64);

    /* F# (66) should quantize to F (65) or G (67) */
    result = music_quantize_to_scale(66, 60, SCALE_MAJOR, 7);
    ASSERT_TRUE(result == 65 || result == 67);
}

TEST(quantize_preserves_range) {
    /* Result should be valid MIDI pitch */
    int result = music_quantize_to_scale(0, 0, SCALE_MAJOR, 7);
    ASSERT_TRUE(result >= 0 && result <= 127);

    result = music_quantize_to_scale(127, 60, SCALE_MAJOR, 7);
    ASSERT_TRUE(result >= 0 && result <= 127);
}

TEST(quantize_invalid_passthrough) {
    /* Invalid inputs should return original pitch */
    ASSERT_EQ(music_quantize_to_scale(60, 60, NULL, 7), 60);
    ASSERT_EQ(music_quantize_to_scale(60, 60, SCALE_MAJOR, 0), 60);
}

/* ============================================================================
 * Microtonal Tests
 * ============================================================================ */

TEST(cents_to_note_zero) {
    MicrotonalNote note = music_cents_to_note(60, 0);
    ASSERT_EQ(note.midi_note, 60);
    ASSERT_EQ(note.bend_cents, 0);
}

TEST(cents_to_note_semitone) {
    MicrotonalNote note = music_cents_to_note(60, 100);
    ASSERT_EQ(note.midi_note, 61);
    ASSERT_EQ(note.bend_cents, 0);
}

TEST(cents_to_note_quarter_tone) {
    MicrotonalNote note = music_cents_to_note(60, 50);
    /* 50 cents is exactly a quarter tone - should round down */
    ASSERT_EQ(note.midi_note, 60);
    ASSERT_EQ(note.bend_cents, 50);
}

TEST(cents_to_note_three_quarter_tone) {
    MicrotonalNote note = music_cents_to_note(60, 150);
    /* 150 cents = 1 semitone + 50 cents remainder */
    /* Implementation uses >50 threshold (not >=), so no rounding up */
    ASSERT_EQ(note.midi_note, 61);
    ASSERT_EQ(note.bend_cents, 50);
}

TEST(cents_to_note_octave) {
    MicrotonalNote note = music_cents_to_note(60, 1200);
    ASSERT_EQ(note.midi_note, 72);
    ASSERT_EQ(note.bend_cents, 0);
}

TEST(cents_to_note_negative) {
    MicrotonalNote note = music_cents_to_note(60, -100);
    ASSERT_EQ(note.midi_note, 59);
}

TEST(cents_to_note_clamp_high) {
    MicrotonalNote note = music_cents_to_note(120, 1200);
    /* Would be 132, but clamped to 127 */
    ASSERT_EQ(note.midi_note, 127);
    ASSERT_EQ(note.bend_cents, 0);
}

TEST(cents_to_bend_center) {
    ASSERT_EQ(music_cents_to_bend(0), 8192);  /* center */
}

TEST(cents_to_bend_semitone_up) {
    /* +100 cents = +1 semitone = halfway to max (for 2-semitone range) */
    int bend = music_cents_to_bend(100);
    ASSERT_EQ(bend, 8192 + 4096);  /* 12288 */
}

TEST(cents_to_bend_semitone_down) {
    int bend = music_cents_to_bend(-100);
    ASSERT_EQ(bend, 8192 - 4096);  /* 4096 */
}

TEST(cents_to_bend_max) {
    /* +200 cents (full range up) */
    int bend = music_cents_to_bend(200);
    ASSERT_EQ(bend, 16383);  /* max */
}

TEST(cents_to_bend_min) {
    /* -200 cents (full range down) */
    int bend = music_cents_to_bend(-200);
    ASSERT_EQ(bend, 0);  /* min */
}

TEST(cents_to_bend_clamp) {
    /* Beyond range should clamp */
    ASSERT_EQ(music_cents_to_bend(300), 16383);
    ASSERT_EQ(music_cents_to_bend(-300), 0);
}

TEST(build_microtonal_scale) {
    MicrotonalNote notes[8];
    int count = music_build_microtonal_scale(60, SCALE_MAQAM_BAYATI_CENTS, 7, notes);
    ASSERT_EQ(count, 7);
    ASSERT_EQ(notes[0].midi_note, 60);
    ASSERT_EQ(notes[0].bend_cents, 0);
    /* Second note at 150 cents (neutral second) = 1 semitone + 50 cents */
    ASSERT_EQ(notes[1].midi_note, 61);
    ASSERT_EQ(notes[1].bend_cents, 50);
}

TEST(microtonal_degree) {
    MicrotonalNote note = music_microtonal_degree(60, SCALE_MAQAM_BAYATI_CENTS, 7, 1);
    ASSERT_EQ(note.midi_note, 60);
    ASSERT_EQ(note.bend_cents, 0);

    note = music_microtonal_degree(60, SCALE_MAQAM_BAYATI_CENTS, 7, 2);
    ASSERT_EQ(note.midi_note, 61);  /* 150 cents = 1 semitone + 50 cents bend */
    ASSERT_EQ(note.bend_cents, 50);
}

/* ============================================================================
 * Dynamics Tests
 * ============================================================================ */

TEST(parse_dynamics_all) {
    ASSERT_EQ(music_parse_dynamics("ppp"), DYN_PPP);
    ASSERT_EQ(music_parse_dynamics("pp"), DYN_PP);
    ASSERT_EQ(music_parse_dynamics("p"), DYN_P);
    ASSERT_EQ(music_parse_dynamics("mp"), DYN_MP);
    ASSERT_EQ(music_parse_dynamics("mf"), DYN_MF);
    ASSERT_EQ(music_parse_dynamics("f"), DYN_F);
    ASSERT_EQ(music_parse_dynamics("ff"), DYN_FF);
    ASSERT_EQ(music_parse_dynamics("fff"), DYN_FFF);
}

TEST(parse_dynamics_case_insensitive) {
    ASSERT_EQ(music_parse_dynamics("PPP"), DYN_PPP);
    ASSERT_EQ(music_parse_dynamics("Mf"), DYN_MF);
    ASSERT_EQ(music_parse_dynamics("FF"), DYN_FF);
}

TEST(parse_dynamics_invalid) {
    ASSERT_EQ(music_parse_dynamics(NULL), -1);
    ASSERT_EQ(music_parse_dynamics(""), -1);
    ASSERT_EQ(music_parse_dynamics("loud"), -1);
    ASSERT_EQ(music_parse_dynamics("ffff"), -1);
    ASSERT_EQ(music_parse_dynamics("pppp"), -1);
}

TEST(dynamics_velocity_values) {
    /* Verify actual velocity values */
    ASSERT_EQ(DYN_PPP, 16);
    ASSERT_EQ(DYN_PP, 33);
    ASSERT_EQ(DYN_P, 49);
    ASSERT_EQ(DYN_MP, 64);
    ASSERT_EQ(DYN_MF, 80);
    ASSERT_EQ(DYN_F, 96);
    ASSERT_EQ(DYN_FF, 112);
    ASSERT_EQ(DYN_FFF, 127);
}

/* ============================================================================
 * Duration Tests
 * ============================================================================ */

TEST(duration_ms_120bpm) {
    /* At 120 BPM, 1 beat = 500ms */
    ASSERT_EQ(music_duration_ms(1.0, 120), 500);
    ASSERT_EQ(music_duration_ms(2.0, 120), 1000);
    ASSERT_EQ(music_duration_ms(0.5, 120), 250);
    ASSERT_EQ(music_duration_ms(4.0, 120), 2000);
}

TEST(duration_ms_60bpm) {
    /* At 60 BPM, 1 beat = 1000ms */
    ASSERT_EQ(music_duration_ms(1.0, 60), 1000);
    ASSERT_EQ(music_duration_ms(0.5, 60), 500);
}

TEST(duration_ms_other_tempos) {
    /* At 90 BPM, 1 beat = 666.67ms */
    int dur = music_duration_ms(1.0, 90);
    ASSERT_TRUE(dur >= 666 && dur <= 667);

    /* At 180 BPM, 1 beat = 333.33ms */
    dur = music_duration_ms(1.0, 180);
    ASSERT_TRUE(dur >= 333 && dur <= 334);
}

TEST(duration_ms_invalid_bpm) {
    ASSERT_EQ(music_duration_ms(1.0, 0), 0);
    ASSERT_EQ(music_duration_ms(1.0, -120), 0);
}

TEST(duration_constants) {
    /* Verify duration constants (at 120 BPM) */
    ASSERT_EQ(DUR_WHOLE, 2000);
    ASSERT_EQ(DUR_HALF, 1000);
    ASSERT_EQ(DUR_QUARTER, 500);
    ASSERT_EQ(DUR_EIGHTH, 250);
    ASSERT_EQ(DUR_SIXTEENTH, 125);
}

TEST(dotted_duration) {
    ASSERT_EQ(music_dotted(500), 750);    /* dotted quarter */
    ASSERT_EQ(music_dotted(1000), 1500);  /* dotted half */
    ASSERT_EQ(music_dotted(250), 375);    /* dotted eighth */
}

/* ============================================================================
 * Chord Interval Verification
 * ============================================================================ */

TEST(chord_intervals_correct) {
    /* Verify chord interval arrays have correct values */
    ASSERT_EQ(CHORD_MAJOR[0], 0);
    ASSERT_EQ(CHORD_MAJOR[1], 4);
    ASSERT_EQ(CHORD_MAJOR[2], 7);

    ASSERT_EQ(CHORD_MINOR[0], 0);
    ASSERT_EQ(CHORD_MINOR[1], 3);
    ASSERT_EQ(CHORD_MINOR[2], 7);

    ASSERT_EQ(CHORD_DIM[0], 0);
    ASSERT_EQ(CHORD_DIM[1], 3);
    ASSERT_EQ(CHORD_DIM[2], 6);

    ASSERT_EQ(CHORD_DOM7[0], 0);
    ASSERT_EQ(CHORD_DOM7[1], 4);
    ASSERT_EQ(CHORD_DOM7[2], 7);
    ASSERT_EQ(CHORD_DOM7[3], 10);
}

/* ============================================================================
 * Scale Interval Verification
 * ============================================================================ */

TEST(scale_intervals_major) {
    /* Major scale: W W H W W W H */
    ASSERT_EQ(SCALE_MAJOR[0], 0);
    ASSERT_EQ(SCALE_MAJOR[1], 2);
    ASSERT_EQ(SCALE_MAJOR[2], 4);
    ASSERT_EQ(SCALE_MAJOR[3], 5);
    ASSERT_EQ(SCALE_MAJOR[4], 7);
    ASSERT_EQ(SCALE_MAJOR[5], 9);
    ASSERT_EQ(SCALE_MAJOR[6], 11);
}

TEST(scale_intervals_minor) {
    /* Natural minor: W H W W H W W */
    ASSERT_EQ(SCALE_MINOR[0], 0);
    ASSERT_EQ(SCALE_MINOR[1], 2);
    ASSERT_EQ(SCALE_MINOR[2], 3);
    ASSERT_EQ(SCALE_MINOR[3], 5);
    ASSERT_EQ(SCALE_MINOR[4], 7);
    ASSERT_EQ(SCALE_MINOR[5], 8);
    ASSERT_EQ(SCALE_MINOR[6], 10);
}

TEST(scale_modes_relationship) {
    /* Dorian is major with b3 and b7 */
    ASSERT_EQ(SCALE_DORIAN[2], 3);   /* minor 3rd */
    ASSERT_EQ(SCALE_DORIAN[6], 10);  /* minor 7th */

    /* Lydian is major with #4 */
    ASSERT_EQ(SCALE_LYDIAN[3], 6);   /* augmented 4th */

    /* Mixolydian is major with b7 */
    ASSERT_EQ(SCALE_MIXOLYDIAN[6], 10);  /* minor 7th */
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

/* Global test stats required by framework */
test_stats_t test_stats;

BEGIN_TEST_SUITE("Music Theory")

    /* Pitch parsing */
    RUN_TEST(parse_pitch_middle_c);
    RUN_TEST(parse_pitch_all_naturals);
    RUN_TEST(parse_pitch_sharps);
    RUN_TEST(parse_pitch_flats);
    RUN_TEST(parse_pitch_octaves);
    RUN_TEST(parse_pitch_negative_octave);
    RUN_TEST(parse_pitch_highest_notes);
    RUN_TEST(parse_pitch_invalid);
    RUN_TEST(parse_pitch_enharmonic);

    /* Pitch to name */
    RUN_TEST(pitch_to_name_middle_c);
    RUN_TEST(pitch_to_name_sharps);
    RUN_TEST(pitch_to_name_flats);
    RUN_TEST(pitch_to_name_octaves);
    RUN_TEST(pitch_to_name_roundtrip);
    RUN_TEST(pitch_to_name_invalid);

    /* Chord building */
    RUN_TEST(build_chord_major);
    RUN_TEST(build_chord_minor);
    RUN_TEST(build_chord_diminished);
    RUN_TEST(build_chord_augmented);
    RUN_TEST(build_chord_dom7);
    RUN_TEST(build_chord_maj7);
    RUN_TEST(build_chord_sus2);
    RUN_TEST(build_chord_sus4);
    RUN_TEST(build_chord_high_root);
    RUN_TEST(build_chord_clipping);
    RUN_TEST(build_chord_invalid);

    /* Scale building */
    RUN_TEST(build_scale_major);
    RUN_TEST(build_scale_minor);
    RUN_TEST(build_scale_pentatonic_major);
    RUN_TEST(build_scale_blues);
    RUN_TEST(build_scale_chromatic);
    RUN_TEST(build_scale_invalid);

    /* Scale degree */
    RUN_TEST(scale_degree_basic);
    RUN_TEST(scale_degree_extended);
    RUN_TEST(scale_degree_minor);
    RUN_TEST(scale_degree_invalid);

    /* Scale membership */
    RUN_TEST(in_scale_c_major);
    RUN_TEST(in_scale_octave_invariant);
    RUN_TEST(in_scale_different_roots);
    RUN_TEST(in_scale_invalid);

    /* Scale quantization */
    RUN_TEST(quantize_already_in_scale);
    RUN_TEST(quantize_to_nearest);
    RUN_TEST(quantize_preserves_range);
    RUN_TEST(quantize_invalid_passthrough);

    /* Microtonal */
    RUN_TEST(cents_to_note_zero);
    RUN_TEST(cents_to_note_semitone);
    RUN_TEST(cents_to_note_quarter_tone);
    RUN_TEST(cents_to_note_three_quarter_tone);
    RUN_TEST(cents_to_note_octave);
    RUN_TEST(cents_to_note_negative);
    RUN_TEST(cents_to_note_clamp_high);
    RUN_TEST(cents_to_bend_center);
    RUN_TEST(cents_to_bend_semitone_up);
    RUN_TEST(cents_to_bend_semitone_down);
    RUN_TEST(cents_to_bend_max);
    RUN_TEST(cents_to_bend_min);
    RUN_TEST(cents_to_bend_clamp);
    RUN_TEST(build_microtonal_scale);
    RUN_TEST(microtonal_degree);

    /* Dynamics */
    RUN_TEST(parse_dynamics_all);
    RUN_TEST(parse_dynamics_case_insensitive);
    RUN_TEST(parse_dynamics_invalid);
    RUN_TEST(dynamics_velocity_values);

    /* Duration */
    RUN_TEST(duration_ms_120bpm);
    RUN_TEST(duration_ms_60bpm);
    RUN_TEST(duration_ms_other_tempos);
    RUN_TEST(duration_ms_invalid_bpm);
    RUN_TEST(duration_constants);
    RUN_TEST(dotted_duration);

    /* Interval verification */
    RUN_TEST(chord_intervals_correct);
    RUN_TEST(scale_intervals_major);
    RUN_TEST(scale_intervals_minor);
    RUN_TEST(scale_modes_relationship);

END_TEST_SUITE()
