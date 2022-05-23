// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <cairo.h>
#include <ctype.h>
#include <glib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <pango/pangocairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sway-client-helpers/log.h>
#include <sway-client-helpers/loop.h>
#include <sway-client-helpers/util.h>
#include <sys/wait.h>
#include <unistd.h>
#include "menu.h"
#include "trappist.h"

/* state-machine variables for processing <item></item> */
static bool in_item;
static struct menuitem *current_item;

static int menu_level;
static struct menu *current_menu;

/* vector for <menu id="" label=""> elements */
static struct menu *menus;
static int nr_menus, alloc_menus;

static char *
nodename(xmlNode *node, char *buf, int len)
{
	if (!node || !node->name) {
		return NULL;
	}

	/* Ignore superflous 'text.' in node name */
	if (node->parent && !strcmp((char *)node->name, "text")) {
		node = node->parent;
	}

	char *p = buf;
	p[--len] = 0;
	for (;;) {
		const char *name = (char *)node->name;
		char c;
		while ((c = *name++) != 0) {
			*p++ = tolower(c);
			if (!--len) {
				return buf;
			}
		}
		*p = 0;
		node = node->parent;
		if (!node || !node->name) {
			return buf;
		}
		*p++ = '.';
		if (!--len) {
			return buf;
		}
	}
}

static void
string_truncate_at_pattern(char *buf, const char *pattern)
{
	char *p = strstr(buf, pattern);
	if (!p) {
		return;
	}
	*p = '\0';
}

static struct menu *
menu_create(const char *id, const char *label)
{
	if (nr_menus == alloc_menus) {
		alloc_menus = (alloc_menus + 16) * 2;
		menus = realloc(menus, alloc_menus * sizeof(struct menu));
	}
	struct menu *menu = menus + nr_menus;
	memset(menu, 0, sizeof(*menu));
	nr_menus++;
	wl_list_init(&menu->menuitems);
	menu->id = strdup(id);
	menu->label = strdup(label);
	menu->parent = current_menu;
	return menu;
}

static struct menu *
get_menu_by_id(const char *id)
{
	struct menu *menu;
	for (int i = 0; i < nr_menus; ++i) {
		menu = menus + i;
		if (!strcmp(menu->id, id)) {
			return menu;
		}
	}
	return NULL;
}

static struct menuitem *
item_create(struct menu *menu, const char *label)
{
	struct menuitem *menuitem = calloc(1, sizeof(struct menuitem));
	if (!menuitem) {
		return NULL;
	}
	menuitem->label = strdup(label);
	menuitem->box.width = MENU_ITEM_WIDTH;
	menuitem->box.height = MENU_ITEM_HEIGHT;
	menuitem->selectable = true;
	wl_list_insert(&menu->menuitems, &menuitem->link);
	return menuitem;
}

static struct menuitem *
separator_create(struct menu *menu, const char *label)
{
	struct menuitem *menuitem = calloc(1, sizeof(struct menuitem));
	if (!menuitem) {
		return NULL;
	}
	menuitem->box.width = MENU_ITEM_WIDTH;
	menuitem->box.height = 5;
	menuitem->selectable = false;
	wl_list_insert(&menu->menuitems, &menuitem->link);
	return menuitem;
}

/*
 * Handle the following:
 * <item label="" icon="">
 *   <action name="">
 *     <command></command>
 *   </action>
 * </item>
 */
static void
fill_item(char *nodename, char *content)
{
	string_truncate_at_pattern(nodename, ".item.menu");

	/* <item label=""> defines the start of a new item */
	if (!strcmp(nodename, "label")) {
		current_item = item_create(current_menu, content);
	}
	if (!current_item) {
		LOG(LOG_ERROR, "expect <item label=\"\"> element first");
		return;
	}
	if (!strcmp(nodename, "name.action")) {
		current_item->action = strdup(content);
	} else if (!strcmp(nodename, "command.action")
			|| !strcmp(nodename, "execute.action")) {
		current_item->command = strdup(content);
	} else if (!strcmp(nodename, "icon")) {
		current_item->icon = strdup(content);
	}
}

static void
entry(xmlNode *node, char *nodename, char *content)
{
	if (!nodename || !content) {
		return;
	}
	string_truncate_at_pattern(nodename, ".openbox_menu");
	if (in_item) {
		fill_item(nodename, content);
	}
}

static void
process_node(xmlNode *node)
{
	static char buffer[256];

	char *content = (char *)node->content;
	if (xmlIsBlankNode(node)) {
		return;
	}
	char *name = nodename(node, buffer, sizeof(buffer));
	entry(node, name, content);
}

