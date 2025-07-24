#include "collections.h"
#include "network.h"
#include "campanula.h"
#include "xmalloc.h"
#include "log.h"

static struct curl_global_data {
    CURLM *multi;
    struct pollen_callback *timer;
} curl_global;

struct connection_data {
    CURL *easy;
    char *url;
    ARRAY(uint8_t) received;
    char error[CURL_ERROR_SIZE];

    request_callback_t callback;
    void *callback_data;
};

struct socket_data {
    struct pollen_callback *callback;
    int sock_fd;
};

static size_t easy_writefunction(void *ptr, size_t size, size_t nmemb, void *data) {
    struct connection_data *conn = data;
    ARRAY_EXTEND(&conn->received, (uint8_t *)ptr, nmemb);

    return size * nmemb;
}

static int easy_xferinfofunction(void *data,
                                 curl_off_t dltotal, curl_off_t dlnow,
                                 curl_off_t ultotal, curl_off_t ulnow) {
    struct connection_data *conn = data;
    TRACE("progress: %s (%li/%li) DL (%li/%li) UL",
          conn->url, dlnow, dltotal, ulnow, ultotal);

    return 0;
}

static void check_multi_info(struct curl_global_data *global_data) {
    /* check for completed transfers */
    int msgs_left;
    CURLMsg *msg;
    while ((msg = curl_multi_info_read(global_data->multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL *easy = msg->easy_handle;
            CURLcode res = msg->data.result;

            struct connection_data *conn;
            const char *effective_url;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &effective_url);
            INFO("DONE: %s => (%d) %s", effective_url, res, conn->error);

            curl_multi_remove_handle(global_data->multi, easy);
            curl_easy_cleanup(easy);

            conn->callback(res, (char *)ARRAY_DATA(&conn->received),
                           ARRAY_SIZE(&conn->received), conn->callback_data);

            ARRAY_FREE(&conn->received);
            free(conn->url);
            free(conn);
        }
    }
}

static int socket_callback(struct pollen_callback *callback, int fd, uint32_t events, void *data) {
    struct curl_global_data *global_data = data;
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
        DEBUG("last transfer done, removing timeout");
        pollen_timer_disarm(global_data->timer);
    }

    check_multi_info(global_data);

    return 0;
}

static int timer_callback(struct pollen_callback *callback, void *data) {
    struct curl_global_data *global_data = data;
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
    struct curl_global_data *global_data = multi_timerdata;

    if (timeout_ms > 0) {
        /* curl wants us to update timer timeout */
        DEBUG("multi_timerfunction: setting timeout to %ld ms", timeout_ms);
        pollen_timer_arm(global_data->timer, timeout_ms, 0);
    } else if (timeout_ms == 0) {
        /* curl wants us to trigger timeout immediately,
         * but setting timeout to 0 disarms the timer.
         * Schedule the timer to fire in 1 ns instead. */
        DEBUG("multi_timerfunction: setting timeout to fire immediately");
        pollen_timer_arm_ns(global_data->timer, 1, 0);
    } else {
        /* curl wants us to disarm our timer */
        DEBUG("multi_timerfunction: removing timeout");
        pollen_timer_disarm(global_data->timer);
    }

    return 0;
}

static int multi_socketfunction(CURL *easy, int fd, int what,
                                void *multi_socketdata, void *private_socket_data) {
    struct curl_global_data *global_data = multi_socketdata;
    struct socket_data *socket_data = private_socket_data;
    const char *whatstr[] = {"none", "IN", "OUT", "INOUT", "REMOVE"};

    DEBUG("multi_socketfunction: fd %d %s ", fd, whatstr[what]);

    if (what == CURL_POLL_REMOVE) {
        /* curl wants us to stop monitoring fd */
        DEBUG("multi_socketfunction: removing fd %d", fd);

        pollen_loop_remove_callback(socket_data->callback);
        free(socket_data);
    } else {
        const uint32_t events = \
            ((what & CURL_POLL_IN) ? EPOLLIN : 0) | ((what & CURL_POLL_OUT) ? EPOLLOUT : 0);

        if (socket_data == NULL) {
            /* curl notifies us about new fd we need to start monitorign */
            DEBUG("multi_socketfunction: setting up callback for fd %d", fd);

            socket_data = xcalloc(1, sizeof(*socket_data));
            socket_data->sock_fd = fd;
            socket_data->callback = pollen_loop_add_fd(event_loop, fd, events, false,
                                                       socket_callback, global_data);

            curl_multi_assign(global_data->multi, fd, socket_data);
        } else {
            /* we are already monitoring this fd, so just change event mask here */
            DEBUG("multi_socketfunction: changing action for fd %d to %s", fd, whatstr[what]);

            pollen_fd_modify_events(socket_data->callback, events);
        }
    }

    return 0;
}

bool make_request(const char *url, request_callback_t callback, void *callback_data) {
    struct connection_data *conn = xcalloc(1, sizeof(*conn));
    conn->callback_data = callback_data;
    conn->callback = callback;
    conn->url = xstrdup(url);

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
    curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_XFERINFOFUNCTION, easy_xferinfofunction);
    curl_easy_setopt(conn->easy, CURLOPT_XFERINFODATA, conn);
    /* setup buffer for storing error messages */
    curl_easy_setopt(conn->easy, CURLOPT_ERRORBUFFER, conn->error);
    /* follow HTTP 3XX redirects */
    curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1L);
    /* set speed limit for aborting transfers that are too slow */
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_TIME, 3L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_LIMIT, 10L);

    TRACE("Adding easy with url %s to multi", url);
    CURLMcode rc = curl_multi_add_handle(curl_global.multi, conn->easy);
    if (rc != CURLM_OK) {
        ERROR("curl_multi_add_handle() failed: %s", curl_multi_strerror(rc));
        goto err;
    }

    /* note that the add_handle() sets a timeout to trigger soon so that the
     * necessary socket_action() call gets called by this app */

    return true;

err:
    curl_easy_cleanup(conn->easy);
    free(conn->url);
    free(conn);
    return false;
}

bool curl_init(void) {
    CURLcode rc;

    rc = curl_global_init_mem(CURL_GLOBAL_ALL, xmalloc, free, xrealloc, xstrdup, xcalloc);
    if (rc != CURLE_OK) {
        ERROR("failed to init libcurl: %s", curl_easy_strerror(rc));
        return false;
    }

    curl_global.multi = curl_multi_init();
    if (curl_global.multi == NULL) {
        ERROR("failed to create curl multi");
        return false;
    }

    curl_multi_setopt(curl_global.multi, CURLMOPT_SOCKETFUNCTION, multi_socketfunction);
    curl_multi_setopt(curl_global.multi, CURLMOPT_SOCKETDATA, &curl_global);
    curl_multi_setopt(curl_global.multi, CURLMOPT_TIMERFUNCTION, multi_timerfunction);
    curl_multi_setopt(curl_global.multi, CURLMOPT_TIMERDATA, &curl_global);

    curl_global.timer = pollen_loop_add_timer(event_loop, timer_callback, &curl_global);
    if (curl_global.timer == NULL) {
        ERROR("failed to create timer");
        return false;
    }

    return true;
}

void curl_cleanup(void) {
    curl_multi_cleanup(curl_global.multi);
    pollen_loop_remove_callback(curl_global.timer);
}

