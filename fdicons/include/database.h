#ifndef _DATABASE_H
#define _DATABASE_H
#include <fdicons.h>

struct fd_icon_database {
	char *default_theme;
	struct fd_icon_theme *themes;
};

#endif
