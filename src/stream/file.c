#include <unistd.h>

#include "stream/file.h"
#include "xmalloc.h"

struct file_stream_data {
    int fd;
    size_t size;
};

static int64_t file_stream_read(void *userdata, char *buf, uint64_t nbytes) {
    const struct file_stream_data *d = userdata;
    return read(d->fd, buf, nbytes);
}

static int64_t file_stream_seek(void *userdata, int64_t offset) {
    const struct file_stream_data *d = userdata;
    return lseek(d->fd, offset, SEEK_SET);
}

static int64_t file_stream_size(void *userdata) {
    const struct file_stream_data *d = userdata;
    return d->size;
}

static void file_stream_close(void *userdata) {
    struct file_stream_data *d = userdata;
    close(d->fd);
    free(d);
}

bool stream_open_from_fd(int fd, size_t size, struct stream_functions *funcs, void **userdata) {
    struct file_stream_data *d = xmalloc(sizeof(*d));
    *d = (struct file_stream_data){
        .fd = fd,
        .size = size,
    };

    *funcs = (struct stream_functions){
        .read = file_stream_read,
        .seek = file_stream_seek,
        .size = file_stream_size,
        .close = file_stream_close,
    };

    *userdata = d;

    return true;
}

