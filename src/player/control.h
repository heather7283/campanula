#ifndef SRC_PLAYER_CONTROL_H
#define SRC_PLAYER_CONTROL_H

void player_set_pause(bool pause);
void player_toggle_pause(void);

void player_seek(long off, bool relative);

void player_set_volume(int volume, bool relative);
void player_toggle_mute(void);

void player_next(void);
void player_prev(void);

#endif /* #ifndef SRC_PLAYER_CONTROL_H */

