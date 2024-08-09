// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <cairo.h>
#include <ctype.h>
#include <glib.h>
#include <librsvg/rsvg.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pango/pangocairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sway-client-helpers/log.h>
#include <sway-client-helpers/loop.h>
#include <sway-client-helpers/pango.h>
#include <sway-client-helpers/util.h>
#include <sys/wait.h>
#include <unistd.h>
#include "conf.h"
#include "menu.h"
#include "trappist.h"

static void
add_svg(cairo_t *cairo, const char *filename, int icon_size)
{
	GError *err = NULL;
	RsvgRectangle viewport = { .width = icon_size, .height = icon_size };
	RsvgHandle *svg = rsvg_handle_new_from_file(filename, &err);
	if (err) {
		LOG(LOG_DEBUG, "error reading svg %s-%s", filename, err->message);
		g_error_free(err);
		return;
	}

	cairo_surface_t *image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
		icon_size, icon_size);
	cairo_t *cr = cairo_create(image);

	rsvg_handle_render_document(svg, cr, &viewport, &err);
	if (err) {
		LOG(LOG_ERROR, "error rendering svg %s-%s\n", filename, err->message);
		g_error_free(err);
		goto error;
	}

	if (cairo_surface_status(image)) {
		LOG(LOG_ERROR, "error reading svg button '%s'", filename);
		goto error;
	}
	cairo_surface_flush(image);

	cairo_translate(cairo, MENU_ITEM_PADDING_X, MENU_ITEM_PADDING_Y);

	double w = cairo_image_surface_get_width(image);
	double h = cairo_image_surface_get_height(image);
	double x = (icon_size - w) / 2.0;
	double y = (icon_size - h) / 2.0;
	cairo_set_source_surface(cairo, image, x, y);
	cairo_paint_with_alpha(cairo, 1.0);

error:
	cairo_destroy(cr);
	cairo_surface_destroy(image);
	g_object_unref(svg);
}

static void
add_png(cairo_t *cairo, const char *filename, int icon_size)
{
	cairo_surface_t *image = cairo_image_surface_create_from_png(filename);
	if (cairo_surface_status(image)) {
		cairo_surface_destroy(image);
		LOG(LOG_ERROR, "bad png icon (%s)", filename);
		return;
	}

	cairo_translate(cairo, MENU_ITEM_PADDING_X, MENU_ITEM_PADDING_Y);

	/* TODO: use cairo_image_surface_scale() */
	double w, h, max;
	w = cairo_image_surface_get_width(image);
	h = cairo_image_surface_get_height(image);
	max = h > w ? h : w;
	if (max != icon_size) {
		cairo_scale(cairo, icon_size / max, icon_size / max);
	}
	cairo_set_source_surface(cairo, image, 0, 0);
	cairo_paint_with_alpha(cairo, 1.0);
	cairo_surface_destroy(image);
}

bool
ends_with(const char *string, const char *ending)
{
	size_t pos = strlen(string) - strlen(ending);
	return strcmp(string+pos, ending) == 0;
}

static void
render_menu_entry(cairo_surface_t **pixmap, struct menuitem *item,
		struct conf *conf, uint32_t color)
{
	if (!item || !item->label || !*item->label) {
		return;
	}
	cairo_t *cairo = cairo_create(*pixmap);

	int scale = 1.0;
//	int icon_size = item->box.height - 2 * MENU_ITEM_PADDING_Y;

	int font_height, font_baseline;
	get_text_metrics(MENU_FONT, &font_height, &font_baseline);
	int offset_y = (MENU_ITEM_HEIGHT - font_height) / 2;

	set_source_u32(cairo, color);

	cairo_move_to(cairo, conf->icon.size + MENU_ITEM_PADDING_X * 2, offset_y);
	render_text(cairo, MENU_FONT, scale, item->label);

	if (item->submenu) {
		cairo_move_to(cairo, MENU_ITEM_WIDTH - 10, offset_y);
		render_text(cairo, MENU_FONT, scale, "›");
	}

	if (item->icon && *item->icon) {
		if (ends_with(item->icon, ".png")) {
			add_png(cairo, item->icon, conf->icon.size);
		} else if (ends_with(item->icon, ".svg")) {
			add_svg(cairo, item->icon, conf->icon.size);
		}
	}

	cairo_destroy(cairo);
}

static void
render_separator(cairo_surface_t **pixmap)
{
	cairo_t *cairo = cairo_create(*pixmap);
	set_source_u32(cairo, COLOR_SEPARATOR_FG);
	cairo_set_line_width(cairo, 1.0);
	cairo_move_to(cairo, 3.0, 2.5);
	cairo_line_to(cairo, MENU_ITEM_WIDTH - 3.0, 2.5);
	cairo_stroke(cairo);
	cairo_destroy(cairo);
}

static void
pixmap_create(cairo_surface_t **pixmap, struct box *box)
{
	if (*pixmap) {
		cairo_surface_destroy(*pixmap);
		*pixmap = NULL;
	}
	*pixmap = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
			box->width, box->height);
}

void
pixmap_pair_create(struct menuitem *item, struct conf *conf)
{
	pixmap_create(&item->pixmap.active, &item->box);
	pixmap_create(&item->pixmap.inactive, &item->box);

	if (item->selectable) {
		render_menu_entry(&item->pixmap.active, item, conf,
			COLOR_ITEM_ACTIVE_FG);
		render_menu_entry(&item->pixmap.inactive, item, conf,
			COLOR_ITEM_INACTIVE_FG);
	} else {
		render_separator(&item->pixmap.inactive);
		render_separator(&item->pixmap.active);
	}
}
