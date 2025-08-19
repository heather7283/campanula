#include <pthread.h>
#include <errno.h>

#include <curl/curl.h>

#include "network/init.h"
#include "network/request.h"
#include "network/events.h"
#include "collections/array.h"
#include "eventloop.h"
#include "xmalloc.h"
#include "log.h"

static struct network_state {
    CURLM *multi;
    struct pollen_callback *timer;

    /* some stuff for events */
    struct signal_emitter emitter;
    int n_connections;
    size_t download, upload;
    struct timespec last_event_time;

    /* libmpv might try to open network stream from foreign thread, which will call in here */
    pthread_mutex_t mutex;
} state = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};

struct connection_data {
    CURL *easy;
    char *url;
    struct response_headers headers;
    char error[CURL_ERROR_SIZE];

    bool stream;
    ARRAY(uint8_t) received;

    /* for events */
    size_t prev_download, prev_upload;

    bool cancelled;
    request_callback_t callback;
    void *callback_data;
};

struct socket_data {
    struct pollen_callback *callback;
    int sock_fd;
};

static struct timespec timespec_sub(const struct timespec *lhs, const struct timespec *rhs) {
    const struct timespec zero = {
        .tv_sec = 0,
        .tv_nsec = 0,
    };
    struct timespec ret;

    if (lhs->tv_sec < rhs->tv_sec) {
        return zero;
    }

    ret.tv_sec = lhs->tv_sec - rhs->tv_sec;

    if (lhs->tv_nsec < rhs->tv_nsec) {
        if (ret.tv_sec == 0) {
            return zero;
        }

        ret.tv_sec--;
        ret.tv_nsec = 1000000000L - rhs->tv_nsec + lhs->tv_nsec;
    } else {
        ret.tv_nsec = lhs->tv_nsec - rhs->tv_nsec;
    }

    return ret;
}

static int timespec_cmp(const struct timespec *lhs, const struct timespec *rhs) {
    if (lhs->tv_sec != rhs->tv_nsec) {
        return lhs->tv_sec > rhs->tv_sec ? 1 : -1;
    } else {
        return lhs->tv_nsec > rhs->tv_nsec ? 1 : -1;
    }
}

static size_t easy_writefunction(void *ptr, size_t size, size_t nmemb, void *data) {
    pthread_mutex_lock(&state.mutex);

    struct connection_data *conn_data = data;
    size_t ret = size * nmemb;

    if (!conn_data->headers.content_type.present) {
        struct curl_header *h;
        int ret = curl_easy_header(conn_data->easy, "Content-Type", 0, CURLH_HEADER, -1, &h);
        if (ret == CURLHE_OK) {
            conn_data->headers.content_type.present = true;
            conn_data->headers.content_type.str = xstrdup(h->value);
            TRACE("got Content-Type: %s", conn_data->headers.content_type.str);
        }
    }
    if (!conn_data->headers.content_length.present) {
        struct curl_header *h;
        int ret = curl_easy_header(conn_data->easy, "Content-Length", 0, CURLH_HEADER, -1, &h);
        if (ret == CURLHE_OK) {
            errno = 0;
            conn_data->headers.content_length.size = strtoul(h->value, NULL, 10);
            if (errno == 0) {
                conn_data->headers.content_length.present = true;
                TRACE("got Content-Length: %zu", conn_data->headers.content_length.size);
            }
        }

        if (!conn_data->stream) {
            ARRAY_RESERVE(&conn_data->received, conn_data->headers.content_length.size);
        }
    }

    if (!conn_data->stream) {
        ARRAY_APPEND_N(&conn_data->received, (uint8_t *)ptr, size * nmemb);
    } else if (!conn_data->callback(NULL, &conn_data->headers,
                                    ptr, size * nmemb,
                                    conn_data->callback_data)) {
        conn_data->cancelled = true;
        ret = CURL_WRITEFUNC_ERROR;
    }

    pthread_mutex_unlock(&state.mutex);

    return ret;
}

static int easy_xferinfofunction(void *data,
                                 curl_off_t dltotal, curl_off_t dlnow,
                                 curl_off_t ultotal, curl_off_t ulnow) {
    struct connection_data *conn = data;

    state.download += dlnow - conn->prev_download;
    conn->prev_download = dlnow;

    state.upload += ulnow - conn->prev_upload;
    conn->prev_upload = ulnow;

    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);

    const struct timespec diff = timespec_sub(&t, &state.last_event_time);
    const struct timespec thresh = { .tv_nsec = 100'000'000 };
    if (timespec_cmp(&diff, &thresh) > 0) {
        const double tdiff = (double)diff.tv_sec + (double)diff.tv_nsec / 1'000'000'000.0;

        if (state.download > 0) {
            const uint64_t speed_dl = (double)state.download / tdiff;
            signal_emit_u64(&state.emitter, NETWORK_EVENT_SPEED_DL, speed_dl);
            state.download = 0;
        }

        if (state.upload > 0) {
            const uint64_t speed_ul = (double)state.upload / tdiff;
            signal_emit_u64(&state.emitter, NETWORK_EVENT_SPEED_UL, speed_ul);
            state.upload = 0;
        }

        state.last_event_time = t;
    }

    return 0;
}

