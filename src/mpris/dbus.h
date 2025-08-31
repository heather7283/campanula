#ifndef SRC_MPRIS_DBUS_H
#define SRC_MPRIS_DBUS_H

#ifdef HAVE_LIBSYSTEMD
#include <systemd/sd-bus.h>
#elif HAVE_LIBELOGIND
#include <elogind/sd-bus.h>
#elif HAVE_BASU
#include <basu/sd-bus.h>
#endif

struct dbus_state {
    struct sd_bus *bus;

    struct sd_bus_slot *mpris_vtable_slot;
    struct sd_bus_slot *player_vtable_slot;

    int bus_fd;
    uint32_t events;
    bool timer_armed;
    struct pollen_callback *bus_fd_callback;
    struct pollen_callback *bus_timer_callback;
};

extern struct dbus_state dbus;

typedef int dbus_bool;

#endif /* #ifndef SRC_MPRIS_DBUS_H */

