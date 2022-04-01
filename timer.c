// SPDX-License-Identifier: GPL-2.0-only
#include <sway-client-helpers/loop.h>
#include "trappist.h"

#define TRAPPIST_HOVER_DELAY_MSEC (750)

static void
timer_hover_clear(void *data)
{
	struct state *state = data;
	state->hover_timer = NULL;
	surface_damage(state->surface);
}

void
timer_hover_start(struct state *state)
{
	if (state->hover_timer) {
		loop_remove_timer(state->eventloop, state->hover_timer);
	}
	state->hover_timer = loop_add_timer(state->eventloop,
		TRAPPIST_HOVER_DELAY_MSEC, timer_hover_clear, state);
}
