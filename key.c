// SPDX-License-Identifier: GPL-2.0-only
#include <stdlib.h>
#include <xkbcommon/xkbcommon.h>
#include "trappist.h"

void
key_handle(struct state *state, xkb_keysym_t keysym, uint32_t codepoint)
{
	switch (keysym) {
	case XKB_KEY_KP_Enter:
	case XKB_KEY_Return:
		state->run_display = false;
		break;
	case XKB_KEY_BackSpace:
		search_remove_last_uft8_character();
		timer_hover_start(state);
		break;
	case XKB_KEY_Escape:
		state->run_display = false;
		break;
	case XKB_KEY_Up:
		break;
	case XKB_KEY_Down:
		break;
	default:
		if (!codepoint) {
			break;
		}
		search_add_utf8_character(codepoint);
		timer_hover_start(state);
		break;
	}
	surface_damage(state->surface);
}
