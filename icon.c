#include <stdio.h>
#include <string.h>
#include "icon.h"

static struct fd_icon_database *fd_icon_database;
static int icon_size = 22;

void
icon_init(void)
{
	fd_icon_database = fd_icon_database_create();
	fd_icon_database_add_default_paths(fd_icon_database);
}

void
icon_finish(void)
{
	fd_icon_database_destroy(fd_icon_database);
}

void
icon_set_size(int size)
{
	icon_size = size;
}

#define FALLBACK "folder"

const char *
icon_strdup_path(const char *name)
{
	struct fd_icon *fd_icon = fd_icon_database_get_icon(fd_icon_database,
		icon_size, name, FALLBACK, NULL);
	const char *path = strdup(fd_icon->path);
	fd_icon_destroy(fd_icon);
	return path;
}
