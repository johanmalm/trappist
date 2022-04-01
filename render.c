// SPDX-License-Identifier: GPL-2.0-only
#include <cairo.h>
#include <stdint.h>
#include <stdlib.h>
#include <sway-client-helpers/util.h>
#include "menu.h"
#include "trappist.h"

static void
draw_rect(cairo_t *cairo, struct box *box, uint32_t color)
{
	cairo_save(cairo);
	set_source_u32(cairo, color);
	cairo_rectangle(cairo, box->x, box->y, box->width, box->height);
	cairo_fill(cairo);
	cairo_restore(cairo);
}

static void
draw_pixmap(cairo_t *cairo, cairo_surface_t *pixmap, struct box *box)
{
	cairo_save(cairo);
	cairo_set_source_surface(cairo, pixmap, box->x, box->y);
	cairo_paint_with_alpha(cairo, 1.0);
	cairo_restore(cairo);
}

static void
draw_menu(cairo_t *cairo, struct menu *menu)
{
	struct menuitem *menuitem;
	draw_rect(cairo, &menu->box, COLOR_MENU_BG);
	wl_list_for_each (menuitem, &menu->menuitems, link) {
		cairo_surface_t *pixmap;
		uint32_t color_item_bg;

		if (menuitem != menu->state->selection) {
			pixmap = menuitem->pixmap.inactive;
			color_item_bg = COLOR_ITEM_BG_INACTIVE;
		} else {
			pixmap = menuitem->pixmap.active;
			color_item_bg = COLOR_ITEM_BG_ACTIVE;
		}
		draw_rect(cairo, &menuitem->box, color_item_bg);
		draw_pixmap(cairo, pixmap, &menuitem->box);

		if (menuitem->submenu && menuitem->submenu->visible) {
			draw_menu(cairo, menuitem->submenu);
		}
	}
}

static void
draw(cairo_t *cairo, struct state *state)
{
	/* Clear background */
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	set_source_u32(cairo, 0x00000000);
	cairo_paint(cairo);
	cairo_restore(cairo);

	/* Search Box */
	static int w = 200, h = 20, padding = 5;

	/* Black background */
	set_source_u32(cairo, COLOR_ITEM_BG_INACTIVE);
	cairo_rectangle(cairo, 0, 0, w, h);
	cairo_fill(cairo);

	char *text = *search_str() ? search_str() : "&lt;type to search&gt;";
	PangoLayout *layout;
	layout = pango_cairo_create_layout(cairo);

	PangoFontDescription *font = pango_font_description_new();
	pango_font_description_set_family(font, MENU_FONT_NAME);
	pango_font_description_set_size(font, MENU_FONT_SIZE * PANGO_SCALE);

	pango_layout_set_width(layout, (w - padding * 2) * PANGO_SCALE);
	pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_font_description(layout, font);
	pango_layout_set_markup(layout, text, -1);
	set_source_u32(cairo, COLOR_ITEM_FG_INACTIVE);
	pango_cairo_update_layout(cairo, layout);
	pango_cairo_show_layout(cairo, layout);
	pango_font_description_free(font);
	g_object_unref(layout);

	/* Draw red border on hover time-out */
	if (state->hover_timer) {
		set_source_u32(cairo, COLOR_ITEM_FG_ACTIVE);
		cairo_rectangle(cairo, 0, 0, w, h);
		cairo_stroke(cairo);
	}

	draw_menu(cairo, state->menu);
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
