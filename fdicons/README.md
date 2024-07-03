# fdicons

A small library which implements the [FreeDesktop Icon Theme
specification][spec] in a small POSIX C99 library with no external dependencies,
as well as a simple binary which can be used for searching icon themes with
shell scripts.

[spec]: https://specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html#context

## Basic Usage (C)

```c
struct fd_icon_database *db = fd_icon_database_create();
fd_icon_database_add_default_paths(db);

struct fd_icon *icon;

/* Basic search, finds most suitable icon with a list of fallbacks */
icon = fd_icon_database_get_icon(db, 256
	"firefox", "web-browser", "application", NULL);

struct fd_icon_criteria criteria = {
	.names = { "firefox", "web-browser", "application", NULL },
	.extensions = { "png", NULL },
	.size = 64,
	.scale = 2,
};

/* Advanced search */
icon = fd_icon_database_get_icon_criteria(db, &criteria);
```

## Basic Usage (CLI)

```sh
$ fdicons get-icon -x png,svg -S 2 64 firefox web-browser application
/usr/share/icons/hicolor/64x64@2/apps/firefox.png
```

## Specifying the default theme

fdicons attempts to introduce a simple standard for global theme configuration.
To this end, it first reads `~/.config/theme.conf` (the exact path is subject to
XDG environment), which is a short INI file with the following format:

```ini
[Settings]
icon-theme-name=...
# ...other keys may be present...
```

If this file is not present, fdicons will search for the user's GTK+ 3.0
settings. If found, fdicons will create a `theme.conf` based on it.

TODO: Reach out to other projects and standardize `theme.conf`
