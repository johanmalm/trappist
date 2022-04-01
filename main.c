// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <getopt.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sway-client-helpers/log.h>
#include <sway-client-helpers/loop.h>
#include "trappist.h"
#include "wlr-input-inhibitor-unstable-v1-client-protocol.h"

static struct state state = { 0 };

static void
display_in(int fd, short mask, void *data)
{
	if (wl_display_dispatch(state.display) == -1) {
		state.run_display = false;
	}
}

#define DIE_ON(condition, message) do { \
	if ((condition) != 0) { \
		LOG(LOG_ERROR, message); \
		exit(EXIT_FAILURE); \
	} \
} while (0)

int
main(int argc, char *argv[])
{
	log_init(LOG_DEBUG);

	wl_list_init(&state.outputs);

	state.display = wl_display_connect(NULL);
	DIE_ON(!state.display, "unable to connect to compositor");

	/* Register global interfaces */
	globals_init(&state);
	wl_display_roundtrip(state.display);
	DIE_ON(!state.compositor, "no compositor");
	DIE_ON(!state.shm, "no shm");
	DIE_ON(!state.seat, "no seat");
	DIE_ON(!state.layer_shell, "no layer-shell");
	DIE_ON(!state.input_inhibit_manager, "no input-inhibit-manager");

	state.seat->xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	zwlr_input_inhibit_manager_v1_get_inhibitor(state.input_inhibit_manager);
	if (wl_display_roundtrip(state.display) == -1) {
		LOG(LOG_ERROR, "failed to inhibit input");
		exit(EXIT_FAILURE);
	}

	state.seat->cursor_surface =
		wl_compositor_create_surface(state.compositor);

	/*
	 * TODO: Consider creating one surface per output to avoid cursor
	 * button on other outputs
	 */
	state.surface = calloc(1, sizeof(struct surface));
	state.surface->state = &state;
	state.surface->surface = wl_compositor_create_surface(state.compositor);

	struct output *output;
	wl_list_for_each(output, &state.outputs, link) {
		/* TODO: Allow selection of output */
		state.surface->wl_output = output->wl_output;
		break;
	}
	LOG(LOG_INFO, "using output '%s'", output->name);

	surface_layer_surface_create(state.surface);

	state.eventloop = loop_create();
	loop_add_fd(state.eventloop, wl_display_get_fd(state.display), POLLIN,
		display_in, NULL);

	state.run_display = true;
	while (state.run_display) {
		errno = 0;
		if (wl_display_flush(state.display) == -1 && errno != EAGAIN) {
			break;
		}
		loop_poll(state.eventloop);
	}

	surface_destroy(state.surface);
	if (state.seat->cursor_theme) {
		wl_cursor_theme_destroy(state.seat->cursor_theme);
	}
	pango_cairo_font_map_set_default(NULL);
	return 0;
}
