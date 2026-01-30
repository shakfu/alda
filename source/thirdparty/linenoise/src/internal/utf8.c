/* utf8.c -- UTF-8 encoding/decoding and grapheme cluster handling.
 *
 * See utf8.h for API documentation.
 */

#include "utf8.h"

/* Return the number of bytes that compose the UTF-8 character starting at 'c'.
 * This function assumes a valid UTF-8 encoding and handles the four
 * standard byte patterns:
 *   0xxxxxxx -> 1 byte (ASCII)
 *   110xxxxx -> 2 bytes
 *   1110xxxx -> 3 bytes
 *   11110xxx -> 4 bytes */
int utf8_byte_len(char c) {
    unsigned char uc = (unsigned char)c;
    if ((uc & 0x80) == 0)    return 1;   /* 0xxxxxxx: ASCII */
    if ((uc & 0xE0) == 0xC0) return 2;   /* 110xxxxx: 2-byte seq */
    if ((uc & 0xF0) == 0xE0) return 3;   /* 1110xxxx: 3-byte seq */
    if ((uc & 0xF8) == 0xF0) return 4;   /* 11110xxx: 4-byte seq */
    return 1; /* Fallback for invalid encoding, treat as single byte. */
}

/* Decode a UTF-8 sequence starting at 's' into a Unicode codepoint.
 * Returns the codepoint value. Assumes valid UTF-8 encoding. */
uint32_t utf8_decode(const char *s, size_t *len) {
    unsigned char *p = (unsigned char *)s;
    uint32_t cp;

    if ((*p & 0x80) == 0) {
        *len = 1;
        return *p;
    } else if ((*p & 0xE0) == 0xC0) {
        *len = 2;
        cp = (*p & 0x1F) << 6;
        cp |= (p[1] & 0x3F);
        return cp;
    } else if ((*p & 0xF0) == 0xE0) {
        *len = 3;
        cp = (*p & 0x0F) << 12;
        cp |= (p[1] & 0x3F) << 6;
        cp |= (p[2] & 0x3F);
        return cp;
    } else if ((*p & 0xF8) == 0xF0) {
        *len = 4;
        cp = (*p & 0x07) << 18;
        cp |= (p[1] & 0x3F) << 12;
        cp |= (p[2] & 0x3F) << 6;
        cp |= (p[3] & 0x3F);
        return cp;
    }
    *len = 1;
    return *p; /* Fallback for invalid sequences. */
}

/* Check if codepoint is a variation selector (emoji style modifiers). */
int utf8_is_variation_selector(uint32_t cp) {
    return cp == 0xFE0E || cp == 0xFE0F;  /* Text/emoji style */
}

/* Check if codepoint is a skin tone modifier. */
int utf8_is_skin_tone_modifier(uint32_t cp) {
    return cp >= 0x1F3FB && cp <= 0x1F3FF;
}

/* Check if codepoint is Zero Width Joiner. */
int utf8_is_zwj(uint32_t cp) {
    return cp == 0x200D;
}

/* Check if codepoint is a Regional Indicator (for flag emoji). */
int utf8_is_regional_indicator(uint32_t cp) {
    return cp >= 0x1F1E6 && cp <= 0x1F1FF;
}

/* Check if codepoint is a combining mark or other zero-width character. */
int utf8_is_combining_mark(uint32_t cp) {
    return (cp >= 0x0300 && cp <= 0x036F) ||   /* Combining Diacriticals */
           (cp >= 0x1AB0 && cp <= 0x1AFF) ||   /* Combining Diacriticals Extended */
           (cp >= 0x1DC0 && cp <= 0x1DFF) ||   /* Combining Diacriticals Supplement */
           (cp >= 0x20D0 && cp <= 0x20FF) ||   /* Combining Diacriticals for Symbols */
           (cp >= 0xFE20 && cp <= 0xFE2F);     /* Combining Half Marks */
}

/* Check if codepoint extends the previous character (doesn't start a new grapheme). */
int utf8_is_grapheme_extend(uint32_t cp) {
    return utf8_is_variation_selector(cp) || utf8_is_skin_tone_modifier(cp) ||
           utf8_is_zwj(cp) || utf8_is_combining_mark(cp);
}

/* Decode the UTF-8 codepoint ending at position 'pos' (exclusive) and
 * return its value. Also sets *cplen to the byte length of the codepoint. */
