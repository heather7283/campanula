#ifndef SRC_PLAYER_PLAYLIST_H
#define SRC_PLAYER_PLAYLIST_H

#include <sys/types.h>
#include <stddef.h>

#include "types/song.h"

/* Does not take ownership of song, creates its own private copy. */
void playlist_append_song(const struct song *song);
void playlist_clear(void);

/* returns number of songs */
int playlist_get_songs(const struct song **songs);
/* returns index of current song, -1 if no currrent */
int playlist_get_current_song(const struct song **song);

#endif /* #ifndef SRC_PLAYER_PLAYLIST_H */

