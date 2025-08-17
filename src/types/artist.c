#include "types/artist.h"
#include "xmalloc.h"

void artist_deep_copy(struct artist *dst, const struct artist *src) {
    dst->id = xstrdup(src->id);
    dst->name = xstrdup(src->name);
}

void artist_free_contents(struct artist *a) {
    if (a == NULL) {
        return;
    }

    free(a->id);
    free(a->name);
}

