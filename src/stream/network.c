#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "stream/network.h"
#include "api/requests.h"
#include "collections/vec.h"
#include "db/cache.h"
#include "xmalloc.h"
#include "config.h"
#include "log.h"

struct network_stream_data {
    VEC(uint8_t) data;
    int64_t pos;
    bool eof, error, closed;

    bool new_data;
    pthread_cond_t cond;
    pthread_mutex_t mutex;

    int bitrate;
    char *filetype;
    char *id;

    int out_fd;
};

static void network_stream_finalise(struct network_stream_data *d) {
    /* TODO: do this asynchronously? this can potentially block for a long time on slow storage */
    int fd = -1;
    char *filepath = NULL;

    if (!d->eof) {
        TRACE("network stream closed before entire file was received; not saving into cache");
        goto out;
    }

    char *filename = d->id;
    xasprintf(&filepath, "%s/%s", config.music_cache_dir, filename);
    TRACE("opening file at %s", filepath);
    fd = open(filepath, O_TRUNC | O_CREAT | O_WRONLY | O_CLOEXEC, 0644);
    if (fd < 0) {
        ERROR("cannot save song into cache: failed to create file %s: %m", filepath);
        goto out;
    }

    size_t written = 0;
    while (written < VEC_SIZE(&d->data)) {
        ssize_t ret = write(fd, VEC_DATA(&d->data), VEC_SIZE(&d->data) - written);
        if (ret < 0) {
            ERROR("cannot save song into cache: failed to write to file: %m");
            goto out;
        }
        written += ret;
    }

    db_add_cached_song(&(struct cached_song){
        .id = d->id,
        .filetype = d->filetype,
        .bitrate = d->bitrate,
        .filename = filename,
        .size = VEC_SIZE(&d->data),
    });

out:
    if (fd >= 0) {
        close(fd);
    }
    free(filepath);

    VEC_FREE(&d->data);
    free(d->id);
    free(d->filetype);
    free(d);
}

static int64_t network_stream_read(void *cookie, char *buf, uint64_t nbytes) {
    struct network_stream_data *d = cookie;
    int64_t ret = -1;

    pthread_mutex_lock(&d->mutex);

again:
    if (d->error) {
        ret = -1;
        goto out;
    } if ((size_t)d->pos < VEC_SIZE(&d->data)) {
        /* can return at least 1 byte before hitting end of buffer */
        const uint64_t available = VEC_SIZE(&d->data) - d->pos;
        const uint64_t len = MIN(available, nbytes);

        memcpy(buf, VEC_DATA(&d->data) + d->pos, len);
        d->pos += len;

        ret = len;
        goto out;
    } else if (d->eof) {
        /* hit end of buffer and there's no more data to be received. Signal EOF */
        ret = 0;
        goto out;
    } else /* if (!d->eof) */ {
        /* hit end of buffer, but there might be more data to receive.
         * Need to block until more data arrives and retry */
        while (!d->new_data) {
            pthread_cond_wait(&d->cond, &d->mutex);
        }
        d->new_data = false;

        goto again;
    }

out:
    pthread_mutex_unlock(&d->mutex);

    return ret;
}

static int64_t network_stream_seek(void *cookie, int64_t offset) {
    struct network_stream_data *d = cookie;

    pthread_mutex_lock(&d->mutex);

    d->pos = MIN(VEC_SIZE(&d->data), (size_t)offset);

    pthread_mutex_unlock(&d->mutex);

    return d->pos;
}

static int64_t network_stream_size(void *cookie) {
    struct network_stream_data *d = cookie;
    return VEC_SIZE(&d->data);
}

static void network_stream_close(void *cookie) {
    struct network_stream_data *d = cookie;

    pthread_mutex_lock(&d->mutex);

    TRACE("close; eof %d error %d closed %d", d->eof, d->error, d->closed);

    if (d->eof || d->error) {
        /* api_stream_data_callback won't be called anymore, can free now */
        pthread_mutex_unlock(&d->mutex);
        network_stream_finalise(d);
    } else {
        d->closed = true;
        pthread_mutex_unlock(&d->mutex);
    }
}

static bool api_stream_data_callback(const char *errmsg, size_t expected_size,
                                     const void *data, ssize_t data_size, void *userdata) {
    struct network_stream_data *d = userdata;

    pthread_mutex_lock(&d->mutex);

    if (d->closed) {
        /* mpv doesn't need this stream anymore */
        pthread_mutex_unlock(&d->mutex);
        network_stream_finalise(d);
        return false;
    }

    switch (data_size) {
    case -1: /* error */
        d->error = true;
        ERROR("data: %s", errmsg);
        /* fall through */
    case 0: /* EOF */
        d->eof = true;
        break;
    default: /* data */
        if (VEC_SIZE(&d->data) == 0 && expected_size > 0) {
            VEC_RESERVE(&d->data, expected_size);
        }
        VEC_APPEND_N(&d->data, (uint8_t *)data, data_size);
        break;
    }
    d->new_data = true;
    pthread_cond_signal(&d->cond);

    pthread_mutex_unlock(&d->mutex);

    return true;
}

bool stream_open_from_network(const char *id, int bitrate, const char *filetype,
                              struct stream_functions *funcs, void **userdata) {
    struct network_stream_data *d = xmalloc(sizeof(*d));
    *d = (struct network_stream_data){
        .cond = PTHREAD_COND_INITIALIZER,
        .mutex = PTHREAD_MUTEX_INITIALIZER,

        .id = xstrdup(id),
        .bitrate = bitrate,
        .filetype = xstrdup(filetype),
    };

    if (!api_stream(id, bitrate, filetype, api_stream_data_callback, d)) {
        goto err;
    }

    *funcs = (struct stream_functions){
        .read = network_stream_read,
        .seek = network_stream_seek,
        .size = network_stream_size,
        .close = network_stream_close,
    };

    *userdata = d;

    return true;

err:
    network_stream_finalise(d);
    return false;
}

