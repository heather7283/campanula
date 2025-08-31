#ifndef SRC_MPRIS_INTERFACES_H
#define SRC_MPRIS_INTERFACES_H

#include "mpris/dbus.h"
#include "types/song.h"

bool mpris_init_interfaces(struct dbus_state *dbus_state);

bool mpris_update_metadata(const struct song *song);

#endif /* #ifndef SRC_MPRIS_INTERFACES_H */