static void check_multi_info(struct network_state *global_data) {
    /* check for completed transfers */
    int msgs_left;
    CURLMsg *msg;
    while ((msg = curl_multi_info_read(global_data->multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL *easy = msg->easy_handle;
            CURLcode res = msg->data.result;

            struct connection_data *conn_data;
            const char *effective_url;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn_data);
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &effective_url);

            if (conn_data->cancelled) {
                /* no need to do anything. user doesn't want any more callbacks. */
            } else if (res != CURLE_OK) {
                const char *errmsg = NULL;
                if (conn_data->error[0] == '\0') {
                    errmsg = curl_easy_strerror(res);
                } else {
                    errmsg = conn_data->error;
                }

                /* error, both stream and regular */
                conn_data->callback(errmsg, &conn_data->headers,
                                    NULL, -1,
                                    conn_data->callback_data);
            } else if (conn_data->stream) {
                /* EOF, stream callback */
                conn_data->callback(NULL, &conn_data->headers,
                                    NULL, 0,
                                    conn_data->callback_data);
            } else {
                /* EOF, regular callback */
                conn_data->callback(NULL, &conn_data->headers,
                                    ARRAY_DATA(&conn_data->received),
                                    ARRAY_SIZE(&conn_data->received),
                                    conn_data->callback_data);
            }

            curl_multi_remove_handle(global_data->multi, easy);
            curl_easy_cleanup(easy);

            ARRAY_FREE(&conn_data->received);
            free(conn_data->url);
            if (conn_data->headers.content_type.present) {
                free(conn_data->headers.content_type.str);
            }
            free(conn_data);

            signal_emit_u64(&state.emitter, NETWORK_EVENT_CONNECTIONS,
                            --state.n_connections);
        }
    }
}

static int socket_callback(struct pollen_callback *callback, int fd, uint32_t events, void *data) {
    struct network_state *global_data = data;
    CURLMcode rc;

    const int action = \
        ((events & EPOLLIN) ? CURL_CSELECT_IN : 0) | ((events & EPOLLOUT) ? CURL_CSELECT_OUT : 0);

    /* notify curl that fd is available for reading/writing */
    int remaining;
    rc = curl_multi_socket_action(global_data->multi, fd, action, &remaining);
    if (rc != CURLM_OK) {
        ERROR("curl_multi_socket_action() failed: %s", curl_multi_strerror(rc));
        return -1;
    }

    if (remaining <= 0) {
        pollen_timer_disarm(global_data->timer);
    }

    check_multi_info(global_data);

    return 0;
}

static int timer_callback(struct pollen_callback *callback, void *data) {
    struct network_state *global_data = data;
    CURLMcode rc;

    /* notify curl that timeout has expired */
    int remaining;
    rc = curl_multi_socket_action(global_data->multi, CURL_SOCKET_TIMEOUT, 0, &remaining);
    if (rc != CURLM_OK) {
        ERROR("curl_multi_socket_action() failed: %s", curl_multi_strerror(rc));
        return -1;
    }

    check_multi_info(global_data);

    return 0;
}

static int multi_timerfunction(CURLM *multi, long timeout_ms, void *multi_timerdata) {
    pthread_mutex_lock(&state.mutex);

    struct network_state *global_data = multi_timerdata;

    if (timeout_ms > 0) {
        /* curl wants us to update timer timeout */
        pollen_timer_arm(global_data->timer, timeout_ms, 0);
    } else if (timeout_ms == 0) {
        /* curl wants us to trigger timeout immediately,
         * but setting timeout to 0 disarms the timer.
         * Schedule the timer to fire in 1 ns instead. */
        pollen_timer_arm_ns(global_data->timer, 1, 0);
    } else {
        /* curl wants us to disarm our timer */
        pollen_timer_disarm(global_data->timer);
    }

    pthread_mutex_unlock(&state.mutex);

    return 0;
}

