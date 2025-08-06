#ifndef SRC_PLAYER_CONTROL_H
#define SRC_PLAYER_CONTROL_H

#include "song.h"

void player_set_pause(bool pause);

bool player_load_song(const struct song *song);
bool player_unload_song(void);

#endif /* #ifndef SRC_PLAYER_CONTROL_H */

