#include "types/song.h"
#include "xmalloc.h"

void song_deep_copy(struct song *dst, const struct song *src) {
    dst->id = xstrdup(src->id);
    dst->title = xstrdup(src->title);
    dst->album = xstrdup(src->album);
    dst->album_id = xstrdup(src->album_id);
    dst->artist = xstrdup(src->artist);
    dst->artist_id = xstrdup(src->artist_id);
    dst->filetype = xstrdup(src->filetype);

    dst->track = src->track;
    dst->year = src->year;
    dst->duration = src->duration;
    dst->bitrate = src->bitrate;
    dst->size = src->size;
}

void song_free_contents(struct song *song) {
    free(song->id);
    free(song->title);
    free(song->album);
    free(song->album_id);
    free(song->artist);
    free(song->artist_id);
    free(song->filetype);
}

