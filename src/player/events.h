#ifndef SRC_PLAYER_EVENTS_H
#define SRC_PLAYER_EVENTS_H

#include <mpv/client.h>

#include "signals.h"

enum player_event: uint64_t {
    PLAYER_EVENT_PLAYLIST_POSITION = 1ULL << 0, /* position as i64 */
    PLAYER_EVENT_PERCENT_POSITION = 1ULL << 1, /* percentage as i64 */
    PLAYER_EVENT_PAUSE = 1ULL << 2, /* boolean, true if paused, false if not */
    PLAYER_EVENT_VOLUME = 1ULL << 3, /* volume as i64 */
    PLAYER_EVENT_MUTE = 1ULL << 4, /* boolean */
    PLAYER_EVENT_DURATION = 1ULL << 5, /* duration in milliseconds as u64 */
    PLAYER_EVENT_TIME_POSITION = 1ULL << 6, /* milliseconds as u64 */
    PLAYER_EVENT_SEEK = 1ULL << 7, /* new offset in milliseconds as u64 */
    PLAYER_EVENT_IDLE = 1ULL << 8, /* boolean */

    PLAYER_EVENT_PLAYLIST_SONG_ADDED = 1ULL << 31, /* song index as u64 */
    PLAYER_EVENT_PLAYLIST_SONG_REMOVED = 1ULL << 32, /* song index as u64 */
    PLAYER_EVENT_PLAYLIST_CLEARED = 1ULL << 33, /* nothing */
};

void player_event_subscribe(struct signal_listener *listener, enum player_event events,
                            signal_callback_func_t callback, void *callback_data);

#endif /* #ifndef SRC_PLAYER_EVENTS_H */

