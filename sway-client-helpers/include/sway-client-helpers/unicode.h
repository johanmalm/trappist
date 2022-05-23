/*
 * Copied from https://github.com/swaylock/include/unicode.h
 *
 * Copyright (C) 2016-2019 Drew DeVault
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _SWAY_UNICODE_H
#define _SWAY_UNICODE_H
#include <stddef.h>
#include <stdint.h>

// Technically UTF-8 supports up to 6 byte codepoints, but Unicode itself
// doesn't really bother with more than 4.
#define UTF8_MAX_SIZE 4

#define UTF8_INVALID 0x80

/**
 * Grabs the next UTF-8 character and advances the string pointer
 */
uint32_t utf8_decode(const char **str);

/**
 * Encodes a character as UTF-8 and returns the length of that character.
 */
size_t utf8_encode(char *str, uint32_t ch);

/**
 * Returns the size of the next UTF-8 character
 */
int utf8_size(const char *str);

/**
 * Returns the size of a UTF-8 character
 */
size_t utf8_chsize(uint32_t ch);

#endif

