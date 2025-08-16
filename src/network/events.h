#ifndef SRC_NETWORK_EVENTS_H
#define SRC_NETWORK_EVENTS_H

#include "signals.h"

enum network_event: uint64_t {
    NETWORK_EVENT_SPEED = 1 << 0, /* speed in bytes per second as u64 */
    NETWORK_EVENT_CONNECTIONS = 1 << 1, /* number of active connections as u64 */
};

void network_event_subscribe(struct signal_listener *listener, enum network_event events,
                             signal_callback_func_t callback, void *callback_data);


#endif /* #ifndef SRC_NETWORK_EVENTS_H */

