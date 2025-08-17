#ifndef SRC_STREAM_NETWORK_H
#define SRC_STREAM_NETWORK_H

#include "stream/open.h"

bool stream_open_from_network(const char *id, int bitrate, const char *filetype,
                              struct stream_functions *funcs, void **userdata);

#endif /* #ifndef SRC_STREAM_NETWORK_H */

