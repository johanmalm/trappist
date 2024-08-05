#include <assert.h>
#include <sfdo-basedir.h>
#include <sfdo-icon.h>
#include <stdio.h>
#include <string.h>
#include <sway-client-helpers/log.h>
#include "icon.h"

static struct sfdo_icon_ctx *sfdo_icon_ctx;
static struct sfdo_icon_theme *icon_theme;

static int icon_size = 22;

void
icon_init(void)
{
	struct sfdo_basedir_ctx *sfdo_basedir_ctx = sfdo_basedir_ctx_create();

	sfdo_icon_ctx = sfdo_icon_ctx_create(sfdo_basedir_ctx);
	if (!sfdo_icon_ctx) {
		LOG(LOG_ERROR, "no sfdo_icon_ctx");
	}

	int options = SFDO_ICON_THEME_LOAD_OPTIONS_DEFAULT;
	options |= SFDO_ICON_THEME_LOAD_OPTION_RELAXED;
	options |= SFDO_ICON_THEME_LOAD_OPTION_ALLOW_MISSING;
	icon_theme = sfdo_icon_theme_load(sfdo_icon_ctx, "Papirus", options);

	sfdo_basedir_ctx_destroy(sfdo_basedir_ctx);
}

void
icon_finish(void)
{
	sfdo_icon_theme_destroy(icon_theme);
	sfdo_icon_ctx_destroy(sfdo_icon_ctx);
}

void
icon_set_size(int size)
{
	icon_size = size;
}

#define DEFAULT_ICON_NAME "folder"
#define ICON_LOOKUP_OPTIONS (SFDO_ICON_THEME_LOOKUP_OPTIONS_DEFAULT)

const char *
icon_strdup_path(const char *icon)
{
	assert(icon);
	size_t icon_len = strlen(icon);
	int scale = 1.0;
	struct sfdo_icon_file *file = sfdo_icon_theme_lookup(icon_theme, icon,
		icon_len, icon_size, scale, ICON_LOOKUP_OPTIONS);
	if (file == SFDO_ICON_FILE_INVALID) {
		return NULL;
	}
	if (!file) {
		/* Try to find the default icon */
		file = sfdo_icon_theme_lookup(icon_theme, DEFAULT_ICON_NAME,
			sizeof(DEFAULT_ICON_NAME) - 1, icon_size, scale,
			ICON_LOOKUP_OPTIONS);
		if (!file || file == SFDO_ICON_FILE_INVALID) {
			return NULL;
		}
	}

	const char *path = strdup(sfdo_icon_file_get_path(file, NULL));
	sfdo_icon_file_destroy(file);
	return path;
}

#undef ICON_LOOKUP_OPTIONS
