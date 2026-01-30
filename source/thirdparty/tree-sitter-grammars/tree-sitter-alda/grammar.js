/**
 * Tree-sitter grammar for Alda music notation language
 * https://alda.io/
 */

module.exports = grammar({
  name: 'alda',

  extras: $ => [
    /[ \t\r]+/,
    $.comment,
  ],

  conflicts: $ => [
    [$.note, $.chord],
  ],

  rules: {
    source_file: $ => repeat($._toplevel),

    _toplevel: $ => choice(
      $.part_declaration,
      $.variable_definition,
      $._event,
      /\n/,
    ),

    comment: $ => /#[^\n]*/,

    // Part declaration: piano: or violin/viola "strings":
    part_declaration: $ => prec.right(1, seq(
      $.instrument_call,
      repeat($._event),
    )),

    instrument_call: $ => seq(
      $.identifier,
      repeat(seq('/', $.identifier)),
      optional($.string),
      ':',
    ),

    // Variable definition: notes = c d e f
    variable_definition: $ => prec.right(1, seq(
      $.identifier,
      '=',
      repeat1($._event),
    )),

    _event: $ => choice(
      $.note,
      $.rest,
      $.chord,
      $.sexp,
      $.octave_set,
      $.octave_up,
      $.octave_down,
      $.barline,
      $.marker,
      $.at_marker,
      $.voice_marker,
      $.cram,
      $.bracket_seq,
    ),

    // Notes: c, c4, c+4, c4., c4~4, etc.
    note: $ => prec.left(seq(
      $.pitch,
      optional($.duration),
      optional($.repeat_count),
      optional($.on_repetitions),
    )),

    pitch: $ => seq(
      $.note_letter,
      repeat($.accidental),
    ),

    note_letter: $ => /[a-gA-G]/,

    accidental: $ => choice('+', '-', '_'),

    // Rest: r, r4, r4., etc.
    rest: $ => seq(
      /[rR]/,
      optional($.duration),
      optional($.repeat_count),
      optional($.on_repetitions),
    ),

    // Chord: c/e/g, c/e/g4
    chord: $ => prec.left(1, seq(
      $.pitch,
      repeat1(seq('/', choice($.pitch, $.octave_up, $.octave_down))),
      optional($.duration),
      optional($.repeat_count),
      optional($.on_repetitions),
    )),

    // Duration: 4, 4., 4.., 500ms, 2s, 4~4, etc.
    duration: $ => prec.left(seq(
      $._duration_value,
      repeat($.dot),
      repeat($.tie_duration),
    )),

    _duration_value: $ => choice(
      $.note_length,
      $.duration_ms,
      $.duration_s,
    ),

    note_length: $ => /[1-9][0-9]*/,
    duration_ms: $ => /[0-9]+ms/,
    duration_s: $ => /[0-9]+s/,
    dot: $ => '.',
    tie_duration: $ => seq('~', $._duration_value, repeat($.dot)),

    // Octave control
    octave_set: $ => /o[0-9]/,
    octave_up: $ => '>',
    octave_down: $ => '<',

    // Barline
    barline: $ => '|',

    // Markers
    marker: $ => /%[a-zA-Z_][a-zA-Z0-9_\-]*/,
    at_marker: $ => /@[a-zA-Z_][a-zA-Z0-9_\-]*/,

    // Voice markers: V1:, V2:, V0:
    voice_marker: $ => /V[0-9]+:/,

    // Cram: {c d e f}4
    cram: $ => seq(
      '{',
      repeat($._event),
      '}',
      optional($.duration),
      optional($.repeat_count),
    ),

    // Bracket sequence: [c d e f]
    bracket_seq: $ => seq(
      '[',
      repeat($._event),
      ']',
      optional($.duration),
      optional($.repeat_count),
      optional($.on_repetitions),
    ),

    // Repetition: *3
    repeat_count: $ => /\*[0-9]+/,

    // On repetitions: '1-3,5
    on_repetitions: $ => /'[0-9,\-]+/,

    // S-expression (Lisp-like): (tempo 120), (vol 80)
    sexp: $ => seq(
      '(',
      repeat($._sexp_item),
      ')',
    ),

    _sexp_item: $ => choice(
      $.sexp_symbol,
      $.sexp_number,
      $.string,
      $.quoted_list,
      $.sexp,  // nested
    ),

    sexp_symbol: $ => /[a-zA-Z_!?+\-*/<>=.:][a-zA-Z0-9_!?+\-*/<>=.:]*/,

    sexp_number: $ => /-?[0-9]+(\.[0-9]+)?/,

    quoted_list: $ => seq("'", $.sexp),

    // Strings: "hello"
    string: $ => /"[^"]*"/,

    // Identifiers
    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_\-]*/,
  },
});
