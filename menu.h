/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _TRAPPIST_MENU_H
#define _TRAPPIST_MENU_H
#include <cairo.h>
#include <wayland-server.h>

#define MENU_X (1)
#define MENU_Y (25)
#define MENU_ITEM_WIDTH (110)
#define MENU_ITEM_HEIGHT (30)
#define MENU_ITEM_PADDING_X (4)
#define MENU_PADDING_X (1)
#define MENU_PADDING_Y (1)
#define MENU_FONT_NAME ("monospace")
#define MENU_FONT_SIZE (10)
#define COLOR_MENU_BG (0xCCCCCCFF)
#define COLOR_ITEM_BG_ACTIVE (0xCCCCCCFF)
#define COLOR_ITEM_BG_INACTIVE (0x222222FF)
#define COLOR_ITEM_FG_ACTIVE (0xFF0000FF)
#define COLOR_ITEM_FG_INACTIVE (0x00FFFFFF)

struct state;

struct box {
	int x;
	int y;
	int width;
	int height;
};

struct menuitem {
	char *action;
	char *command;
	struct menu *submenu;
	struct box box;
	struct {
		cairo_surface_t *active;
		cairo_surface_t *inactive;
	} pixmap;
	struct wl_list link; /* menu::menuitems */
};

/* This could be the root-menu or a submenu */
struct menu {
	char *id;
	char *label;
	bool visible;
	struct menu *parent;
	struct box box;
	struct wl_list menuitems;

	struct state *state;
};

void menu_init(struct state *state, const char *filename);
void menu_finish(struct state *state);
void menu_move(struct menu *menu, int x, int y);
void menu_handle_cursor_motion(struct menu *menu, int x, int y);
void menu_handle_button_pressed(struct state *state, int x, int y);
void menu_handle_button_released(struct state *state, int x, int y);

#endif /* _TRAPPIST_MENU_H */
