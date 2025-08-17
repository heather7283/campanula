#include "types/cached_song.h"
#include "xmalloc.h"

void cached_song_deep_copy(struct cached_song *dst, const struct cached_song *src) {
    dst->id = xstrdup(src->id);
    dst->filename = xstrdup(src->filename);
    dst->bitrate = src->bitrate;
    dst->filetype = xstrdup(src->filetype);
    dst->size = src->size;
}

void cached_song_free_contents(struct cached_song *song) {
    if (song == NULL) {
        return;
    }

    free(song->id);
    free(song->filename);
    free(song->filetype);
}

