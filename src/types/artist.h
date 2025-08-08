#ifndef SRC_TYPES_ARTIST_H
#define SRC_TYPES_ARTIST_H

struct artist {
    char *id;
    char *name;
};

void artist_deep_copy(struct artist *dst, const struct artist *src);
void artist_free_contents(struct artist *artist);

#endif /* #ifndef SRC_TYPES_ARTIST_H */