static void xml_tree_walk(xmlNode *node);

static void
traverse(xmlNode *n)
{
	process_node(n);
	for (xmlAttr * attr = n->properties; attr; attr = attr->next) {
		xml_tree_walk(attr->children);
	}
	xml_tree_walk(n->children);
}

/*
 * <menu> elements have three different roles:
 *  * Definition of (sub)menu - has ID, LABEL and CONTENT
 *  * Menuitem of pipemenu type - has EXECUTE and LABEL
 *  * Menuitem of submenu type - has ID only
 */
static void
handle_menu_element(xmlNode *n)
{
	char *label = (char *)xmlGetProp(n, (const xmlChar *)"label");
	char *execute = (char *)xmlGetProp(n, (const xmlChar *)"execute");
	char *id = (char *)xmlGetProp(n, (const xmlChar *)"id");

	if (execute) {
		LOG(LOG_ERROR, "we do not support pipemenus - yet");
	} else if (label && id) {
		struct menu **submenu = NULL;
		if (menu_level > 0) {
			/*
			 * In a nested (inline) menu definition we need to
			 * create an item pointing to the new submenu
			 */
			current_item = item_create(current_menu, label);
			submenu = &current_item->submenu;
		}
		++menu_level;
		current_menu = menu_create(id, label);
		if (submenu) {
			*submenu = current_menu;
		}
		traverse(n);
		current_menu = current_menu->parent;
		--menu_level;
	} else if (id) {
		struct menu *menu = get_menu_by_id(id);
		if (menu) {
			current_item = item_create(current_menu, menu->label);
			current_item->submenu = menu;
		} else {
			LOG(LOG_ERROR, "no menu with id '%s'", id);
		}
	}
	if (label) {
		free(label);
	}
	if (execute) {
		free(execute);
	}
	if (id) {
		free(id);
	}
}

/* This can be one of <separator> and <separator label=""> */
static void
handle_separator_element(xmlNode *n)
{
	char *label = (char *)xmlGetProp(n, (const xmlChar *)"label");
	current_item = separator_create(current_menu, label);
	if (label) {
		free(label);
	}
}

static void
xml_tree_walk(xmlNode *node)
{
	for (xmlNode *n = node; n && n->name; n = n->next) {
		if (!strcasecmp((char *)n->name, "comment")) {
			continue;
		}
		if (!strcasecmp((char *)n->name, "menu")) {
			handle_menu_element(n);
			continue;
		}
		if (!strcasecmp((char *)n->name, "separator")) {
			handle_separator_element(n);
			continue;
		}
		if (!strcasecmp((char *)n->name, "item")) {
			in_item = true;
			traverse(n);
			in_item = false;
			continue;
		}
		traverse(n);
	}
}

static void
parse_xml(const char *filename)
{
	if (!filename) {
		LOG(LOG_INFO, "no menu.xml specified");
		return;
	}
	xmlDoc * d = xmlReadFile(filename, NULL, 0);
	if (!d) {
		LOG(LOG_ERROR, "xmlReadFile()");
		return;
	}
	xml_tree_walk(xmlDocGetRootElement(d));
	xmlFreeDoc(d);
	xmlCleanupParser();
}

static void
menu_configure(struct menu *menu, int x, int y)
{
	menu->box.x = x;
	menu->box.y = y;

	int menu_overlap_x = 0;
	int menu_overlap_y = 0;

	int menu_width = MENU_ITEM_WIDTH + 2 * MENU_PADDING_X;

	int offset = 0;
	struct menuitem *menuitem;
	wl_list_for_each_reverse(menuitem, &menu->menuitems, link) {
		menuitem->box.x = menu->box.x + MENU_PADDING_X;
		menuitem->box.y = menu->box.y + MENU_PADDING_Y + offset;
		offset += menuitem->box.height;
		if (menuitem->submenu) {
			menu_configure(menuitem->submenu, menuitem->box.x
				+ menu_width - menu_overlap_x,
				menuitem->box.y - MENU_PADDING_Y
				+ menu_overlap_y);
		}
	}

	menu->box.width = menu_width;
	menu->box.height = offset + MENU_PADDING_Y * 2;
}

static struct menuitem *
first_selectable_menuitem(struct state *state)
{
	struct menu *menu = state->menu;
	for (int i = 0; i < nr_menus; ++i) {
		menu = menus + i;
		if (!menu->visible) {
			continue;
		}
		struct menuitem *item;
		return wl_container_of(menu->menuitems.prev, item, link);
	}
	return NULL;
}

