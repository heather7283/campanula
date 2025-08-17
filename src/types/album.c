#include "types/album.h"
#include "xmalloc.h"

void album_deep_copy(struct album *dst, const struct album *src) {
    dst->id = xstrdup(dst->id);
    dst->name = xstrdup(dst->name);
    dst->artist = xstrdup(dst->artist);
    dst->artist_id = xstrdup(dst->artist_id);

    dst->song_count = src->song_count;
    dst->duration = src->duration;
}

void album_free_contents(struct album *a) {
    if (a == NULL) {
        return;
    }

    free(a->id);
    free(a->name);
    free(a->artist);
    free(a->artist_id);
}

