#ifndef SRC_PLAYER_UTILS_H
#define SRC_PLAYER_UTILS_H

#include <mpv/client.h>

#include "log.h"

void print_mpv_node(const struct mpv_node *n, enum log_level lvl, int indent);

#endif /* #ifndef SRC_PLAYER_UTILS_H */

