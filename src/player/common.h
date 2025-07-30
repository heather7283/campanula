#ifndef SRC_PLAYER_COMMON_H
#define SRC_PLAYER_COMMON_H

struct player_state {
    struct mpv_handle *mpv_handle;
    struct pollen_callback *events_callback;
};

extern struct player_state player_state;

#endif /* #ifndef SRC_PLAYER_COMMON_H */

