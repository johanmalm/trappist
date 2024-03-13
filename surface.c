// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include "trappist.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

static const struct zwlr_layer_surface_v1_listener layer_surface_listener;

static void
layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *layer_surface,
		uint32_t serial, uint32_t width, uint32_t height)
{
	struct surface *surface = data;
	surface->width = width;
	surface->height = height;
	zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
	render_frame(surface);
}

static void
layer_surface_closed(void *data, struct zwlr_layer_surface_v1 *layer_surface)
{
	struct surface *surface = data;
	surface_destroy(surface);
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = layer_surface_closed,
};

void
surface_layer_surface_create(struct surface *surface)
{
	struct state *state = surface->state;

	assert(surface->surface);
	surface->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
		state->layer_shell, surface->surface, surface->wl_output,
		ZWLR_LAYER_SHELL_V1_LAYER_TOP, "trappist");
	assert(surface->layer_surface);

	zwlr_layer_surface_v1_set_size(surface->layer_surface, 0, 0);
	zwlr_layer_surface_v1_set_anchor(surface->layer_surface,
			ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
			ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
			ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
			ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
	zwlr_layer_surface_v1_set_exclusive_zone(surface->layer_surface, -1);
	zwlr_layer_surface_v1_set_keyboard_interactivity(surface->layer_surface,
		ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND);
	zwlr_layer_surface_v1_add_listener(surface->layer_surface,
			&layer_surface_listener, surface);
	wl_surface_commit(surface->surface);
}

static const struct wl_callback_listener surface_frame_listener;

static void
surface_frame_done(void *data, struct wl_callback *callback, uint32_t time)
{
	struct surface *surface = data;

	wl_callback_destroy(callback);
	surface->frame_pending = false;
	if (!surface->dirty) {
		return;
	}
	struct wl_callback *_callback = wl_surface_frame(surface->surface);
	wl_callback_add_listener(_callback, &surface_frame_listener, surface);
	surface->frame_pending = true;
	render_frame(surface);
	surface->dirty = false;
}

static const struct wl_callback_listener surface_frame_listener = {
	.done = surface_frame_done,
};

bool
surface_is_configured(struct surface *surface)
{
	return (surface->width && surface->height);
}

void
surface_damage(struct surface *surface)
{
	if (!surface_is_configured(surface)) {
		return;
	}
	surface->dirty = true;
	if (surface->frame_pending) {
		return;
	}
	struct wl_callback *callback = wl_surface_frame(surface->surface);
	wl_callback_add_listener(callback, &surface_frame_listener, surface);
	surface->frame_pending = true;
	wl_surface_commit(surface->surface);
}

void
surface_destroy(struct surface *surface)
{
	if (surface->layer_surface) {
		zwlr_layer_surface_v1_destroy(surface->layer_surface);
	}
	if (surface->surface) {
		wl_surface_destroy(surface->surface);
	}
	destroy_buffer(&surface->buffers[0]);
	destroy_buffer(&surface->buffers[1]);
	free(surface);
}
