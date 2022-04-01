// SPDX-License-Identifier: GPL-2.0-only
#include <assert.h>
#include <stdlib.h>
#include <sway-client-helpers/log.h>
#include <sway-client-helpers/loop.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>
#include "trappist.h"
#include "menu.h"

/*
 * Rigged up using references:
 *   - https://wayland.freedesktop.org/docs/html/apa.html
 *   - https://wayland-book.com/seat/example.html
 */

static void
handle_wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t format, int32_t fd, uint32_t size)
{
	struct seat *seat = data;
	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		close(fd);
		LOG(LOG_ERROR, "unknown keymap format %d", format);
		exit(EXIT_FAILURE);
	}
	char *map_shm = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map_shm == MAP_FAILED) {
		close(fd);
		LOG(LOG_ERROR, "unable to initialize keymap shm");
		exit(EXIT_FAILURE);
	}
	struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_string(
		seat->xkb.context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1,
		XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap(map_shm, size);
	close(fd);

	struct xkb_state *xkb_state = xkb_state_new(xkb_keymap);
	xkb_keymap_unref(seat->xkb.keymap);
	xkb_state_unref(seat->xkb.state);
	seat->xkb.keymap = xkb_keymap;
	seat->xkb.state = xkb_state;
}

static void
handle_wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, struct wl_surface *surface,
		struct wl_array *keys)
{
	/* nop */
}

static void
handle_wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, struct wl_surface *surface)
{
	/* nop */
}

static void
keyboard_repeat(void *data)
{
	struct seat *seat = data;
	struct state *state = seat->state;
	seat->repeat_timer = loop_add_timer(state->eventloop,
		seat->repeat_period_ms, keyboard_repeat, seat);
	key_handle(state, seat->repeat_sym, seat->repeat_codepoint);
}

static void
handle_wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, uint32_t time, uint32_t key, uint32_t _state)
{
	struct seat *seat = data;
	struct state *state = seat->state;
	enum wl_keyboard_key_state key_state = _state;
	xkb_keysym_t sym = xkb_state_key_get_one_sym(seat->xkb.state, key + 8);
	uint32_t keycode = key_state == WL_KEYBOARD_KEY_STATE_PRESSED ?
		key + 8 : 0;
	uint32_t codepoint = xkb_state_key_get_utf32(seat->xkb.state, keycode);
	if (key_state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		key_handle(state, sym, codepoint);
	}

	if (seat->repeat_timer) {
		loop_remove_timer(seat->state->eventloop, seat->repeat_timer);
		seat->repeat_timer = NULL;
	}
	bool pressed = key_state == WL_KEYBOARD_KEY_STATE_PRESSED;
	if (pressed && seat->repeat_period_ms > 0) {
		seat->repeat_sym = sym;
		seat->repeat_codepoint = codepoint;
		seat->repeat_timer = loop_add_timer(seat->state->eventloop,
			seat->repeat_delay_ms, keyboard_repeat, seat);
	}
}

static void
handle_wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard,
		uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
		uint32_t mods_locked, uint32_t group)
{
	struct seat *seat = data;
	if (!seat->xkb.state) {
		return;
	}
	xkb_state_update_mask(seat->xkb.state, mods_depressed, mods_latched,
		mods_locked, 0, 0, group);
}

static void
handle_wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
		int32_t rate, int32_t delay)
{
	struct seat *seat = data;
	if (rate <= 0) {
		seat->repeat_period_ms = -1;
	} else {
		seat->repeat_period_ms = 1000 / rate;
	}
	seat->repeat_delay_ms = delay;
}

static const struct wl_keyboard_listener keyboard_listener = {
	.keymap = handle_wl_keyboard_keymap,
	.enter = handle_wl_keyboard_enter,
	.leave = handle_wl_keyboard_leave,
	.key = handle_wl_keyboard_key,
	.modifiers = handle_wl_keyboard_modifiers,
	.repeat_info = handle_wl_keyboard_repeat_info,
};

static void
update_cursor(struct seat *seat, uint32_t serial)
{
	struct state *state = seat->state;
	seat->cursor_theme =
		wl_cursor_theme_load(getenv("XCURSOR_THEME"), 24, state->shm);
	struct wl_cursor *cursor =
		wl_cursor_theme_get_cursor(seat->cursor_theme, "left_ptr");
	struct wl_cursor_image *cursor_image = cursor->images[0];
	wl_surface_set_buffer_scale(seat->cursor_surface, 1);
	wl_surface_attach(seat->cursor_surface,
		wl_cursor_image_get_buffer(cursor_image), 0, 0);
	wl_pointer_set_cursor(seat->pointer, serial, seat->cursor_surface,
		cursor_image->hotspot_x, cursor_image->hotspot_y);
	wl_surface_damage_buffer(seat->cursor_surface, 0, 0,
		INT32_MAX, INT32_MAX);
	wl_surface_commit(seat->cursor_surface);
}

static void
handle_wl_pointer_enter(void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface,
		wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	struct seat *seat = data;
	seat->pointer_event.event_mask |= POINTER_EVENT_ENTER;
	seat->pointer_event.serial = serial;
	seat->pointer_event.surface_x = surface_x;
	seat->pointer_event.surface_y = surface_y;
	update_cursor(seat, serial);
}

