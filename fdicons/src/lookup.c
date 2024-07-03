#include <assert.h>
#include <fdicons.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "database.h"
#include "ini.h"

static struct fd_icon *
fd_icon_from_path(struct fd_icon_theme_dir *dir,
		const char *path, const char *name)
{
	struct fd_icon *icon = calloc(1, sizeof(struct fd_icon));
	if (icon == NULL) {
		return NULL;
	}

	icon->theme_dir = dir;
	icon->path = strdup(path);
	icon->name = strdup(name);
	/* TODO: Read the .icon file if present */

	return icon;
}

static bool
fd_icon_theme_dir_matches_size(
		const struct fd_icon_theme_dir *dir,
		const struct fd_icon_criteria *criteria)
{
	int scale = criteria->scale == 0 ? 1 : criteria->scale;
	if (dir->scale != scale) {
		return false;
	}
	switch (dir->type) {
	case FD_ICON_TYPE_FIXED:
		return dir->size == criteria->size;
	case FD_ICON_TYPE_SCALABLE:
		return dir->min_size <= criteria->size &&
				dir->max_size >= criteria->size;
	case FD_ICON_TYPE_THRESHOLD:
		return (dir->size - dir->threshold <= criteria->size
				&& criteria->size <= dir->size + dir->threshold);
	case FD_ICON_TYPE_UNSPECIFIED:
		/* Fallthrough */
	default:
		return false;
	}
}

static int
fd_icon_theme_dir_size_distance(
		const struct fd_icon_theme_dir *dir,
		const struct fd_icon_criteria *criteria)
{
	int scale = criteria->scale == 0 ? 1 : criteria->scale;
	int size = scale * criteria->size;
	int min_size = dir->min_size != 0 ? dir->min_size : dir->size,
	    max_size = dir->max_size != 0 ? dir->max_size : dir->size;
	switch (dir->type) {
	case FD_ICON_TYPE_FIXED:
		return abs(dir->size * dir->scale - size);
	case FD_ICON_TYPE_SCALABLE:
		if (size < min_size * dir->scale) {
			return min_size * scale - size;
		}
		if (size > max_size * dir->scale) {
			return size - max_size * scale;
		}
		return INT_MAX;
	case FD_ICON_TYPE_THRESHOLD:
		if (size < (dir->size-dir->threshold) * dir->scale) {
			return min_size * dir->scale - size;
		}
		if (size > (dir->size+dir->threshold) * dir->scale) {
			return size - max_size * dir->scale;
		}
		return INT_MAX;
	case FD_ICON_TYPE_UNSPECIFIED:
		/* Fallthrough */
	default:
		return INT_MAX;
	}
}

static struct fd_icon *
fd_icon_theme_dir_get_icon_criteria(
		struct fd_icon_database *database,
		struct fd_icon_theme *theme,
		struct fd_icon_theme_dir *dir,
		const struct fd_icon_criteria *criteria,
		struct fd_icon_theme_dir **minimal_dir,
		int *minimal_size,
		char **minimal_path,
		const char **minimal_name)
{
	static const char *default_exts[] = { "png", "svg", "xpm", NULL };
	const char **exts = criteria->extensions;
	if (exts == NULL || exts[0] == NULL) {
		exts = (const char **)default_exts;
	}
	struct stat st;
	char path[PATH_MAX + 1];
	for (int i = 0; criteria->names[i]; ++i)
	for (int j = 0; exts[j]; ++j)
	{
		int n = snprintf(path, sizeof(path), "%s/%s/%s.%s",
			theme->path, dir->name, criteria->names[i], exts[j]);
		if (n >= PATH_MAX) {
			continue;
		}
		if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
			continue;
		}
		if (fd_icon_theme_dir_matches_size(dir, criteria)) {
			return fd_icon_from_path(dir, path, criteria->names[i]);
		}
		int distance = fd_icon_theme_dir_size_distance(dir, criteria);
		if (distance < *minimal_size) {
			*minimal_dir = dir;
			*minimal_size = distance;
			free(*minimal_path);
			*minimal_path = strdup(path);
			*minimal_name = criteria->names[i];
		}
	}
	return NULL;
}

static struct fd_icon *
fd_icon_theme_get_icon_criteria(
		struct fd_icon_database *database,
		struct fd_icon_theme *theme,
		const struct fd_icon_criteria *criteria)
{
	struct fd_icon *icon = NULL;
	struct fd_icon_theme_dir *dir = theme->directories;
	int minimal_size = INT_MAX;
	struct fd_icon_theme_dir *minimal_dir = NULL;
	char *minimal_path = NULL;
	const char *minimal_name = NULL;
	while (dir) {
		icon = fd_icon_theme_dir_get_icon_criteria(
				database, theme, dir, criteria,
				&minimal_dir, &minimal_size, &minimal_path,
				&minimal_name);
		if (icon) {
			goto exit;
		}
		dir = dir->next;
	}
	if (minimal_size != INT_MAX) {
		icon = fd_icon_from_path(minimal_dir,
				minimal_path, minimal_name);
		goto exit;
	}

	if (icon == NULL && theme->inherits) {
		struct fd_icon_theme *parent =
			fd_icon_database_get_theme(database, theme->inherits);
		if (parent) {
			icon = fd_icon_theme_get_icon_criteria(
					database, parent, criteria);
		}
	}

exit:
	free(minimal_path);
	return icon;
}

struct fd_icon *
fd_icon_database_get_icon_criteria(struct fd_icon_database *database,
		const struct fd_icon_criteria *criteria)
{
	assert(criteria->names && criteria->names[0] && criteria->size > 0);
	struct fd_icon *icon = NULL;
	struct fd_icon_theme *theme = criteria->theme,
	     *hicolor = fd_icon_database_get_theme(database, "hicolor");
	if (!theme) {
		theme = fd_icon_database_get_theme(database,
				database->default_theme);
		if (!theme) {
			theme = hicolor;
		}
		if (!theme) {
			return NULL;
		}
	}
	icon = fd_icon_theme_get_icon_criteria(database, theme, criteria);
	if (!icon && theme != hicolor && hicolor) {
		icon = fd_icon_theme_get_icon_criteria(
				database, hicolor, criteria);
	}
	return icon;
}

struct fd_icon *
fd_icon_database_get_icon(struct fd_icon_database *database, int size, ...)
{
	va_list ap;
	const char *name;
	int nnames = 0;

	va_start(ap, size);
	while (va_arg(ap, const char *)) {
		++nnames;
	}
	va_end(ap);

	struct fd_icon_criteria criteria = {
		.names = malloc(sizeof(const char *) * (nnames + 1)),
		.size = size,
	};

	int i = 0;
	va_start(ap, size);
	while ((name = va_arg(ap, const char *))) {
		criteria.names[i++] = name;
	}
	va_end(ap);
	criteria.names[i] = NULL;

	struct fd_icon *icon =
		fd_icon_database_get_icon_criteria(database, &criteria);
	free(criteria.names);
	return icon;
}

static void
fd_icon_attach_point_destroy(struct fd_icon_attach_point *attach_points)
{
	while (attach_points) {
		struct fd_icon_attach_point *next = attach_points->next;
		free(attach_points);
		attach_points = next;
	}
}

void
fd_icon_destroy(struct fd_icon *icon)
{
	if (!icon) {
		return;
	}
	free(icon->path);
	free(icon->name);
	free(icon->display_name);
	fd_icon_attach_point_destroy(icon->attach_points);
	free(icon);
}
