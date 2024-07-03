#include <dirent.h>
#include <fdicons.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
usage(const char *argv_0)
{
	fprintf(stderr, "Usage:\n"
			"%s list-themes [-H]\n"
			"%s list-icon-dirs [<theme>]\n"
			"%s list-icons [<theme> [<dir>]]\n"
			"%s get-icon [-i] [-S <scale>] [-x <png,svg...>] "
				"<size> <name...>\n",
			argv_0, argv_0, argv_0, argv_0);
}

static int
list_themes(struct fd_icon_database *database, int argc, char *argv[])
{
	bool hidden = false;

	int c;
	while ((c = getopt(argc - 1, &argv[1], "H")) != -1) {
		switch (c) {
		case 'H':
			hidden = true;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	struct fd_icon_theme *theme = fd_icon_database_get_themes(database);
	const char *fmt = "%-15s %-25s %s\n";
	if (isatty(STDOUT_FILENO)) {
		printf(fmt, "NAME", "DISPLAY NAME", "COMMENT");
		printf(fmt, "----", "------------", "-------");
	}
	while (theme) {
		if (theme->hidden && !hidden) {
			goto next;
		}
		printf(fmt, theme->name, theme->display_name, theme->comment);
next:
		theme = theme->next;
	}
	return 0;
}

static char *
split_extension(char *name)
{
	char *ext = strrchr(name, '.');
	if (!ext) {
		return NULL;
	}

	*ext = '\0';
	ext++;
	return ext;
}

static bool
is_icon(const char *ext)
{
	const char *default_exts[] = { "png", "svg", "xpm", NULL };
	for (int i = 0; default_exts[i]; ++i) {
		if (strcmp(ext, default_exts[i]) == 0) {
			return true;
		}
	}
	return false;
}

static const char *
typestr(enum fd_icon_type type)
{
	switch (type) {
	case FD_ICON_TYPE_UNSPECIFIED:
		return "Unspecified";
	case FD_ICON_TYPE_FIXED:
		return "Fixed";
	case FD_ICON_TYPE_SCALABLE:
		return "Scalable";
	case FD_ICON_TYPE_THRESHOLD:
		return "Threshold";
	}
	return "Unknown";
}

static int
list_icon_dirs(struct fd_icon_database *database, int argc, char *argv[])
{
	struct fd_icon_theme *theme;
	char *theme_name = argc > 2 ? argv[2] : NULL;
	if (theme_name) {
		theme = fd_icon_database_get_theme(database, theme_name);
	} else {
		theme = fd_icon_database_get_themes(database);
	}

	const char *fmt = "%-30s %-20s %-10s %5d %5d (%s)\n";
	if (isatty(STDOUT_FILENO)) {
		const char *table_fmt = "%-30s %-20s %-10s %5s %5s %s\n";
		printf(table_fmt, "NAME", "CONTEXT", "TYPE",
				"SIZE", "SCALE", "THEME");
		printf(table_fmt, "----", "-------", "----",
				"----", "-----", "-----");
	}
	while (theme) {
		struct fd_icon_theme_dir *dir = theme->directories;
		while (dir) {
			printf(fmt, dir->name, dir->context,
				typestr(dir->type), dir->size, dir->scale,
				theme->display_name);
			dir = dir->next;
		}

		if (theme_name) {
			break;
		}
		theme = theme->next;
	}
	return 0;
}

static void
list_icons_in_dir(struct fd_icon_theme_dir *theme_dir, const char *path)
{
	DIR *dir = opendir(path);
	if (!dir) {
		return;
	}

	struct dirent *d;
	while ((d = readdir(dir))) {
		char *name = strdup(d->d_name);
		char *ext = split_extension(name);
		if (!ext || !is_icon(ext)) {
			free(name);
			continue;
		}

		printf("%-40s %-20s %-10s %5d %5d %-20s %s/%s\n",
				name, theme_dir->context,
				typestr(theme_dir->type), theme_dir->size,
				theme_dir->scale, theme_dir->theme->name,
				path, d->d_name);
		free(name);
	}
}

static int
list_icons(struct fd_icon_database *database, int argc, char *argv[])
{
	/* TODO: Check argc for a specific directory */
	struct fd_icon_theme *theme;
	char *theme_name = argc > 2 ? argv[2] : NULL;
	if (theme_name) {
		theme = fd_icon_database_get_theme(database, theme_name);
	} else {
		theme = fd_icon_database_get_themes(database);
	}

	if (true || isatty(STDOUT_FILENO)) {
		const char *table_fmt = "%-40s %-20s %-10s %5s %5s %-20s %s\n";
		printf(table_fmt, "NAME", "CONTEXT", "TYPE",
				"SIZE", "SCALE", "THEME", "PATH");
		printf(table_fmt, "----", "-------", "----",
				"----", "-----", "-----", "----");
	}

	while (theme) {
		struct fd_icon_theme_dir *dir = theme->directories;
		char path[PATH_MAX + 1];
		while (dir) {
			int n = snprintf(path, sizeof(path), "%s/%s",
					theme->path, dir->name);
			if (n < 0 || n >= PATH_MAX) {
				dir = dir->next;
				continue;
			}

			list_icons_in_dir(dir, path);
			dir = dir->next;
		}

		if (theme_name) {
			break;
		}
		theme = theme->next;
	}

	return 0;
}

static int
get_icon(struct fd_icon_database *database, int argc, char **argv)
{
	bool show_info = false;
	struct fd_icon_criteria criteria = {
		.names = malloc(sizeof(char *) * argc),
	};

	int c;
	while ((c = getopt(argc - 1, &argv[1], "iS:x:")) != -1) {
		switch (c) {
		case 'i':
			show_info = true;
			break;
		case 'S':
			criteria.scale = strtol(optarg, NULL, 10);
			break;
		case 'x':;
			int nexts = 1;
			char *p = optarg;
			while ((p = strchr(p, ','))) {
				*p = '\0';
				++nexts;
			}
			criteria.extensions =
				malloc(sizeof(char *) * (nexts + 1));
			for (int i = 0; i < nexts; ++i) {
				criteria.extensions[i] = optarg;
				optarg = strchr(optarg, '\0') + 1;
			}
			criteria.extensions[nexts] = NULL;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (argc - optind < 2) {
		usage(argv[0]);
		return 1;
	}

	++optind;
	criteria.size = strtol(argv[optind++], NULL, 10);
	int i;
	for (i = 0; optind + i < argc; ++i) {
		criteria.names[i] = argv[optind + i];
	}
	criteria.names[i] = NULL;

	struct fd_icon *icon = fd_icon_database_get_icon_criteria(
			database, &criteria);
	if (icon == NULL) {
		goto error;
	}

	if (show_info) {
		struct fd_icon_theme_dir *dir = icon->theme_dir;
		struct fd_icon_theme *theme = dir->theme;
		printf("Size:    %dx%d@%d\n"
			"Type:    %s\n"
			"Context: %s\n"
			"Theme:   %s (%s)\n"
			"Name:    %s\n"
			"Path:    %s\n",
			dir->size, dir->size, dir->scale,
			typestr(dir->type), dir->context,
			theme->display_name, theme->name,
			icon->display_name ? icon->display_name : icon->name,
			icon->path);
	} else {
		printf("%s\n", icon->path);
	}

	fd_icon_destroy(icon);
	free(criteria.names);
	return 0;
error:
	free(criteria.names);
	return 1;
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	struct fd_icon_database *database = fd_icon_database_create();
	fd_icon_database_add_default_paths(database);

	struct {
		const char *name;
		int (*func)(struct fd_icon_database *, int, char *[]);
	} programs[] = {
		{ "list-themes", list_themes },
		{ "list-icon-dirs", list_icon_dirs },
		{ "list-icons", list_icons },
		{ "get-icon", get_icon },
	};

	for (size_t i = 0; i < sizeof(programs) / sizeof(programs[0]); ++i) {
		if (strcmp(argv[1], programs[i].name) == 0) {
			return programs[i].func(database, argc, argv);
		}
	}

	usage(argv[0]);
	return 1;
}
