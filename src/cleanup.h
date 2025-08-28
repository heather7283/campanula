#ifndef SRC_CLEANUP_H
#define SRC_CLEANUP_H

#include <stdlib.h>

static inline void cleanup_free(void *pptr) {
	free(*(void **)pptr);
}

#endif /* #ifndef SRC_CLEANUP */

