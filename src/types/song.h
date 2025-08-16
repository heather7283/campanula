#ifndef SRC_TYPES_SONG_H
#define SRC_TYPES_SONG_H

struct song {
    char *id;
    char *title;

    char *album, *album_id;
    char *artist, *artist_id;

    char *filetype;
    int track, year, duration, bitrate;
    long size;
};

void song_deep_copy(struct song *dst, const struct song *src);
void song_free_contents(struct song *song);

#endif /* #ifndef SRC_TYPES_SONG_H */

