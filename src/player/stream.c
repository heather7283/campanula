#include <sys/eventfd.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "player/stream.h"
#include "player/common.h"
#include <api/requests.h>
#include "collections/array.h"
#include "xmalloc.h"
#include "log.h"

struct stream_data {
    ARRAY(uint8_t) data;
    bool eof, error;
    int64_t pos;

    /* TODO: those 2 can most likely be merged to get rid of the spaghetti */
    int efd; /* for blocking until there's more data */
    sem_t sem; /* for thread safety */
};

static void stream_data_free(struct stream_data *d) {
    close(d->efd);
    sem_destroy(&d->sem);
    ARRAY_FREE(&d->data);
    free(d);
}

static int64_t stream_read_callback(void *cookie, char *buf, uint64_t nbytes) {
    struct stream_data *d = cookie;
    int64_t ret = -1;

    sem_wait(&d->sem);

    TRACE("player/stream/read: requested read of %lu bytes at off %li", nbytes, d->pos);

again:
    if (d->error) {
        TRACE("player/stream/read: returning -1 (error)");
        return -1;
    }

    if (d->eof) { /* we don't expect new data to be appended to the buffer */
        if ((size_t)d->pos >= ARRAY_SIZE(&d->data)) {
            /* requested read past data end, signal EOF */
            TRACE("player/stream/read: returning 0 (EOF)");
            ret = 0;
            goto out;
        } else {
            const uint64_t available = ARRAY_SIZE(&d->data) - d->pos;
            const uint64_t len = MIN(available, nbytes);

            memcpy(buf, ARRAY_DATA(&d->data) + d->pos, len);
            d->pos += len;

            TRACE("player/stream/read: returning %li", len);
            ret = len;
            goto out;
        }
    } else {
        if ((size_t)d->pos >= ARRAY_SIZE(&d->data)) {
            /* not at EOF yet, but there's no more data available at the moment.
             * We need to block until new data arrives. */
            TRACE("player/stream/read: waiting for more data");
            sem_post(&d->sem);
            uint64_t dummy;
            eventfd_read(d->efd, &dummy);
            sem_wait(&d->sem);

            goto again;
        } else {
            const uint64_t available = ARRAY_SIZE(&d->data) - d->pos;
            const uint64_t len = MIN(available, nbytes);

            memcpy(buf, ARRAY_DATA(&d->data) + d->pos, len);
            d->pos += len;

            TRACE("player/stream/read: returning %li", len);
            ret = len;
            goto out;
        }
    }

out:
    sem_post(&d->sem);
    return ret;
}

static int64_t stream_seek_callback(void *cookie, int64_t offset) {
    struct stream_data *d = cookie;

    sem_wait(&d->sem);

    TRACE("player/stream/seek: requested seek to %li", offset);
    if ((size_t)offset >= ARRAY_SIZE(&d->data)) {
        TRACE("player/stream/seek: offset %li past end of data %zu, returning %li",
              offset, ARRAY_SIZE(&d->data), d->pos);
        d->pos = ARRAY_SIZE(&d->data);
    } else {
        d->pos = offset;
    }

    sem_post(&d->sem);

    return d->pos;
}

static int64_t stream_size_callback(void *cookie) {
    return MPV_ERROR_UNSUPPORTED; /* TODO? */
}

static void stream_close_callback(void *cookie) {
    TRACE("player/stream/close: destroying stream");
    stream_data_free(cookie);
}

static void stream_cancel_callback(void *cookie) {
    /* TODO: what does this even do? */
}

static void api_stream_data_callback(const char *errmsg, const void *data,
                                     ssize_t data_size, void *userdata) {
    struct stream_data *d = userdata;

    sem_wait(&d->sem);

    switch (data_size) {
    case -1: /* error */
        ERROR("player/stream/data: %s", errmsg);
        d->error = true;
        break;
    case 0: /* EOF */
        TRACE("player/stream/data: EOF");
        d->eof = true;
        eventfd_write(d->efd, 1);
        break;
    default: /* data */
        TRACE("player/stream/data: new data arrived, %zu bytes", data_size);
        ARRAY_EXTEND(&d->data, (uint8_t *)data, data_size);
        eventfd_write(d->efd, 1);
        break;
    }

    sem_post(&d->sem);

    return;
}

int player_stream_open(void *userdata, char *uri, struct mpv_stream_cb_info *info) {
    const char *id = uri + strlen(PLAYER_PROTOCOL) + strlen("://");
    TRACE("stream_open_callback: uri %s id %s", uri, id);

    struct stream_data *d = xcalloc(1, sizeof(*d));
    d->efd = eventfd(0, EFD_CLOEXEC);
    if (d->efd < 0) {
        ERROR("could not create eventfd: %s", strerror(errno));
        goto err;
    }
    if (sem_init(&d->sem, 0, 1) < 0) {
        ERROR("could not create semaphore: %s", strerror(errno));
        goto err;
    }

    if (!api_stream(id, 128, "opus", false, api_stream_data_callback, d)) {
        goto err;
    }

    *info = (struct mpv_stream_cb_info){
        .read_fn = stream_read_callback,
        .seek_fn = stream_seek_callback,
        .size_fn = stream_size_callback,
        .close_fn = stream_close_callback,
        .cancel_fn = NULL,

        .cookie = d,
    };

    return 0;

err:
    stream_data_free(d);
    return MPV_ERROR_LOADING_FAILED;
}


