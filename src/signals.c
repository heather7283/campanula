#include "signals.h"

void signal_emitter_init(struct signal_emitter *emitter) {
    LIST_INIT(&emitter->listeners);
}

void signal_subscribe(struct signal_emitter *emitter, struct signal_listener *listener,
                      uint64_t events, signal_callback_func_t callback, void *callback_data) {
    listener->events = events;
    listener->callback = callback;
    listener->callback_data = callback_data;

    LIST_APPEND(&emitter->listeners, &listener->link);
}

void signal_unsubscribe(struct signal_listener *listener) {
    LIST_REMOVE(&listener->link);
}

static void signal_emit_internal(const struct signal_emitter *emitter,
                                 uint64_t event, struct signal_data *data) {
    const struct signal_listener *listener;
    LIST_FOREACH(listener, &emitter->listeners, link) {
        if (event & listener->events) {
            listener->callback(event, data, listener->callback_data);
        }
    }
}

void signal_emit_ptr(const struct signal_emitter *emitter, uint64_t event, void *ptr) {
    struct signal_data data = {
        .type = SIGNAL_DATA_TYPE_PTR,
        .as.ptr = ptr,
    };
    signal_emit_internal(emitter, event, &data);
}

void signal_emit_u64(const struct signal_emitter *emitter, uint64_t event, uint64_t u64) {
    struct signal_data data = {
        .type = SIGNAL_DATA_TYPE_U64,
        .as.u64 = u64,
    };
    signal_emit_internal(emitter, event, &data);
}

void signal_emit_i64(const struct signal_emitter *emitter, uint64_t event, int64_t i64) {
    struct signal_data data = {
        .type = SIGNAL_DATA_TYPE_I64,
        .as.i64 = i64,
    };
    signal_emit_internal(emitter, event, &data);
}

