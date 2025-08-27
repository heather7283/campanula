#include "types/album.h"
#include "xmalloc.h"

void album_deep_copy(struct album *dst, const struct album *src) {
    dst->id = xstrdup(src->id);
    dst->name = xstrdup(src->name);
    dst->artist = xstrdup(src->artist);
    dst->artist_id = xstrdup(src->artist_id);

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

