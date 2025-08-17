#ifndef SRC_TYPES_CACHED_SONG_H
#define SRC_TYPES_CACHED_SONG_H

#include <stdint.h>
#include <stddef.h>

struct cached_song {
    char *id;
    char *filename;
    int bitrate;
    char *filetype;
    size_t size;
};

void cached_song_deep_copy(struct cached_song *dst, const struct cached_song *src);
void cached_song_free_contents(struct cached_song *cached_song);

#endif /* #ifndef SRC_TYPES_CACHED_SONG_H */

