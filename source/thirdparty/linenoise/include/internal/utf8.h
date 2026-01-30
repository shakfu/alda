/* utf8.h -- UTF-8 encoding/decoding and grapheme cluster handling.
 *
 * This module provides UTF-8 utilities for linenoise, including:
 * - Byte length detection
 * - Codepoint decoding (forward and backward)
 * - Grapheme cluster navigation (handles emoji, ZWJ sequences, etc.)
 * - Display width calculation for terminal positioning
 */

#ifndef LINENOISE_UTF8_H
#define LINENOISE_UTF8_H

#include <stddef.h>
#include <stdint.h>

/* Return the number of bytes that compose the UTF-8 character starting at 'c'.
 * Returns 1-4 for valid UTF-8, or 1 for invalid sequences. */
int utf8_byte_len(char c);

/* Decode a UTF-8 sequence starting at 's' into a Unicode codepoint.
 * Sets *len to the number of bytes consumed. Returns the codepoint value. */
uint32_t utf8_decode(const char *s, size_t *len);

/* Decode the UTF-8 codepoint ending at position 'pos' (exclusive).
 * Sets *cplen to the byte length. Returns the codepoint value. */
uint32_t utf8_decode_prev(const char *buf, size_t pos, size_t *cplen);

/* Return the display width of a Unicode codepoint (0, 1, or 2 columns). */
int utf8_codepoint_width(uint32_t cp);

/* Calculate the display width of a UTF-8 string of 'len' bytes.
 * Handles grapheme clusters (ZWJ sequences count as single width). */
size_t utf8_str_width(const char *s, size_t len);

/* Return the display width of a single UTF-8 character at position 's'. */
int utf8_single_char_width(const char *s, size_t len);

/* Return the byte length of the grapheme cluster at position 'pos'.
 * A grapheme cluster includes base char + combining marks + ZWJ sequences. */
size_t utf8_next_grapheme_len(const char *buf, size_t pos, size_t len);

/* Return the byte length of the grapheme cluster before position 'pos'. */
size_t utf8_prev_grapheme_len(const char *buf, size_t pos);

/* Codepoint classification functions. */
int utf8_is_variation_selector(uint32_t cp);
int utf8_is_skin_tone_modifier(uint32_t cp);
int utf8_is_zwj(uint32_t cp);
int utf8_is_regional_indicator(uint32_t cp);
int utf8_is_combining_mark(uint32_t cp);
int utf8_is_grapheme_extend(uint32_t cp);

#endif /* LINENOISE_UTF8_H */
