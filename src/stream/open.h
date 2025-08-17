#ifndef SRC_STREAM_OPEN_H
#define SRC_STREAM_OPEN_H

#include <stdint.h>

typedef int64_t (*stream_read_fn)(void *userdata, char *buf, uint64_t nbytes);
typedef int64_t (*stream_seek_fn)(void *userdata, int64_t offset);
typedef int64_t (*stream_size_fn)(void *userdata);
typedef void (*stream_close_fn)(void *userdata);

struct stream_functions {
    stream_read_fn read;
    stream_seek_fn seek;
    stream_size_fn size;
    stream_close_fn close;
};

bool stream_open(const char *song_id, int bitrate, const char *filetype,
                 struct stream_functions *functions, void **userdata);

#endif /* #ifndef SRC_STREAM_OPEN_H */

