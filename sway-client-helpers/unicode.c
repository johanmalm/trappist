/*
 * Copied from https://github.com/swaylock/unicode.c
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

#include <stdint.h>
#include <stddef.h>
#include "sway-client-helpers/unicode.h"

size_t utf8_chsize(uint32_t ch) {
	if (ch < 0x80) {
		return 1;
	} else if (ch < 0x800) {
		return 2;
	} else if (ch < 0x10000) {
		return 3;
	}
	return 4;
}

size_t utf8_encode(char *str, uint32_t ch) {
	size_t len = 0;
	uint8_t first;

	if (ch < 0x80) {
		first = 0;
		len = 1;
	} else if (ch < 0x800) {
		first = 0xc0;
		len = 2;
	} else if (ch < 0x10000) {
		first = 0xe0;
		len = 3;
	} else {
		first = 0xf0;
		len = 4;
	}

	for (size_t i = len - 1; i > 0; --i) {
		str[i] = (ch & 0x3f) | 0x80;
		ch >>= 6;
	}

	str[0] = ch | first;
	return len;
}


static const struct {
	uint8_t mask;
	uint8_t result;
	int octets;
} sizes[] = {
	{ 0x80, 0x00, 1 },
	{ 0xE0, 0xC0, 2 },
	{ 0xF0, 0xE0, 3 },
	{ 0xF8, 0xF0, 4 },
	{ 0xFC, 0xF8, 5 },
	{ 0xFE, 0xF8, 6 },
	{ 0x80, 0x80, -1 },
};

int utf8_size(const char *s) {
	uint8_t c = (uint8_t)*s;
	for (size_t i = 0; i < sizeof(sizes) / sizeof(*sizes); ++i) {
		if ((c & sizes[i].mask) == sizes[i].result) {
			return sizes[i].octets;
		}
	}
	return -1;
}
