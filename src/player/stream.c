#include <sys/eventfd.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "player/stream.h"
#include "player/common.h"
#include "api/requests.h"
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

    TRACE("read: requested %lu bytes; size %lu off %li eof %d err %d",
          nbytes, ARRAY_SIZE(&d->data), d->pos, d->eof, d->error);

again:
    if (d->error) {
        ret = -1;
        goto out;
    } if ((size_t)d->pos < ARRAY_SIZE(&d->data)) {
        /* can return at least 1 byte before hitting end of buffer */
        const uint64_t available = ARRAY_SIZE(&d->data) - d->pos;
        const uint64_t len = MIN(available, nbytes);

        memcpy(buf, ARRAY_DATA(&d->data) + d->pos, len);
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
        TRACE("read: waiting for more data");

        sem_post(&d->sem);
        uint64_t dummy;
        eventfd_read(d->efd, &dummy);
        sem_wait(&d->sem);

        goto again;
    }

out:
    TRACE("read: returning %lu bytes; size %lu off %li eof %d err %d",
          ret, ARRAY_SIZE(&d->data), d->pos, d->eof, d->error);

    sem_post(&d->sem);

    return ret;
}

static int64_t stream_seek_callback(void *cookie, int64_t offset) {
    struct stream_data *d = cookie;

    sem_wait(&d->sem);

    TRACE("seek: requested seek to %li; size %lu off %li eof %d err %d",
          offset, ARRAY_SIZE(&d->data), d->pos, d->eof, d->error);

    d->pos = MIN(ARRAY_SIZE(&d->data), (size_t)offset);

    TRACE("seek: returning %li; size %lu off %li eof %d err %d",
          offset, ARRAY_SIZE(&d->data), d->pos, d->eof, d->error);

    sem_post(&d->sem);

    return d->pos;
}

static int64_t stream_size_callback(void *cookie) {
    return MPV_ERROR_UNSUPPORTED; /* TODO? */
}

static void stream_close_callback(void *cookie) {
    TRACE("close: destroying stream");
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
        ERROR("data: %s", errmsg);
        d->error = true;
        break;
    case 0: /* EOF */
        TRACE("data: EOF");
        d->eof = true;
        break;
    default: /* data */
        TRACE("data: new data arrived, %zu bytes", data_size);
        ARRAY_EXTEND(&d->data, (uint8_t *)data, data_size);
        break;
    }
    eventfd_write(d->efd, 1);

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

    if (!api_stream(id, 128, "raw", false, api_stream_data_callback, d)) {
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


