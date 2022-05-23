/*
 * Copied from https://github.com/swaywm/sway/blob/master/include/pango.h
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

#ifndef _SWAY_PANGO_H
#define _SWAY_PANGO_H
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <cairo.h>
#include <pango/pangocairo.h>

void get_text_size(cairo_t *cairo, const char *font, int *width, int *height,
		int *baseline, double scale, const char *fmt, ...);
void get_text_metrics(const char *font, int *height, int *baseline);
void render_text(cairo_t *cairo, const char *font,
		double scale, const char *fmt, ...);

#endif
