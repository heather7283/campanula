#ifndef SRC_PLAYER_CONTROL_H
#define SRC_PLAYER_CONTROL_H

void player_set_pause(bool pause);
void player_toggle_pause(void);

void player_seek(long off, bool relative);

void player_set_volume(int volume, bool relative);
void player_toggle_mute(void);

void player_next(void);
void player_prev(void);
void player_play_nth(int index);

bool player_stop(void);
bool player_quit(void);

bool player_is_paused(void);
bool player_is_idle(void);

#endif /* #ifndef SRC_PLAYER_CONTROL_H */

