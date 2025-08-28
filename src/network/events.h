#ifndef SRC_NETWORK_EVENTS_H
#define SRC_NETWORK_EVENTS_H

#include "signals.h"

enum network_event: uint64_t {
    NETWORK_EVENT_SPEED_DL = 1ULL << 0, /* download speed in bytes per second as u64 */
    NETWORK_EVENT_SPEED_UL = 1ULL << 1, /* upload speed in bytes per second as u64 */
    NETWORK_EVENT_CONNECTIONS = 1ULL << 2, /* number of active connections as u64 */
};

void network_event_subscribe(struct signal_listener *listener, enum network_event events,
                             signal_callback_func_t callback, void *callback_data);


#endif /* #ifndef SRC_NETWORK_EVENTS_H */

