// SPDX-License-Identifier: GPL-2.0-only
#define _POSIX_C_SOURCE 200809L
#include <ccan/opt/opt.h>
#include <ccan/err/err.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sway-client-helpers/log.h>
#include <sway-client-helpers/loop.h>
#include "icon.h"
#include "menu.h"
#include "trappist.h"

static bool show_version;
static int verbose;
static char *config_file;
static char *menu_file;

static struct opt_table opts[] = {
	OPT_WITH_ARG("-c|--config-file=<filename>", opt_set_charp, opt_show_charp,
	     &config_file, "Specify config file (with path)"),
	OPT_WITHOUT_ARG("-h|--help", opt_usage_and_exit, "[options...]",
		"Show help message and quit"),
	OPT_WITH_ARG("-m|--menu-file=<filename>", opt_set_charp, opt_show_charp,
	     &menu_file, "Specify menu file (with path)"),
	OPT_WITHOUT_ARG("-v|--version", opt_set_bool, &show_version,
		"Show version and quit"),
	OPT_WITHOUT_ARG("-V|--verbose", opt_inc_intval, &verbose,
		"Enable more verbose logging; can be specified twice"),
	OPT_ENDTABLE
};

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
	err_set_progname(argv[0]);

	opt_register_table(opts, NULL);
	if (!opt_parse(&argc, argv, opt_log_stderr)) {
		exit(EXIT_FAILURE);
	}

	if (show_version) {
		printf("No version exists yet\n");
		exit(EXIT_FAILURE);
	}

	enum log_importance importance = LOG_ERROR + verbose;
	importance = MIN(importance, LOG_DEBUG);
	log_init(importance);

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

	state.seat->xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

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

	icon_init();

	if (!menu_file) {
		errx(EXIT_FAILURE, "cannot find menu file");
	}
	menu_init(&state, menu_file);

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

	menu_finish(&state);
	surface_destroy(state.surface);
	if (state.seat->cursor_theme) {
		wl_cursor_theme_destroy(state.seat->cursor_theme);
	}
	icon_finish();
	pango_cairo_font_map_set_default(NULL);
	return 0;
}
