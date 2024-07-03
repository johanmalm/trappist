#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fdicons.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <wordexp.h>
#include "database.h"
#include "ini.h"

#define DATADIR "/usr/share"

static int
mkdirs(char *path, mode_t mode)
{
	char dname[PATH_MAX + 1];
	strcpy(dname, path);
	if (strcmp(dirname(dname), "/") == 0) {
		return 0;
	}
	strcpy(dname, path);
	if (mkdirs(dirname(dname), mode) != 0) {
		if (errno != EEXIST) {
			return -1;
		}
		errno = 0;
	}
	return mkdir(path, mode);
}

static int
gtk_theme_ini_handler(void *data, const char *sec,
		const char *key, const char *val)
{
	struct fd_icon_database *database = data;
	if (strcmp(sec, "Settings") == 0 &&
			strcmp(key, "gtk-icon-theme-name") == 0) {
		free(database->default_theme);
		database->default_theme = strdup(val);
	}
	return 1;
}

static void
attempt_gtk_fallback(struct fd_icon_database *database)
{
	FILE *gtk_conf = NULL;
	const char *gtk_paths[] = {
		"$XDG_CONFIG_HOME/gtk-3.0/settings.ini",
		"$HOME/.config/gtk-3.0/settings.ini",
		"$HOME/.gtk-3.0/settings.ini",
		"/etc/gtk-3.0/settings.ini",
	};
	for (size_t i = 0; i < sizeof(gtk_paths) / sizeof(gtk_paths[0]); ++i) {
		wordexp_t p;
		if (wordexp(gtk_paths[i], &p, WRDE_UNDEF) == 0) {
			gtk_conf = fopen(p.we_wordv[0], "r");
			wordfree(&p);
			if (gtk_conf) {
				break;
			}
		}
	}
	if (!gtk_conf) {
		database->default_theme = strdup("hicolor");
		return;
	}
	ini_parse_file(gtk_conf, gtk_theme_ini_handler, database);
	fclose(gtk_conf);

	if (!database->default_theme) {
		database->default_theme = strdup("hicolor");
		return;
	}

	FILE *theme_conf = NULL;
	const char *paths[] = {
		"$XDG_CONFIG_HOME/theme.conf",
		"$HOME/.config/theme.conf",
	};
	for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
		wordexp_t p;
		if (wordexp(paths[i], &p, WRDE_UNDEF) == 0) {
			char path[PATH_MAX + 1];
			assert(strlen(p.we_wordv[0]) < sizeof(path) - 1);
			strcpy(path, p.we_wordv[0]);
			if (mkdirs(dirname(path), 0755) != 0 &&
					errno != EEXIST) {
				wordfree(&p);
				continue;
			}
			theme_conf = fopen(p.we_wordv[0], "w");
			wordfree(&p);
			if (theme_conf) {
				break;
			}
		}
	}
	if (theme_conf) {
		fprintf(theme_conf, "[Settings]\nicon-theme-name=%s\n",
			database->default_theme);
		fclose(theme_conf);
	}
}

static int
user_theme_ini_handler(void *data, const char *sec,
		const char *key, const char *val)
{
	struct fd_icon_database *database = data;
	if (strcmp(sec, "Settings") == 0 &&
			strcmp(key, "icon-theme-name") == 0) {
		free(database->default_theme);
		database->default_theme = strdup(val);
	}
	return 1;
}

struct fd_icon_database *
fd_icon_database_create(void)
{
	struct fd_icon_database *database =
		calloc(1, sizeof(struct fd_icon_database));

	FILE *theme_conf = NULL;
	const char *paths[] = {
		"$XDG_CONFIG_HOME/theme.conf",
		"$HOME/.config/theme.conf",
		"$HOME/.theme.conf",
		"/etc/theme.conf",
	};
	for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
		wordexp_t p;
		if (wordexp(paths[i], &p, WRDE_UNDEF) == 0) {
			theme_conf = fopen(p.we_wordv[0], "r");
			wordfree(&p);
			if (theme_conf) {
				break;
			}
		}
	}
	if (!theme_conf) {
		attempt_gtk_fallback(database);
		return database;
	}

	ini_parse_file(theme_conf, user_theme_ini_handler, database);
	fclose(theme_conf);
	if (!database->default_theme) {
		database->default_theme = strdup("hicolor");
	}

	return database;
}

