/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef TALLOC_HELPERS
#define TALLOC_HELPERS
#include <errno.h>
#include <talloc.h>

#define defer __attribute__((__cleanup__(tal_cleanup)))

static void
tal_cleanup(TALLOC_CTX **tal)
{
	talloc_free(*tal);
}

static inline
TALLOC_CTX *xtalloc_new(const void *ctx)
{
	TALLOC_CTX *tal = talloc_new(ctx);
	if (!tal) {
		perror("out of memory");
		exit(EXIT_FAILURE);
	}
	return tal;
}

#endif /* TALLOC_HELPERS */
