#ifndef TRAPPIST_ICON_H
#define TRAPPIST_ICON_H

#include "fdicons.h"

void icon_init(void);
void icon_finish(void);
void icon_set_size(int size);
const char *icon_strdup_path(const char *app_id);

#endif /* TRAPPIST_ICON_H */
