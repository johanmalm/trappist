#include <ini.h>
#include <stdlib.h>
#include <string.h>
#include <sway-client-helpers/log.h>
#include "conf.h"

static void
set_default_values(struct conf *conf)
{
	conf->icon.size = 22;
}

static int
handler(void *user, const char *section, const char *name, const char *value)
{
	struct conf *conf = user;

	if (!strcmp(section, "icon")) {
		if (!strcmp(name, "size")) {
			conf->icon.size = atoi(value);
		}
	} else {
		LOG(LOG_ERROR, "unknown config section: %s", section);
	}

	return 1;
}

void
conf_init(struct conf *conf, const char *filename)
{
	set_default_values(conf);

	if (!filename) {
		return;
	}
	int result = ini_parse(filename, handler, conf);

	if (result == -1) {
		LOG(LOG_DEBUG, "no config file found");
	} else if (result == -2) {
		LOG(LOG_DEBUG, "out of memory");
		exit(EXIT_FAILURE);
	} else if (result != 0) {
		LOG(LOG_DEBUG, "ini file parse failure");
		exit(EXIT_FAILURE);
	}
}
