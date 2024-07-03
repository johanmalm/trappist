#ifndef _FDICONS_H
#define _FDICONS_H
#include <stdbool.h>
#include <stdarg.h>

struct fd_icon_database;
struct fd_icon_criteria;
struct fd_icon;
struct fd_icon_attach_point;
struct fd_icon_theme;
struct fd_icon_theme_dir;

/** Creates an empty icon database. */
struct fd_icon_database *fd_icon_database_create(void);

/** Adds the default search paths for icon themes. */
void fd_icon_database_add_default_paths(struct fd_icon_database *database);

/** Adds a new search path for icon themes. */
void fd_icon_database_add_path(struct fd_icon_database *database,
		const char *path);

/** Returns a pointer to a linked list of known themes. */
struct fd_icon_theme *fd_icon_database_get_themes(
		struct fd_icon_database *database);

/** Returns a specific icon theme, or NULL if unknown. */
struct fd_icon_theme *fd_icon_database_get_theme(
		struct fd_icon_database *database, const char *name);

/** Overrides the icon theme name. */
void fd_icon_database_set_theme(struct fd_icon_database *database,
		const char *name);

/** Destroys an icon database. */
void fd_icon_database_destroy(struct fd_icon_database *database);

/**
 * Finds an icon by name, with a list of icon names in order of preference,
 * followed by a NULL.
 */
struct fd_icon *fd_icon_database_get_icon(
		struct fd_icon_database *database, int size, ...);

/**
 * Finds a specific icon, using the specified criteria to narrow the search.
 * Returns the icon, or NULL if none was found. The returned icon must be
 * destroyed with fd_icon_destroy when the caller is done with it.
 */
struct fd_icon *fd_icon_database_get_icon_criteria(
		struct fd_icon_database *database,
		const struct fd_icon_criteria *criteria);

/** Destroys this icon and frees resources associated with it. */
void fd_icon_destroy(struct fd_icon *icon);

enum fd_icon_type {
	/** No type is specified for this icon. */
	FD_ICON_TYPE_UNSPECIFIED,

	/** Fixed-size icons. */
	FD_ICON_TYPE_FIXED,

	/** Icons which may be scaled within some constraints. */
	FD_ICON_TYPE_SCALABLE,

	/** Icons which may be used if within a certain threshold in size. */
	FD_ICON_TYPE_THRESHOLD,
};

/**
 * Search criteria for finding icons.
 */
struct fd_icon_criteria {
	/**
	 * The names of the icons to search for, in order of preference. Denote
	 * the last entry with a NULL. Required.
	 */
	const char **names;

	/** The desired size of the icon. Required. */
	int size;

	/**
	 * A list of acceptable file extensions, or NULL if no preference.
	 * Denote the last entry with a NULL. For example:
	 *
	 * { "png", "svg", NULL }
	 */
	const char **extensions;

	/** A specific icon theme to search within, or NULL if no preference. */
	struct fd_icon_theme *theme;

	/** The desired scale of the icon, or zero if no preference. */
	int scale;
};

struct fd_icon_theme {
	/** The name of the icon theme.  */
	char *name;

	/** The path to the theme on disk. */
	char *path;

	/** The display name of the icon theme.  */
	char *display_name;

	/** A description of this icon theme. */
	char *comment;

	/** If set, this theme falls back to the specified theme name. */
	char *inherits;

	/**
	 * If true, this theme should not be included in theme selectors shown
	 * to the user.
	 */
	bool hidden;

	/**
	 * The name of an example icon which should represent this theme to the
	 * user when presenting a theme selector. May be NULL.
	 */
	char *example;

	/** Linked list of icon theme directories. */
	struct fd_icon_theme_dir *directories;

	/** Pointer to next icon theme. */
	struct fd_icon_theme *next;
};

struct fd_icon_theme_dir {
	/** The theme this icon dir is a member of. */
	struct fd_icon_theme *theme;

	/** Name of this directory */
	char *name;

	/** The size (in pixels) of icons in this directory. */
	int size;

	/** The scale factor these icons are designed for. */
	int scale;

	/** The context in which these icons are used. */
	char *context;

	/** The scaling logistics for these icons. */
	enum fd_icon_type type;

	/** The minimum size these icons may be scaled to. Only valid if
	 * type=SCALABLE. Defaults to size. */
	int min_size;

	/** The maximum size these icons may be scaled to. Only valid if
	 * type=SCALABLE. Defaults to size. */
	int max_size;

	/**
	 * The tolerance in size difference that these icons are still
	 * acceptable for use with. Only valid if type=THRESHOLD. Defaults to 2.
	 */
	int threshold;

	/** Next directory in this icon theme. */
	struct fd_icon_theme_dir *next;
};

/**
 * A specific icon from an icon theme.
 */
struct fd_icon {
	/** The icon theme directory this icon is a member of. */
	struct fd_icon_theme_dir *theme_dir;

	/** Path to this icon. */
	char *path;

	/** Internal name for this icon. */
	char *name;

	/** Display name for this icon. May be NULL. */
	char *display_name;

	/**
	 * A rectangle in which arbitrary text may be drawn by the application.
	 * Specified in pixel coordinates, except in the case of SVGs, which
	 * instead use an arbitrary 1000x1000 coordinate space that should be
	 * scaled to the final icon size. All fields will be zero if unset.
	 */
	struct {
		int x0, y0, x1, y1;
	} embedded_text_rectangle;

	/**
	 * Pointer to first attach point, or NULL if none are specified.
	 */
	struct fd_icon_attach_point *attach_points;
};

/**
 * Specifies a point at which emblems or overlays may be drawn on top of the
 * icon.
 */
struct fd_icon_attach_point {
	/**
	 * Coordinates of this attach point. Specified in pixel coordinates,
	 * except in the case of SVGs, which instead use an arbitrary 1000x1000
	 * coordinate space that should be scaled to the final icon size.
	 */
	int x, y;

	/** Next attach point in the list. */
	struct fd_icon_attach_point *next;
};

#endif