static int multi_socketfunction(CURL *easy, int fd, int what,
                                void *multi_socketdata, void *private_socket_data) {
    pthread_mutex_lock(&state.mutex);

    struct network_state *global_data = multi_socketdata;
    struct socket_data *socket_data = private_socket_data;

    if (what == CURL_POLL_REMOVE) {
        /* curl wants us to stop monitoring fd */
        pollen_loop_remove_callback(socket_data->callback);
        free(socket_data);
    } else {
        const uint32_t events = \
            ((what & CURL_POLL_IN) ? EPOLLIN : 0) | ((what & CURL_POLL_OUT) ? EPOLLOUT : 0);

        if (socket_data == NULL) {
            /* curl notifies us about new fd we need to start monitorign */
            socket_data = xcalloc(1, sizeof(*socket_data));
            socket_data->sock_fd = fd;
            socket_data->callback = pollen_loop_add_fd(event_loop, fd, events, false,
                                                       socket_callback, global_data);

            curl_multi_assign(global_data->multi, fd, socket_data);
        } else {
            /* we are already monitoring this fd, so just change event mask here */
            pollen_fd_modify_events(socket_data->callback, events);
        }
    }

    pthread_mutex_unlock(&state.mutex);

    return 0;
}

bool make_request(const char *url, bool stream, request_callback_t callback, void *callback_data) {
    struct connection_data *conn = xcalloc(1, sizeof(*conn));
    conn->callback_data = callback_data;
    conn->callback = callback;
    conn->url = xstrdup(url);
    conn->stream = stream;

    conn->easy = curl_easy_init();
    if (conn->easy == NULL) {
        ERROR("curl_easy_init() failed");
        goto err;
    }

    curl_easy_setopt(conn->easy, CURLOPT_URL, url);
    curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
    /* setup write callback */
    curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, easy_writefunction);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, conn);
    /* setup progress callback */
    curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(conn->easy, CURLOPT_XFERINFOFUNCTION, easy_xferinfofunction);
    curl_easy_setopt(conn->easy, CURLOPT_XFERINFODATA, conn);
    /* setup buffer for storing error messages */
    curl_easy_setopt(conn->easy, CURLOPT_ERRORBUFFER, conn->error);
    /* follow HTTP 3XX redirects */
    curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1L);
    /* set connection timeout to 10 seconds */
    curl_easy_setopt(conn->easy, CURLOPT_CONNECTTIMEOUT, 10L);
    /* set speed limit for aborting transfers that are too slow */
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_TIME, 5L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_LIMIT, 10L);

    CURLMcode rc = curl_multi_add_handle(state.multi, conn->easy);
    if (rc != CURLM_OK) {
        ERROR("curl_multi_add_handle() failed: %s", curl_multi_strerror(rc));
        goto err;
    }

    signal_emit_u64(&state.emitter, NETWORK_EVENT_CONNECTIONS, ++state.n_connections);

    /* note that the add_handle() sets a timeout to trigger soon so that the
     * necessary socket_action() call gets called by this app */

    return true;

err:
    curl_easy_cleanup(conn->easy);
    free(conn->url);
    free(conn);
    return false;
}

bool network_init(void) {
    CURLcode rc;

    pthread_mutex_init(&state.mutex, NULL);
    signal_emitter_init(&state.emitter);

    rc = curl_global_init_mem(CURL_GLOBAL_ALL, xmalloc, free, xrealloc, xstrdup, xcalloc);
    if (rc != CURLE_OK) {
        ERROR("failed to init libcurl: %s", curl_easy_strerror(rc));
        return false;
    }

    state.multi = curl_multi_init();
    if (state.multi == NULL) {
        ERROR("failed to create curl multi");
        return false;
    }

    curl_multi_setopt(state.multi, CURLMOPT_SOCKETFUNCTION, multi_socketfunction);
    curl_multi_setopt(state.multi, CURLMOPT_SOCKETDATA, &state);
    curl_multi_setopt(state.multi, CURLMOPT_TIMERFUNCTION, multi_timerfunction);
    curl_multi_setopt(state.multi, CURLMOPT_TIMERDATA, &state);

    state.timer = pollen_loop_add_timer(event_loop, timer_callback, &state);
    if (state.timer == NULL) {
        ERROR("failed to create timer");
        return false;
    }

    return true;
}

void network_cleanup(void) {
    curl_multi_cleanup(state.multi);
    pollen_loop_remove_callback(state.timer);
    signal_emitter_cleanup(&state.emitter);
    pthread_mutex_destroy(&state.mutex);
}

void network_event_subscribe(struct signal_listener *listener, enum network_event events,
                             signal_callback_func_t callback, void *callback_data) {
    signal_subscribe(&state.emitter, listener, events, callback, callback_data);
}

