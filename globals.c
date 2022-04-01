// SPDX-License-Identifier: GPL-2.0-only
#include <sway-client-helpers/log.h>
#include "trappist.h"
#include "wlr-input-inhibitor-unstable-v1-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

static void
handle_wl_registry_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	/* https://wayland-book.com/registry/binding.html */

	struct state *state = data;
	if (!strcmp(interface, wl_compositor_interface.name)) {
		/* https://wayland-book.com/surfaces/compositor.html */
		state->compositor = wl_registry_bind(registry, name,
				&wl_compositor_interface, 4);
	} else if (!strcmp(interface, wl_shm_interface.name)) {
		/* https://wayland-book.com/surfaces/shared-memory.html */
		state->shm = wl_registry_bind(registry, name,
				&wl_shm_interface, 1);
	} else if (!strcmp(interface, wl_seat_interface.name)) {
		struct wl_seat *wl_seat = wl_registry_bind(registry, name,
				&wl_seat_interface, 7);
		seat_init(state, wl_seat);
	} else if (!strcmp(interface, zwlr_layer_shell_v1_interface.name)) {
		state->layer_shell = wl_registry_bind(
			registry, name, &zwlr_layer_shell_v1_interface, 4);
	} else if (!strcmp(interface,
			zwlr_input_inhibit_manager_v1_interface.name)) {
		state->input_inhibit_manager = wl_registry_bind(registry, name,
				&zwlr_input_inhibit_manager_v1_interface, 1);
	} else if (!strcmp(interface, wl_output_interface.name)) {
		struct wl_output *wl_output = wl_registry_bind(registry, name,
				&wl_output_interface, 4);
		output_init(state, wl_output);
	}
}

static void
handle_wl_registry_global_remove(void *data, struct wl_registry *registry,
		uint32_t name)
{
	/* nop */
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_wl_registry_global,
	.global_remove = handle_wl_registry_global_remove,
};

void
globals_init(struct state *state)
{
	struct wl_registry *registry = wl_display_get_registry(state->display);
	wl_registry_add_listener(registry, &registry_listener, state);
}
