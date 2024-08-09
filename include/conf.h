/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef TRAPPIST_CONF_H
#define TRAPPIST_CONF_H
#include <stdio.h>
#include <stdlib.h>

struct icon {
	int size;
};

struct conf {
	struct icon icon;
};

void conf_init(struct conf *conf, const char *filename);

#endif /* TRAPPIST_CONF_H */
