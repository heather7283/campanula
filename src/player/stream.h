#ifndef SRC_PLAYER_STREAM_H
#define SRC_PLAYER_STREAM_H

#include <stdint.h>

#include <mpv/stream_cb.h>

int player_stream_open(void *userdata, char *uri, struct mpv_stream_cb_info *info);

#endif /* #ifndef SRC_PLAYER_STREAM_H */

