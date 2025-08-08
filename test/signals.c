#include <assert.h>
#include <stdio.h>

#include "signals.h"

enum events {
    FIRST_COSMIC_VELOCITY = 1 << 0,
    SECOND_COSMIC_VELOCITY = 1 << 1,
    THIRD_COSMIC_VELOCITY = 1 << 2,
};

void first_callback(uint64_t event, const struct signal_data *data, void *userdata) {
    assert(data->type == SIGNAL_DATA_TYPE_U64);
    assert(data->as.u64 == 7910);

    fprintf(stderr, "First cosmic velocity is %lu m/s\n", data->as.u64);
    *(int *)userdata += 1;
}

void second_callback(uint64_t event, const struct signal_data *data, void *userdata) {
    assert(data->type == SIGNAL_DATA_TYPE_U64);
    assert(data->as.u64 == 11200);

    fprintf(stderr, "Second cosmic velocity is %lu m/s\n", data->as.u64);
    *(int *)userdata += 1;
}

void third_callback(uint64_t event, const struct signal_data *data, void *userdata) {
    assert(data->type == SIGNAL_DATA_TYPE_U64);
    assert(data->as.u64 == 16650);

    fprintf(stderr, "Third cosmic velocity is %lu m/s\n", data->as.u64);
    *(int *)userdata += 1;
}

void generic_callback(uint64_t event, const struct signal_data *data, void *userdata) {
    assert(data->type == SIGNAL_DATA_TYPE_U64);

    fprintf(stderr, "Got a number: %lu\n", data->as.u64);
    *(int *)userdata += 1;
}

int main(void) {
    struct signal_emitter e;
    struct signal_listener l1, l2, l3, g;

    signal_emitter_init(&e);

    int c1 = 0;
    signal_subscribe(&e, &l1, FIRST_COSMIC_VELOCITY, first_callback, &c1);
    int c2 = 0;
    signal_subscribe(&e, &l2, SECOND_COSMIC_VELOCITY, second_callback, &c2);
    int c3 = 0;
    signal_subscribe(&e, &l3, THIRD_COSMIC_VELOCITY, third_callback, &c3);
    int cg = 0;
    signal_subscribe(&e, &g,
                     THIRD_COSMIC_VELOCITY | SECOND_COSMIC_VELOCITY | FIRST_COSMIC_VELOCITY,
                     generic_callback, &cg);

    signal_emit_u64(&e, FIRST_COSMIC_VELOCITY, 7910);
    signal_emit_u64(&e, SECOND_COSMIC_VELOCITY, 11200);
    signal_emit_u64(&e, THIRD_COSMIC_VELOCITY, 16650);

    assert(c1 == 1);
    assert(c2 == 1);
    assert(c3 == 1);
    assert(cg == 3);

    return 0;
}

