#include "player/events.h"
#include "player/internal.h"
#include "log.h"

void player_event_subscribe(struct signal_listener *listener, enum player_event events,
                            signal_callback_func_t callback, void *callback_data) {
    signal_subscribe(&player.emitter, listener, events, callback, callback_data);
}

