#ifndef SRC_PLAYER_PLAYLIST_H
#define SRC_PLAYER_PLAYLIST_H

#include <stddef.h>

#include "song.h"

void playlist_append_song(const struct song *song);

size_t playlist_get_songs(const struct song **songs);
size_t playlist_get_current_song(const struct song **song);

void playlist_play(void);
void playlist_pause(void);

#endif /* #ifndef SRC_PLAYER_PLAYLIST_H */