uint32_t utf8_decode_prev(const char *buf, size_t pos, size_t *cplen) {
    if (pos == 0) {
        *cplen = 0;
        return 0;
    }
    /* Scan backwards to find the start byte. */
    size_t i = pos;
    do {
        i--;
    } while (i > 0 && (pos - i) < 4 && ((unsigned char)buf[i] & 0xC0) == 0x80);
    *cplen = pos - i;
    size_t dummy;
    return utf8_decode(buf + i, &dummy);
}

/* Given a buffer and a position, return the byte length of the grapheme
 * cluster before that position. A grapheme cluster includes:
 * - The base character
 * - Any following variation selectors, skin tone modifiers
 * - ZWJ sequences (emoji joined by Zero Width Joiner)
 * - Regional indicator pairs (flag emoji) */
size_t utf8_prev_grapheme_len(const char *buf, size_t pos) {
    if (pos == 0) return 0;

    size_t total = 0;
    size_t curpos = pos;

    /* First, get the last codepoint. */
    size_t cplen;
    uint32_t cp = utf8_decode_prev(buf, curpos, &cplen);
    if (cplen == 0) return 0;
    total += cplen;
    curpos -= cplen;

    /* If we're at an extending character, we need to find what it extends.
     * Keep going back through the grapheme cluster. */
    while (curpos > 0) {
        size_t prevlen;
        uint32_t prevcp = utf8_decode_prev(buf, curpos, &prevlen);
        if (prevlen == 0) break;

        if (utf8_is_zwj(prevcp)) {
            /* ZWJ joins two emoji. Include the ZWJ and continue to get
             * the preceding character. */
            total += prevlen;
            curpos -= prevlen;
            /* Now get the character before ZWJ. */
            prevcp = utf8_decode_prev(buf, curpos, &prevlen);
            if (prevlen == 0) break;
            total += prevlen;
            curpos -= prevlen;
            cp = prevcp;
            continue;  /* Check if there's more extending before this. */
        } else if (utf8_is_grapheme_extend(cp)) {
            /* Current cp is an extending character; include previous. */
            total += prevlen;
            curpos -= prevlen;
            cp = prevcp;
            continue;
        } else if (utf8_is_regional_indicator(cp) && utf8_is_regional_indicator(prevcp)) {
            /* Two regional indicators form a flag. But we need to be careful:
             * flags are always pairs, so only join if we're at an even boundary.
             * For simplicity, just join one pair. */
            total += prevlen;
            curpos -= prevlen;
            break;
        } else {
            /* No more extending; we've found the start of the cluster. */
            break;
        }
    }

    return total;
}

/* Given a buffer, position and total length, return the byte length of the
 * grapheme cluster at the current position. */
size_t utf8_next_grapheme_len(const char *buf, size_t pos, size_t len) {
    if (pos >= len) return 0;

    size_t total = 0;
    size_t curpos = pos;

    /* Get the first codepoint. */
    size_t cplen;
    uint32_t cp = utf8_decode(buf + curpos, &cplen);
    total += cplen;
    curpos += cplen;

    int isRI = utf8_is_regional_indicator(cp);

    /* Consume any extending characters that follow. */
    while (curpos < len) {
        size_t nextlen;
        uint32_t nextcp = utf8_decode(buf + curpos, &nextlen);

        if (utf8_is_zwj(nextcp) && curpos + nextlen < len) {
            /* ZWJ: include it and the following character. */
            total += nextlen;
            curpos += nextlen;
            /* Get the character after ZWJ. */
            nextcp = utf8_decode(buf + curpos, &nextlen);
            total += nextlen;
            curpos += nextlen;
            continue;  /* Check for more extending after the joined char. */
        } else if (utf8_is_grapheme_extend(nextcp)) {
            /* Variation selector, skin tone, combining mark, etc. */
            total += nextlen;
            curpos += nextlen;
            continue;
        } else if (isRI && utf8_is_regional_indicator(nextcp)) {
            /* Second regional indicator for a flag pair. */
            total += nextlen;
            curpos += nextlen;
            isRI = 0;  /* Only pair once. */
            continue;
        } else {
            break;
        }
    }

    return total;
}

/* Return the display width of a Unicode codepoint. This is a heuristic
 * that works for most common cases:
 * - Control chars and zero-width: 0 columns
 * - Grapheme-extending chars (VS, skin tone, ZWJ): 0 columns
 * - ASCII printable: 1 column
 * - Wide chars (CJK, emoji, fullwidth): 2 columns
 * - Everything else: 1 column
 *
 * This is not a full wcwidth() implementation, but a minimal heuristic
 * that handles emoji and CJK characters reasonably well. */
