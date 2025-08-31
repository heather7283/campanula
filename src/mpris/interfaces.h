#ifndef SRC_MPRIS_INTERFACES_H
#define SRC_MPRIS_INTERFACES_H

#include "mpris/dbus.h"
#include "types/song.h"

enum playback_status {
    PLAYBACK_STATUS_PLAYING,
    PLAYBACK_STATUS_PAUSED,
    PLAYBACK_STATUS_STOPPED,
};

enum loop_status {
    LOOP_STATUS_NONE,
    LOOP_STATUS_TRACK,
    LOOP_STATUS_PLAYLIST,
};

bool mpris_init_interfaces(struct dbus_state *dbus_state);

bool mpris_update_metadata(const struct song *song);
bool mpris_update_playback_status(enum playback_status status);
bool mpris_update_position(int64_t pos_seconds);

bool mpris_emit_seek(int64_t new_pos_seconds);

#endif /* #ifndef SRC_MPRIS_INTERFACES_H */

