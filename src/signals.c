#include "signals.h"
#include "eventloop.h"

static int signal_emitter_dispatch_events(struct pollen_callback *, uint64_t, void *data) {
    struct signal_emitter *emitter = data;

    VEC_FOREACH(&emitter->queued_events, i) {
        const struct signal_queued_event *ev = VEC_AT(&emitter->queued_events, i);

        const struct signal_listener *listener;
        LIST_FOREACH(listener, &emitter->listeners, link) {
            if (ev->event & listener->events) {
                listener->callback(ev->event, &ev->data, listener->callback_data);
            }
        }
    }
    VEC_CLEAR(&emitter->queued_events);

    return 0;
}

bool signal_emitter_init(struct signal_emitter *emitter) {
    LIST_INIT(&emitter->listeners);
    VEC_INIT(&emitter->queued_events);

    emitter->efd = pollen_loop_add_efd(event_loop, signal_emitter_dispatch_events, emitter);
    return (emitter->efd == NULL);
}

void signal_emitter_cleanup(struct signal_emitter *emitter) {
    pollen_loop_remove_callback(emitter->efd);
    VEC_FREE(&emitter->queued_events);
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
    struct signal_queued_event *ev = VEC_EMPLACE_BACK(&emitter->queued_events);
    ev->event = event;
    ev->data = *data;

    pollen_efd_trigger(emitter->efd, 1);
}

void signal_emit_ptr(const struct signal_emitter *emitter, uint64_t event, void *ptr) {
    struct signal_data data = {
        .type = SIGNAL_DATA_TYPE_PTR,
        .as.ptr = ptr,
    };
    signal_emit_internal(emitter, event, &data);
}

void signal_emit_str(const struct signal_emitter *emitter, uint64_t event, char *str) {
    struct signal_data data = {
        .type = SIGNAL_DATA_TYPE_STR,
        .as.str = str,
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

void signal_emit_f64(const struct signal_emitter *emitter, uint64_t event, double f64) {
    struct signal_data data = {
        .type = SIGNAL_DATA_TYPE_F64,
        .as.f64 = f64,
    };
    signal_emit_internal(emitter, event, &data);
}

void signal_emit_bool(const struct signal_emitter *emitter, uint64_t event, bool boolean) {
    struct signal_data data = {
        .type = SIGNAL_DATA_TYPE_BOOLEAN,
        .as.boolean = boolean,
    };
    signal_emit_internal(emitter, event, &data);
}