void
fd_icon_database_add_default_paths(struct fd_icon_database *database)
{
	const char *xdg_data_home = getenv("XDG_DATA_HOME"),
	      *xdg_data_dirs = getenv("XDG_DATA_DIRS");

	char *data_home;
	if (xdg_data_home) {
		data_home = "$XDG_DATA_HOME/icons";
	} else {
		data_home = "$HOME/.local/share/icons";
	}

	wordexp_t p;
	if (wordexp("$HOME/.icons", &p, WRDE_UNDEF) == 0) {
		fd_icon_database_add_path(database, p.we_wordv[0]);
		wordfree(&p);
	}
	if (wordexp(data_home, &p, WRDE_UNDEF) == 0) {
		fd_icon_database_add_path(database, p.we_wordv[0]);
		wordfree(&p);
	}

	fd_icon_database_add_path(database, DATADIR "/pixmaps");

	char *data_dirs;
	if (xdg_data_dirs) {
		data_dirs = strdup(xdg_data_dirs);
	} else {
		data_dirs = strdup(DATADIR);
	}

	char *path = strtok(data_dirs, ":");
	do {
		char full_path[PATH_MAX + 1];
		if (snprintf(full_path, sizeof(full_path),
				"%s/icons", path) >= PATH_MAX) {
			fprintf(stderr, "Warning: ignoring icon path %s "
					"(too long)", path);
			continue;
		}
		fd_icon_database_add_path(database, full_path);
	} while ((path = strtok(NULL, ":")));

	free(data_dirs);
}

static bool
strbool(const char *s)
{
	char *truth[] = { "true", "yes", "on" };
	for (size_t i = 0; i < sizeof(truth) / sizeof(truth[0]); ++i) {
		if (strcasecmp(truth[i], s) == 0) {
			return true;
		}
	}
	return false;
}

static enum fd_icon_type
strtype(const char *s)
{
	struct {
		const char *name;
		enum fd_icon_type val;
	} types[] = {
		{ "Fixed", FD_ICON_TYPE_FIXED },
		{ "Scalable", FD_ICON_TYPE_SCALABLE },
		{ "Threshold", FD_ICON_TYPE_THRESHOLD },
	};
	for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
		if (strcasecmp(types[i].name, s) == 0) {
			return types[i].val;
		}
	}
	return FD_ICON_TYPE_UNSPECIFIED;
}

static int
theme_index_ini_handler(void *data, const char *sec,
		const char *key, const char *val)
{
	struct fd_icon_theme *theme = data;
	if (strcmp(sec, "Icon Theme") == 0) {
		if (strcmp(key, "Name") == 0) {
			free(theme->display_name);
			theme->display_name = strdup(val);
		} else if (strcmp(key, "Comment") == 0) {
			free(theme->comment);
			theme->comment = strdup(val);
		} else if (strcmp(key, "Inherits") == 0) {
			free(theme->inherits);
			theme->inherits = strdup(val);
		} else if (strcmp(key, "Hidden") == 0) {
			theme->hidden = strbool(val);
		} else if (strcmp(key, "Example") == 0) {
			free(theme->example);
			theme->example = strdup(val);
		}
		/* Note: the Directories key is ignored */
	} else {
		/* XXX: Traversing the whole linked list every time will get
		 * slow if you have a lot of dirs. Could be solved by making a
		 * little parser context struct which contains a cached pointer
		 * to the last theme dir we were working on. */
		struct fd_icon_theme_dir **dirptr = &theme->directories;
		while (*dirptr) {
			if (strcmp(sec, (*dirptr)->name) == 0) {
				break;
			}
			dirptr = &(*dirptr)->next;
		}
		struct fd_icon_theme_dir *dir = *dirptr;
		if (!*dirptr) {
			dir = calloc(1, sizeof(struct fd_icon_theme_dir));
			dir->name = strdup(sec);
			dir->theme = theme;
			dir->scale = 1;
			dir->min_size = 0;
			dir->max_size = 0;
			dir->threshold = 2;
			*dirptr = dir;
		}

		char *endptr;
		errno = 0;
		if (strcmp(key, "Size") == 0) {
			dir->size = strtol(val, &endptr, 10);
			if (*endptr || errno != 0 || dir->size <= 0) {
				dir->size = -1;
				fprintf(stderr,
					"Warning: %s/%s: invalid size '%s'\n",
					theme->name, dir->name, val);
			}
		} else if (strcmp(key, "Scale") == 0) {
			dir->scale = strtol(val, &endptr, 10);
			if (*endptr || errno != 0 || dir->scale < 1) {
				dir->scale = 1;
				fprintf(stderr,
					"Warning: %s/%s: invalid scale '%s'\n",
					theme->name, dir->name, val);
			}
		} else if (strcmp(key, "Context") == 0) {
			free(dir->context);
			dir->context = strdup(val);
		} else if (strcmp(key, "Type") == 0) {
			dir->type = strtype(val);
			if (dir->type == FD_ICON_TYPE_UNSPECIFIED) {
				fprintf(stderr,
					"Warning: %s/%s: invalid type '%s'\n",
					theme->name, dir->name, val);
			}
		} else if (strcmp(key, "MinSize") == 0) {
			dir->min_size = strtol(val, &endptr, 10);
			if (*endptr || errno != 0 || dir->min_size < 0) {
				dir->min_size = 0;
				fprintf(stderr,
					"Warning: %s/%s: invalid size '%s'\n",
					theme->name, dir->name, val);
			}
		} else if (strcmp(key, "MaxSize") == 0) {
			dir->max_size = strtol(val, &endptr, 10);
			if (*endptr || errno != 0 || dir->max_size < 0) {
				dir->max_size = 0;
				fprintf(stderr,
					"Warning: %s/%s: invalid size '%s'\n",
					theme->name, dir->name, val);
			}
		} else if (strcmp(key, "Threshold") == 0) {
			dir->threshold = strtol(val, &endptr, 10);
			if (*endptr || errno != 0 || dir->threshold <= 0) {
				dir->threshold = 2;
				fprintf(stderr,
					"Warning: %s/%s: invalid threshold '%s'\n",
					theme->name, dir->name, val);
			}
		}
		/* TODO: Capture vendor-specific X-attributes */
	}
	return 1;
}

