/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef TRAPPIST_MENU_H
#define TRAPPIST_MENU_H
#include <cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <wayland-server.h>

#define MENU_X (1)
#define MENU_Y (30)
#define MENU_ITEM_WIDTH (170)
#define MENU_ITEM_HEIGHT (30)
#define MENU_ITEM_PADDING_X (4)
#define MENU_ITEM_PADDING_Y (4)
#define MENU_PADDING_X (1)
#define MENU_PADDING_Y (1)
#define MENU_FONT ("Sans 9")
#define COLOR_MENU_BG (0x000000FF)
#define COLOR_MENU_BORDER (0x222222FF)
#define COLOR_ITEM_ACTIVE_BG (0x333333FF)
#define COLOR_ITEM_ACTIVE_FG (0xDDDDDDFF)
#define COLOR_ITEM_INACTIVE_BG (0x00000000)
#define COLOR_ITEM_INACTIVE_FG (0xDDDDDDFF)
#define COLOR_SEPARATOR_FG (0x444444FF)
#define TRAPPIST_SUBMENU_SHOW_DELAY (100)

struct state;
struct conf;

struct box {
	int x;
	int y;
	int width;
	int height;
};

struct menuitem {
	char *label;
	char *action;
	char *command;
	char *icon;
	struct menu *submenu;
	struct box box;
	bool selectable;
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
	bool right_aligned;
	bool bottom_aligned;
	struct wl_list menuitems;

	struct state *state;
};

void menu_init(struct state *state, struct conf *conf, const char *filename);
void menu_finish(struct state *state);
void pixmap_pair_create(struct menuitem *item, struct conf *conf);
void menu_move(struct menu *menu, int x, int y);
void menu_handle_cursor_motion(struct menu *menu, int x, int y);
void menu_handle_button_pressed(struct state *state, int x, int y);
void menu_handle_button_released(struct state *state, int x, int y);
void menu_handle_key(struct state *state, xkb_keysym_t keysym,
	uint32_t codepoint);

#endif /* TRAPPIST_MENU_H */
