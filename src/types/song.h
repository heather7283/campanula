#ifndef SRC_SONG_H
#define SRC_SONG_H

struct song {
    char *id;
    char *title;

    char *album, *album_id;
    char *artist, *artist_id;

    int track, year, duration, bitrate;
};

void song_deep_copy(struct song *dst, const struct song *src);
void song_free_contents(struct song *song);

#endif /* #ifndef SRC_SONG_H */

