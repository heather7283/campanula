#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "player/internal.h"
#include "api/requests.h"
#include "collections/array.h"
#include "xmalloc.h"
#include "log.h"

struct stream_data {
    ARRAY(uint8_t) data;
    int64_t pos;
    bool eof, error, closed;

    bool new_data;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};

static void stream_data_free(struct stream_data *d) {
    ARRAY_FREE(&d->data);
    free(d);
}

static int64_t stream_read_callback(void *cookie, char *buf, uint64_t nbytes) {
    struct stream_data *d = cookie;
    int64_t ret = -1;

    pthread_mutex_lock(&d->mutex);

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

static int64_t stream_seek_callback(void *cookie, int64_t offset) {
    struct stream_data *d = cookie;

    pthread_mutex_lock(&d->mutex);

    d->pos = MIN(ARRAY_SIZE(&d->data), (size_t)offset);

    pthread_mutex_unlock(&d->mutex);

    return d->pos;
}

static int64_t stream_size_callback(void *cookie) {
    struct stream_data *d = cookie;
    return ARRAY_SIZE(&d->data);
}

static void stream_close_callback(void *cookie) {
    struct stream_data *d = cookie;

    pthread_mutex_lock(&d->mutex);

    TRACE("close; eof %d error %d closed %d", d->eof, d->error, d->closed);

    if (d->eof || d->error) {
        /* api_stream_data_callback won't be called anymore, can free now */
        pthread_mutex_unlock(&d->mutex);
        stream_data_free(d);
    } else {
        d->closed = true;
        pthread_mutex_unlock(&d->mutex);
    }
}

static void stream_cancel_callback(void *cookie) {
    TRACE("cancel; not doing anything"); /* TODO: what is this suppoed to do? */
}

static bool api_stream_data_callback(const char *errmsg, const void *data,
                                     ssize_t data_size, void *userdata) {
    struct stream_data *d = userdata;

    pthread_mutex_lock(&d->mutex);

    if (d->closed) {
        /* mpv doesn't need this stream anymore */
        pthread_mutex_unlock(&d->mutex);
        stream_data_free(d);
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
        ARRAY_APPEND_N(&d->data, (uint8_t *)data, data_size);
        break;
    }
    d->new_data = true;
    pthread_cond_signal(&d->cond);

    pthread_mutex_unlock(&d->mutex);

    return true;
}

int player_stream_open(void *userdata, char *uri, struct mpv_stream_cb_info *info) {
    const char *id = uri + strlen(MPV_PROTOCOL) + strlen("://");

    struct stream_data *d = xcalloc(1, sizeof(*d));
    d->cond = (TYPEOF(d->cond))PTHREAD_COND_INITIALIZER;
    d->mutex = (TYPEOF(d->mutex))PTHREAD_MUTEX_INITIALIZER;

    if (!api_stream(id, 0, "raw", false, api_stream_data_callback, d)) {
        goto err;
    }

    *info = (struct mpv_stream_cb_info){
        .read_fn = stream_read_callback,
        .seek_fn = stream_seek_callback,
        .size_fn = stream_size_callback,
        .close_fn = stream_close_callback,
        .cancel_fn = stream_cancel_callback,

        .cookie = d,
    };

    return 0;

err:
    stream_data_free(d);
    return MPV_ERROR_LOADING_FAILED;
}


