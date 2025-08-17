#ifndef SRC_DB_CACHE_H
#define SRC_DB_CACHE_H

#include "types/cached_song.h"

bool db_get_cached_song(struct cached_song *song, const char *song_id);
bool db_delete_cached_song(const char *song_id);
bool db_add_cached_song(const struct cached_song *song);
bool db_touch_cached_song(const char *song_id);

#endif /* #ifndef SRC_DB_CACHE_H */

