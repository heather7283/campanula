#ifndef SRC_PLAYER_EVENTS_H
#define SRC_PLAYER_EVENTS_H

#include <mpv/client.h>

#include "signals.h"

enum player_event: uint64_t {
    PLAYER_EVENT_PLAYLIST_POSITION = 1 << 0, /* position as i64 */
    PLAYER_EVENT_PERCENT_POSITION = 1 << 1, /* percentage as i64 */
    PLAYER_EVENT_PAUSE = 1 << 2, /* boolean, true if paused, false if not */
    PLAYER_EVENT_VOLUME = 1 << 3, /* volume as i64 */
    PLAYER_EVENT_MUTE = 1 << 4, /* boolean */
    PLAYER_EVENT_PLAYLIST = 1 << 5, /* nothing */
};

void player_event_subscribe(struct signal_listener *listener, enum player_event events,
                            signal_callback_func_t callback, void *callback_data);

#endif /* #ifndef SRC_PLAYER_EVENTS_H */

