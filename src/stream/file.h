#ifndef SRC_STREAM_FILE_H
#define SRC_STREAM_FILE_H

#include <stddef.h>

#include "stream/open.h"

bool stream_open_from_fd(int fd, size_t size, struct stream_functions *funcs, void **userdata);

#endif /* #ifndef SRC_STREAM_FILE_H */