static void
generate_pixmaps(struct menu *menu)
{
	struct menuitem *item;
	wl_list_for_each (item, &menu->menuitems, link) {
		pixmap_pair_create(item);
		if (item->submenu) {
			generate_pixmaps(item->submenu);
		}
	}
}

void
menu_init(struct state *state, const char *filename)
{
	parse_xml(filename);
	state->menu = get_menu_by_id("root-menu");

	/* Default menu if no menu.xml found */
	if (!state->menu) {
		state->menu = menu_create("root-menu", "");
	}
	if (wl_list_empty(&state->menu->menuitems)) {
		current_item = item_create(state->menu, "foo");
		current_item->action = strdup("");
		current_item = item_create(state->menu, "bar");
		current_item->action = strdup("");
	}

	struct menu *menu = state->menu;
	for (int i = 0; i < nr_menus; ++i) {
		menu = menus + i;
		menu->state = state;
	}

	state->menu->visible = true;
	state->selection = first_selectable_menuitem(state);
	generate_pixmaps(state->menu);
	menu_move(state->menu, MENU_X, MENU_Y);
}

void
menu_finish(struct state *state)
{
	struct menu *menu = state->menu;
	for (int i = 0; i < nr_menus; ++i) {
		menu = menus + i;
		struct menuitem *item, *next;
		wl_list_for_each_safe(item, next, &menu->menuitems, link) {
			if (item->label) {
				free(item->label);
			}
			if (item->action) {
				free(item->action);
			}
			if (item->command) {
				free(item->command);
			}
			if (item->icon) {
				free(item->icon);
			}
			cairo_surface_destroy(item->pixmap.active);
			cairo_surface_destroy(item->pixmap.inactive);
			wl_list_remove(&item->link);
			free(item);
		}
		free(menu->id);
		free(menu->label);
	}
	if (menus) {
		free(menus);
	}
	alloc_menus = 0;
	nr_menus = 0;
}

static void
close_all_submenus(struct menu *menu)
{
	struct menuitem *item;
	wl_list_for_each (item, &menu->menuitems, link) {
		if (item->submenu) {
			item->submenu->visible = false;
			close_all_submenus(item->submenu);
		}
	}
}

void
menu_move(struct menu *menu, int x, int y)
{
	assert(menu);
	close_all_submenus(menu);
	menu_configure(menu, x, y);
	surface_damage(menu->state->surface);
}

static struct menu *
menu_from_item(struct state *state, struct menuitem *menuitem)
{
	struct menu *menu = state->menu;
	for (int i = 0; i < nr_menus; ++i) {
		menu = menus + i;
		struct menuitem *item;
		wl_list_for_each (item, &menu->menuitems, link) {
			if (item == menuitem) {
				return menu;
			}
		}
	}
	return NULL;
}

static void
open_submenu(struct state *state, struct menuitem *item)
{
	assert(item && item->submenu);
	struct menu *menu = menu_from_item(state, item);
	assert(menu);
	close_all_submenus(menu);
	item->submenu->visible = true;
}

static void
timer_hover_clear(void *data)
{
	struct state *state = data;
	state->hover_timer = NULL;

	if (state->selection->submenu) {
		open_submenu(state, state->selection);
	}
	surface_damage(state->surface);
}

static void
timer_hover_start(struct state *state)
{
	if (state->hover_timer) {
		loop_remove_timer(state->eventloop, state->hover_timer);
	}
	state->hover_timer = loop_add_timer(state->eventloop,
		TRAPPIST_SUBMENU_SHOW_DELAY, timer_hover_clear, state);
}

static bool
box_empty(const struct box *box)
{
	return !box || box->width <= 0 || box->height <= 0;
}

static bool
box_contains_point(const struct box *box, double x, double y)
{
	if (box_empty(box)) {
		return false;
	}
	return x >= box->x && x < box->x + box->width
		&& y >= box->y && y < box->y + box->height;
}

void
menu_handle_cursor_motion(struct menu *menu, int x, int y)
{
	if (!menu->visible) {
		return;
	}
	struct menuitem *item;
	wl_list_for_each (item, &menu->menuitems, link) {
		if (!box_contains_point(&item->box, x, y)) {
			/* Iterate over open submenus */
			if (item->submenu && item->submenu->visible) {
				menu_handle_cursor_motion(item->submenu, x, y);
			}
			continue;
		}
		if (!item->selectable) {
			continue;
		}
		if (!item->submenu) {
			/* Cursor is over an ordinary (not submenu) item */
			close_all_submenus(menu);
			menu->state->selection = item;
			break;
		} else if (!item->submenu->visible) {
			/*
			 * Cursor is over a new (not visible yet) submenu,
			 * so let's just set the selection and wait for the
			 * hover-timer to timeout
			 */
			menu->state->selection = item;
			break;
		}
	}
	timer_hover_start(menu->state);
	surface_damage(menu->state->surface);
}

