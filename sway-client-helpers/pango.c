/*
 * Copied from https://github.com/swaywm/sway/blob/master/common/pango.c
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

#include <cairo.h>
#include <pango/pangocairo.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sway-client-helpers/cairo_util.h"
#include "sway-client-helpers/log.h"

static PangoLayout *
get_pango_layout(cairo_t *cairo, const char *font, const char *text, double scale)
{
	PangoLayout *layout = pango_cairo_create_layout(cairo);
	PangoAttrList *attrs;
	attrs = pango_attr_list_new();
	pango_layout_set_text(layout, text, -1);

	pango_attr_list_insert(attrs, pango_attr_scale_new(scale));
	PangoFontDescription *desc = pango_font_description_from_string(font);
	pango_layout_set_font_description(layout, desc);
	pango_layout_set_single_paragraph_mode(layout, 1);
	pango_layout_set_attributes(layout, attrs);
	pango_attr_list_unref(attrs);
	pango_font_description_free(desc);
	return layout;
}

void get_text_size(cairo_t *cairo, const char *font, int *width, int *height,
		int *baseline, double scale, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	// Add one since vsnprintf excludes null terminator.
	int length = vsnprintf(NULL, 0, fmt, args) + 1;
	va_end(args);

	char *buf = malloc(length);
	if (buf == NULL) {
		LOG(LOG_ERROR, "Failed to allocate memory");
		return;
	}
	va_start(args, fmt);
	vsnprintf(buf, length, fmt, args);
	va_end(args);

	PangoLayout *layout = get_pango_layout(cairo, font, buf, scale);
	pango_cairo_update_layout(cairo, layout);
	pango_layout_get_pixel_size(layout, width, height);
	if (baseline) {
		*baseline = pango_layout_get_baseline(layout) / PANGO_SCALE;
	}
	g_object_unref(layout);
	free(buf);
}

void get_text_metrics(const char *font, int *height, int *baseline) {
	cairo_t *cairo = cairo_create(NULL);
	PangoContext *pango = pango_cairo_create_context(cairo);
	PangoFontDescription *description = pango_font_description_from_string(font);
	// When passing NULL as a language, pango uses the current locale.
	PangoFontMetrics *metrics = pango_context_get_metrics(pango, description, NULL);

	*baseline = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
	*height = *baseline + pango_font_metrics_get_descent(metrics) / PANGO_SCALE;

	pango_font_metrics_unref(metrics);
	pango_font_description_free(description);
	g_object_unref(pango);
	cairo_destroy(cairo);
}

void
render_text(cairo_t *cairo, const char *font, double scale, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	// Add one since vsnprintf excludes null terminator.
	int length = vsnprintf(NULL, 0, fmt, args) + 1;
	va_end(args);

	char *buf = malloc(length);
	if (buf == NULL) {
		LOG(LOG_ERROR, "Failed to allocate memory");
		return;
	}
	va_start(args, fmt);
	vsnprintf(buf, length, fmt, args);
	va_end(args);

	PangoLayout *layout = get_pango_layout(cairo, font, buf, scale);
	cairo_font_options_t *fo = cairo_font_options_create();
	cairo_get_font_options(cairo, fo);
	pango_cairo_context_set_font_options(pango_layout_get_context(layout), fo);
	cairo_font_options_destroy(fo);
	pango_cairo_update_layout(cairo, layout);
	pango_cairo_show_layout(cairo, layout);
	g_object_unref(layout);
	free(buf);
}