void
fd_icon_database_add_path(struct fd_icon_database *database, const char *path)
{
	struct fd_icon_theme **next = &database->themes;
	while (*next) {
		next = &(*next)->next;
	}

	DIR *dir = opendir(path);
	if (!dir) {
		return;
	}
	char index_path[PATH_MAX + 1];
	struct dirent *d;
	while ((d = readdir(dir))) {
		int n = snprintf(index_path, sizeof(index_path),
				"%s/%s/index.theme", path, d->d_name);
		if (n < 0 || n >= PATH_MAX) {
			continue;
		}
		FILE *f = fopen(index_path, "r");
		if (!f) {
			continue;
		}
		struct fd_icon_theme *theme =
			calloc(1, sizeof(struct fd_icon_theme));
		theme->name = strdup(d->d_name);
		snprintf(index_path, sizeof(index_path),
				"%s/%s", path, d->d_name);
		theme->path = strdup(index_path);

		ini_parse_file(f, theme_index_ini_handler, theme);
		fclose(f);

		*next = theme;
		next = &theme->next;
	}
}

static void
fd_icon_theme_dir_destroy(struct fd_icon_theme_dir *directories)
{
	while (directories) {
		struct fd_icon_theme_dir *next = directories->next;
		free(directories->name);
		free(directories->context);
		free(directories);
		directories = next;
	}
}

static void
fd_icon_theme_destroy(struct fd_icon_theme *themes)
{
	while (themes) {
		struct fd_icon_theme *next = themes->next;
		fd_icon_theme_dir_destroy(themes->directories);
		free(themes->comment);
		free(themes->display_name);
		free(themes->example);
		free(themes->inherits);
		free(themes->name);
		free(themes->path);
		free(themes);
		themes = next;
	}
}

void
fd_icon_database_destroy(struct fd_icon_database *database)
{
	if (!database) {
		return;
	}
	fd_icon_theme_destroy(database->themes);
	free(database->default_theme);
	free(database);
}

struct fd_icon_theme *
fd_icon_database_get_themes(struct fd_icon_database *database)
{
	return database->themes;
}

void
fd_icon_database_set_theme(struct fd_icon_database *database, const char *name)
{
	free(database->default_theme);
	database->default_theme = strdup(name);
}

struct fd_icon_theme *
fd_icon_database_get_theme(struct fd_icon_database *database, const char *name)
{
	struct fd_icon_theme *theme = database->themes;
	while (theme) {
		if (strcmp(theme->name, name) == 0) {
			return theme;
		}
		theme = theme->next;
	}
	return NULL;
}