int utf8_codepoint_width(uint32_t cp) {
    /* Control characters and combining marks: zero width. */
    if (cp < 32 || (cp >= 0x7F && cp < 0xA0)) return 0;
    if (utf8_is_combining_mark(cp)) return 0;

    /* Grapheme-extending characters: zero width.
     * These modify the preceding character rather than taking space. */
    if (utf8_is_variation_selector(cp)) return 0;
    if (utf8_is_skin_tone_modifier(cp)) return 0;
    if (utf8_is_zwj(cp)) return 0;

    /* Wide character ranges - these display as 2 columns:
     * - CJK Unified Ideographs and Extensions
     * - Fullwidth forms
     * - Various emoji ranges */
    if (cp >= 0x1100 &&
        (cp <= 0x115F ||                      /* Hangul Jamo */
         cp == 0x2329 || cp == 0x232A ||      /* Angle brackets */
         (cp >= 0x231A && cp <= 0x231B) ||    /* Watch, Hourglass */
         (cp >= 0x23E9 && cp <= 0x23F3) ||    /* Various symbols */
         (cp >= 0x23F8 && cp <= 0x23FA) ||    /* Various symbols */
         (cp >= 0x25AA && cp <= 0x25AB) ||    /* Small squares */
         (cp >= 0x25B6 && cp <= 0x25C0) ||    /* Play/reverse buttons */
         (cp >= 0x25FB && cp <= 0x25FE) ||    /* Squares */
         (cp >= 0x2600 && cp <= 0x26FF) ||    /* Misc Symbols (sun, cloud, etc) */
         (cp >= 0x2700 && cp <= 0x27BF) ||    /* Dingbats */
         (cp >= 0x2934 && cp <= 0x2935) ||    /* Arrows */
         (cp >= 0x2B05 && cp <= 0x2B07) ||    /* Arrows */
         (cp >= 0x2B1B && cp <= 0x2B1C) ||    /* Squares */
         cp == 0x2B50 || cp == 0x2B55 ||      /* Star, circle */
         (cp >= 0x2E80 && cp <= 0xA4CF &&
          cp != 0x303F) ||                    /* CJK ... Yi */
         (cp >= 0xAC00 && cp <= 0xD7A3) ||    /* Hangul Syllables */
         (cp >= 0xF900 && cp <= 0xFAFF) ||    /* CJK Compatibility Ideographs */
         (cp >= 0xFE10 && cp <= 0xFE1F) ||    /* Vertical forms */
         (cp >= 0xFE30 && cp <= 0xFE6F) ||    /* CJK Compatibility Forms */
         (cp >= 0xFF00 && cp <= 0xFF60) ||    /* Fullwidth Forms */
         (cp >= 0xFFE0 && cp <= 0xFFE6) ||    /* Fullwidth Signs */
         (cp >= 0x1F1E6 && cp <= 0x1F1FF) ||  /* Regional Indicators (flags) */
         (cp >= 0x1F300 && cp <= 0x1F64F) ||  /* Misc Symbols and Emoticons */
         (cp >= 0x1F680 && cp <= 0x1F6FF) ||  /* Transport and Map Symbols */
         (cp >= 0x1F900 && cp <= 0x1F9FF) ||  /* Supplemental Symbols */
         (cp >= 0x1FA00 && cp <= 0x1FAFF) ||  /* Chess, Extended-A */
         (cp >= 0x20000 && cp <= 0x2FFFF)))   /* CJK Extension B and beyond */
        return 2;

    return 1; /* Default: single width */
}

/* Calculate the display width of a UTF-8 string of 'len' bytes.
 * This is used for cursor positioning in the terminal.
 * Handles grapheme clusters: characters joined by ZWJ contribute 0 width
 * after the first character in the sequence. */
size_t utf8_str_width(const char *s, size_t len) {
    size_t width = 0;
    size_t i = 0;
    int after_zwj = 0;  /* Track if previous char was ZWJ */

    while (i < len) {
        size_t clen;
        uint32_t cp = utf8_decode(s + i, &clen);

        if (after_zwj) {
            /* Character after ZWJ: don't add width, it's joined.
             * But do check for extending chars after it. */
            after_zwj = 0;
        } else {
            width += utf8_codepoint_width(cp);
        }

        /* Check if this is a ZWJ - next char will be joined. */
        if (utf8_is_zwj(cp)) {
            after_zwj = 1;
        }

        i += clen;
    }
    return width;
}

/* Return the display width of a single UTF-8 character at position 's'. */
int utf8_single_char_width(const char *s, size_t len) {
    if (len == 0) return 0;
    size_t clen;
    uint32_t cp = utf8_decode(s, &clen);
    return utf8_codepoint_width(cp);
}
