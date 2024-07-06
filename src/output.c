// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <sway-client-helpers/log.h>
#include "trappist.h"

static void
handle_wl_output_geometry(void *data, struct wl_output *wl_output,
		int32_t x, int32_t y, int32_t physical_width,
		int32_t physical_height, int32_t subpixel, const char *make,
		const char *model, int32_t transform)
{
	struct output *output = data;
	output->subpixel = subpixel;
	if (output->state->run_display) {
		surface_damage(output->state->surface);
	}
}

static void
handle_wl_output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
		int32_t width, int32_t height, int32_t refresh)
{
	/* nop */
}

static void
handle_wl_output_done(void *data, struct wl_output *output)
{
	/* nop */
}

static void
handle_wl_output_scale(void *data, struct wl_output *wl_output, int32_t factor)
{
	struct output *output = data;
	output->scale = factor;
	if (output->state->run_display) {
		surface_damage(output->state->surface);
	}
}

static void
handle_wl_output_name(void *data, struct wl_output *wl_output, const char *name)
{
	struct output *output = data;
	output->name = strdup(name);
}

static void
handle_wl_output_description(void *data, struct wl_output *wl_output,
		const char *description)
{
	/* nop */
}

static struct wl_output_listener output_listener = {
	.geometry = handle_wl_output_geometry,
	.mode = handle_wl_output_mode,
	.done = handle_wl_output_done,
	.scale = handle_wl_output_scale,
	.name = handle_wl_output_name,
	.description = handle_wl_output_description,
};

void
output_init(struct state *state, struct wl_output *wl_output)
{
	struct output *output = calloc(1, sizeof(struct output));
	output->state = state;
	output->wl_output = wl_output;
	output->scale = 1;
	wl_output_add_listener(output->wl_output, &output_listener, output);
	wl_list_insert(&state->outputs, &output->link);
}
