// SPDX-License-Identifier: GPL-2.0-only
#include <cairo.h>
#include <stdint.h>
#include <stdlib.h>
#include <sway-client-helpers/util.h>
#include "menu.h"
#include "trappist.h"

static void
draw_rect(cairo_t *cairo, struct box *box, uint32_t color, bool fill)
{
	cairo_save(cairo);
	set_source_u32(cairo, color);
	cairo_rectangle(cairo, box->x, box->y, box->width, box->height);
	if (fill) {
		cairo_fill(cairo);
	} else {
		cairo_stroke(cairo);
	}
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
	if (!menu->visible) {
		return;
	}

	/* background */
	draw_rect(cairo, &menu->box, COLOR_MENU_BG, true);

	/* border */
	draw_rect(cairo, &menu->box, COLOR_MENU_BORDER, false);

	struct menuitem *menuitem;
	wl_list_for_each (menuitem, &menu->menuitems, link) {
		cairo_surface_t *pixmap;
		uint32_t color_item_bg;

		if (menuitem != menu->state->selection) {
			pixmap = menuitem->pixmap.inactive;
			color_item_bg = COLOR_ITEM_INACTIVE_BG;
		} else {
			pixmap = menuitem->pixmap.active;
			color_item_bg = COLOR_ITEM_ACTIVE_BG;
		}
		draw_rect(cairo, &menuitem->box, color_item_bg, true);
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

	draw_menu(cairo, state->menu);
}

void
render_frame(struct surface *surface)
{
	struct state *state = surface->state;

	if (!surface_is_configured(surface)) {
		return;
	}
	struct pool_buffer *buffer = get_next_buffer(state->shm,
		surface->buffers, surface->width, surface->height);
	if (!buffer) {
		return;
	}

	cairo_t *cairo = buffer->cairo;
	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);
	cairo_identity_matrix(cairo);

	draw(cairo, state);

	/* https://wayland-book.com/surfaces/shared-memory.html */
	wl_surface_attach(surface->surface, buffer->buffer, 0, 0);
	wl_surface_damage_buffer(surface->surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(surface->surface);
}