static void
handle_wl_pointer_leave(void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface)
{
	struct seat *seat = data;
	seat->pointer_event.serial = serial;
	seat->pointer_event.event_mask |= POINTER_EVENT_LEAVE;
}

static void
handle_wl_pointer_motion(void *data, struct wl_pointer *wl_pointer,
		uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	struct seat *seat = data;
	seat->pointer_event.event_mask |= POINTER_EVENT_MOTION;
	seat->pointer_event.time = time;
	seat->pointer_event.surface_x = surface_x;
	seat->pointer_event.surface_y = surface_y;
}

static void
handle_wl_pointer_button(void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	struct seat *seat = data;
	seat->pointer_event.event_mask |= POINTER_EVENT_BUTTON;
	seat->pointer_event.time = time;
	seat->pointer_event.serial = serial;
	seat->pointer_event.button = button;
	seat->pointer_event.state = state;
}

static void
handle_wl_pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time,
		uint32_t axis, wl_fixed_t value)
{
	struct seat *seat = data;
	seat->pointer_event.event_mask |= POINTER_EVENT_AXIS;
	seat->pointer_event.time = time;
	seat->pointer_event.axes[axis].valid = true;
	seat->pointer_event.axes[axis].value = value;
}

static void
handle_wl_pointer_axis_source(void *data, struct wl_pointer *wl_pointer,
		uint32_t axis_source)
{
	struct seat *seat = data;
	seat->pointer_event.event_mask |= POINTER_EVENT_AXIS_SOURCE;
	seat->pointer_event.axis_source = axis_source;
}

static void
handle_wl_pointer_axis_stop(void *data, struct wl_pointer *wl_pointer,
		uint32_t time, uint32_t axis)
{
	struct seat *seat = data;
	seat->pointer_event.time = time;
	seat->pointer_event.event_mask |= POINTER_EVENT_AXIS_STOP;
	seat->pointer_event.axes[axis].valid = true;
}

static void
handle_wl_pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer,
		uint32_t axis, int32_t discrete)
{
	struct seat *seat = data;
	seat->pointer_event.event_mask |= POINTER_EVENT_AXIS_DISCRETE;
	seat->pointer_event.axes[axis].valid = true;
	seat->pointer_event.axes[axis].discrete = discrete;
}

static void
handle_wl_pointer_frame(void *data, struct wl_pointer *wl_pointer)
{
	struct seat *seat = data;
	struct pointer_event *event = &seat->pointer_event;

	if (event->event_mask & POINTER_EVENT_MOTION) {
		seat->pointer_x = wl_fixed_to_int(event->surface_x);
		seat->pointer_y = wl_fixed_to_int(event->surface_y);
		menu_handle_cursor_motion(seat->state->menu,
			seat->pointer_x, seat->pointer_y);
	}

	if (event->event_mask & POINTER_EVENT_BUTTON) {
		int x = seat->pointer_x;
		int y = seat->pointer_y;
		switch (event->state) {
		case WL_POINTER_BUTTON_STATE_PRESSED:
			menu_handle_button_pressed(seat->state, x, y);
			break;
		case WL_POINTER_BUTTON_STATE_RELEASED:
			menu_handle_button_released(seat->state, x, y);
			break;
		default:
		}
	}
	memset(event, 0, sizeof(struct pointer_event));
	surface_damage(seat->state->surface);
}

static const struct wl_pointer_listener pointer_listener = {
	.enter = handle_wl_pointer_enter,
	.leave = handle_wl_pointer_leave,
	.motion = handle_wl_pointer_motion,
	.button = handle_wl_pointer_button,
	.axis = handle_wl_pointer_axis,
	.frame = handle_wl_pointer_frame,
	.axis_source = handle_wl_pointer_axis_source,
	.axis_stop = handle_wl_pointer_axis_stop,
	.axis_discrete = handle_wl_pointer_axis_discrete,
};

static void
handle_wl_seat_capabilities(void *data, struct wl_seat *wl_seat,
		enum wl_seat_capability caps)
{
	struct seat *seat = data;
	if (seat->pointer) {
		wl_pointer_release(seat->pointer);
		seat->pointer = NULL;
	}
	if (seat->keyboard) {
		wl_keyboard_release(seat->keyboard);
		seat->keyboard = NULL;
	}
	if ((caps & WL_SEAT_CAPABILITY_POINTER)) {
		seat->pointer = wl_seat_get_pointer(wl_seat);
		wl_pointer_add_listener(seat->pointer, &pointer_listener, seat);
	}
	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
		seat->keyboard = wl_seat_get_keyboard(wl_seat);
		wl_keyboard_add_listener(seat->keyboard,
			&keyboard_listener, seat);
	}
}

static void
handle_wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
	/* nop */
}

const struct wl_seat_listener seat_listener = {
	.capabilities = handle_wl_seat_capabilities,
	.name = handle_wl_seat_name,
};

void
seat_init(struct state *state, struct wl_seat *wl_seat)
{
	struct seat *seat = calloc(1, sizeof(struct seat));
	seat->state = state;
	state->seat = seat;
	wl_seat_add_listener(wl_seat, &seat_listener, seat);
}