void
menu_handle_button_pressed(struct state *state, int x, int y)
{
	struct menu *menu;
	for (int i = 0; i < nr_menus; ++i) {
		menu = menus + i;
		if (!menu->visible) {
			continue;
		}
		if (box_contains_point(&menu->box, (double)x, (double)y)) {
			return;
		}
	}
	state->run_display = false;
}

static void
spawn_async_no_shell(char const *command)
{
	GError *err = NULL;
	gchar **argv = NULL;

	if (!command) {
		return;
	}
	g_shell_parse_argv((gchar *)command, NULL, &argv, &err);
	if (err) {
		g_message("%s", err->message);
		g_error_free(err);
		return;
	}

	/*
	 * Avoid zombie processes by using a double-fork, whereby the
	 * grandchild becomes orphaned & the responsibility of the OS.
	 */
	pid_t child = 0, grandchild = 0;

	child = fork();
	switch (child) {
	case -1:
		LOG(LOG_ERROR, "unable to fork()");
		goto out;
	case 0:
		setsid();
		sigset_t set;
		sigemptyset(&set);
		sigprocmask(SIG_SETMASK, &set, NULL);
		grandchild = fork();
		if (grandchild == 0) {
			execvp(argv[0], argv);
			_exit(0);
		} else if (grandchild < 0) {
			LOG(LOG_ERROR, "unable to fork()");
		}
		_exit(0);
	default:
		break;
	}
	waitpid(child, NULL, 0);
out:
	g_strfreev(argv);
}

void
menu_handle_button_released(struct state *state, int x, int y)
{
	spawn_async_no_shell(state->selection->command);
	state->run_display = false;
}

enum trappist_direction {
	DIRECTION_UP,
	DIRECTION_DOWN,
};

static void
move_selection(struct state *state, enum trappist_direction direction)
{
	struct menu *menu = menu_from_item(state, state->selection);
	assert(menu);
	do {
		struct menuitem *item = state->selection;
		struct wl_list *list = direction == DIRECTION_UP
			? state->selection->link.next
			: state->selection->link.prev;
		state->selection = wl_container_of(list, item, link);
	} while (&state->selection->link == &menu->menuitems
			|| !state->selection->selectable);
	if (state->selection->submenu) {
		open_submenu(state, state->selection);
	} else {
		close_all_submenus(menu);
	}
}

struct menuitem *
parent_of(struct state *state, struct menuitem *menuitem)
{
	struct menu *child = menu_from_item(state, menuitem);
	for (int i = 0; i < nr_menus; ++i) {
		struct menu *menu = menus + i;
		struct menuitem *item;
		wl_list_for_each (item, &menu->menuitems, link) {
			if (item->submenu == child) {
				return item;
			}
		}
	}
	return menuitem;
}

struct menuitem *
child_of(struct state *state, struct menuitem *menuitem)
{
	struct menuitem *item;
	struct wl_list *list = menuitem->submenu->menuitems.prev;
	return wl_container_of(list, item, link);
}

void
menu_handle_key(struct state *state, xkb_keysym_t keysym, uint32_t codepoint)
{
	if (!state->selection) {
		state->selection = first_selectable_menuitem(state);
	}

	switch (keysym) {
	case XKB_KEY_Up:
		move_selection(state, DIRECTION_UP);
		break;
	case XKB_KEY_Down:
		move_selection(state, DIRECTION_DOWN);
		break;
	case XKB_KEY_Right:
		if (state->selection->submenu) {
			state->selection = child_of(state, state->selection);
		}
		break;
	case XKB_KEY_Left:
		state->selection = parent_of(state, state->selection);
		break;
	case XKB_KEY_KP_Enter:
	case XKB_KEY_Return:
		spawn_async_no_shell(state->selection->command);
		state->run_display = false;
		break;
	case XKB_KEY_BackSpace:
		search_remove_last_uft8_character();
		break;
	case XKB_KEY_Escape:
		state->run_display = false;
		break;
	default:
		if (!codepoint) {
			break;
		}
		search_add_utf8_character(codepoint);
		break;
	}
	surface_damage(state->surface);
}
