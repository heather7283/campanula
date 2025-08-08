#ifndef SRC_TYPES_ALBUM_H
#define SRC_TYPES_ALBUM_H

struct album {
    char *id;
    char *name;

    char *artist, *artist_id;

    int song_count, duration;
};

void album_deep_copy(struct album *dst, const struct album *src);
void album_free_contents(struct album *album);

#endif /* #ifndef SRC_TYPES_ALBUM_H */

