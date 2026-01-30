/**
 * Tree-sitter grammar for Csound CSD files
 * Token-based grammar for syntax highlighting
 * https://csound.com/
 */

module.exports = grammar({
  name: 'csound',

  extras: $ => [
    /[ \t]+/,
  ],

  rules: {
    source_file: $ => repeat($._token),

    _token: $ => choice(
      $.xml_tag,
      $.header_var,
      $.instrument_keyword,
      $.opcode_keyword,
      $.block_keyword,
      $.control_keyword,
      $.type_keyword,
      $.comment,
      $.block_comment,
      $.pfield,
      $.variable,
      $.number,
      $.string,
      $.operator,
      $.identifier,
      /\n/,
      /./,
    ),

    // XML section tags
    xml_tag: $ => /<\/?Csound(Synthesizer|Options|Instruments|Score)>/,

    // Header variables: sr, ksmps, nchnls, 0dbfs, A4
    header_var: $ => choice(
      'sr', 'kr', 'ksmps', 'nchnls', 'nchnls_i', '0dbfs', 'A4',
    ),

    // Instrument/opcode definition keywords
    instrument_keyword: $ => 'instr',
    opcode_keyword: $ => 'opcode',

    // Block end keywords
    block_keyword: $ => choice(
      'endin', 'endop',
      'xin', 'xout',
    ),

    // Control flow keywords
    control_keyword: $ => choice(
      'if', 'then', 'ithen', 'kthen', 'elseif', 'else', 'endif', 'fi',
      'while', 'do', 'od', 'endwhile',
      'until', 'enduntil',
      'goto', 'igoto', 'kgoto', 'tigoto', 'cigoto',
    ),

    // Type keywords
    type_keyword: $ => 'init',

    // Variables: ivar, kvar, avar, givar, gkvar, gaout, etc.
    variable: $ => /g?[ikafw][a-zA-Z_][a-zA-Z0-9_]*/,

    // P-fields: p1, p2, p3, p4, etc.
    pfield: $ => /p[0-9]+/,

    // Operators
    operator: $ => choice(
      '=',
      '+', '-', '*', '/', '%', '^',
      '==', '!=', '<=', '>=', '<', '>',
      '&&', '||', '&', '|', '#', '~', '!',
      '(', ')', '[', ']', '{', '}',
      ',', ':', '?',
    ),

    // Literals
    number: $ => /-?[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)?/,

    string: $ => /"[^"]*"/,

    // Identifiers (opcodes, labels, etc.)
    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    // Comments
    comment: $ => /;[^\n]*/,
    block_comment: $ => seq('/*', /[^*]*\*+([^/*][^*]*\*+)*/, '/'),
  },
});
