// SPDX-License-Identifier: GPL-2.0-only
#include <cairo.h>
#include <stdint.h>
#include <stdlib.h>
#include <sway-client-helpers/util.h>
#include "trappist.h"

static void
draw(cairo_t *cairo, struct state *state)
{
	/* Clear background */
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	set_source_u32(cairo, 0xFF000033);
	cairo_paint(cairo);
	cairo_restore(cairo);

	/* Configure */
	cairo_set_line_width(cairo, 2.0);
	static int x = 5, y = 5, w = 200, h = 200, padding = 5;

	/* Black background */
	set_source_u32(cairo, 0x000000FF);
	cairo_rectangle(cairo, x, y, w, h);
	cairo_fill(cairo);

	/* Draw label */
	char *text = *search_str() ? search_str() : "&lt;type to search&gt;";

	PangoLayout *layout;
	PangoFontDescription *font;
	layout = pango_cairo_create_layout(cairo);
	font = pango_font_description_from_string("monospace 10");
	pango_layout_set_width(layout, (w - padding * 2) * PANGO_SCALE);
	pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_font_description(layout, font);
	pango_layout_set_markup(layout, text, -1);

	int height;
	pango_layout_get_pixel_size(layout, NULL, &height);
	cairo_move_to(cairo, x + padding, y + (h - height) / 2);

	set_source_u32(cairo, 0xFFFFFFFF);
	pango_cairo_update_layout(cairo, layout);
	pango_cairo_show_layout(cairo, layout);
	pango_font_description_free(font);
	g_object_unref(layout);

	/* Draw red border on hover time-out */
	if (state->hover_timer) {
		set_source_u32(cairo, 0xFF0000FF);
		cairo_rectangle(cairo, x, y, w, h);
		cairo_stroke(cairo);
	}
}

void
render_frame(struct surface *surface)
{
	struct state *state = surface->state;

	if (!surface_is_configured(surface)) {
		return;
	}
	surface->current_buffer = get_next_buffer(state->shm,
		surface->buffers, surface->width, surface->height);
	if (!surface->current_buffer) {
		return;
	}

	cairo_t *cairo = surface->current_buffer->cairo;
	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);
	cairo_identity_matrix(cairo);

	draw(cairo, state);

	/* https://wayland-book.com/surfaces/shared-memory.html */
	wl_surface_attach(surface->surface,
		surface->current_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(surface->surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(surface->surface);
}
