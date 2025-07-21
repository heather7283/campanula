#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include <pollen.h>

#include "collections.h"
#include "log.h"
#include "xmalloc.h"

#define MSG_OUT stdout /* Send info to stdout, change to stderr if you want */

/* Global information, common to all connections */
struct global_info {
    struct pollen_loop *loop;
    struct pollen_callback *timer;
    CURLM *multi;
    int remaining;
    FILE *input;
};

/* Information associated with a specific easy handle */
struct conn_info {
    CURL *easy;
    char *url;
    struct global_info *global;
    ARRAY(uint8_t) received;
    char error[CURL_ERROR_SIZE];
};

/* Information associated with a specific socket */
struct sock_info {
    struct pollen_callback *callback;
    curl_socket_t sockfd;
    CURL *easy;
    int action;
    long timeout;
    struct global_info *global;
};

static const char fifo[] = "hiper.fifo";

/* Die if we get a bad CURLMcode somewhere */
static void mcode_or_die(const char *where, CURLMcode code) {
    #define mycase(code) \
        case code: \
            s = __STRING(code)

    if (CURLM_OK != code) {
        const char *s;
        switch (code) {
            mycase(CURLM_BAD_HANDLE);
            break;
            mycase(CURLM_BAD_EASY_HANDLE);
            break;
            mycase(CURLM_OUT_OF_MEMORY);
            break;
            mycase(CURLM_INTERNAL_ERROR);
            break;
            mycase(CURLM_UNKNOWN_OPTION);
            break;
            mycase(CURLM_LAST);
            break;
        default:
            s = "CURLM_unknown";
            break;
            mycase(CURLM_BAD_SOCKET);
            ERROR("%s returns %s", where, s);
            /* ignore this error */
            return;
        }
        ERROR("%s returns %s", where, s);
        exit(code);
    }

    #undef mycase
}

/* Update the timer after curl_multi library does its thing. Curl informs the
 * application through this callback what it wants the new timeout to be,
 * after it does some work. */
static int multi_timer_callback(CURLM *multi, long timeout_ms, void *multi_timerdata) {
    struct global_info *global_info = multi_timerdata;


    if (timeout_ms > 0) {
        DEBUG("multi_timer_callback: setting timeout to %ld ms", timeout_ms);
        pollen_timer_arm(global_info->timer, timeout_ms, 0);
    } else if (timeout_ms == 0) {
        /* libcurl wants us to timeout now, however setting both fields of
         * new_value.it_value to zero disarms the timer. The closest we can
         * do is to schedule the timer to fire in 1 ns. */
        DEBUG("multi_timer_callback: setting timeout to fire immediately");
        pollen_timer_arm_ns(global_info->timer, 1, 0);
    } else {
        DEBUG("multi_timer_callback: removing timeout");
        pollen_timer_disarm(global_info->timer);
    }

    return 0;
}

/* Check for completed transfers, and remove their easy handles */
static void check_multi_info(struct global_info *g) {
    char *eff_url;
    CURLMsg *msg;
    int msgs_left;
    struct conn_info *conn;
    CURL *easy;
    CURLcode res;

    INFO("remaining transfers: %d", g->remaining);
    while ((msg = curl_multi_info_read(g->multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            easy = msg->easy_handle;
            res = msg->data.result;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
            INFO("DONE: %s => (%d) %s", eff_url, res, conn->error);
            curl_multi_remove_handle(g->multi, easy);
            free(conn->url);
            curl_easy_cleanup(easy);

            FILE *f = fopen("received", "wb");
            if (f) {
                fwrite(ARRAY_DATA(&conn->received), 1, ARRAY_SIZE(&conn->received), f);
                fclose(f);
            }
            ARRAY_FREE(&conn->received);

            free(conn);
        }
    }
}

/* Called by pollen when we get action on a multi socket filedescriptor */
static int event_callback(struct pollen_callback *callback, int fd, uint32_t events, void *data) {
    struct global_info *g = data;
    CURLMcode rc;

    int action = ((events & EPOLLIN) ? CURL_CSELECT_IN : 0) |
                 ((events & EPOLLOUT) ? CURL_CSELECT_OUT : 0);

    rc = curl_multi_socket_action(g->multi, fd, action, &g->remaining);
    mcode_or_die("event_cb: curl_multi_socket_action", rc);

    check_multi_info(g);
    if (g->remaining <= 0) {
        DEBUG("last transfer done, removing timeout");
        pollen_timer_disarm(g->timer);
    }

    return 0;
}

/* Called by pollen when our timeout expires */
static int timer_callback(struct pollen_callback *callback, void *data) {
    struct global_info *g = data;
    CURLMcode rc;

    rc = curl_multi_socket_action(g->multi, CURL_SOCKET_TIMEOUT, 0, &g->remaining);
    mcode_or_die("timer_cb: curl_multi_socket_action", rc);
    check_multi_info(g);
    return 0;
}

/* CURLMOPT_SOCKETFUNCTION */
static int multi_socket_callback(CURL *easy, int fd, int what,
                                 void *multi_socketdata, void *private_socket_data) {
    struct global_info *global_info = multi_socketdata;
    struct sock_info *sock_info = private_socket_data;
    const char *whatstr[] = {"none", "IN", "OUT", "INOUT", "REMOVE"};

    DEBUG("multi_socket_callback: s=%d e=%p what=%s ", fd, easy, whatstr[what]);

    if (what == CURL_POLL_REMOVE) {
        pollen_loop_remove_callback(sock_info->callback);
        free(sock_info);
    } else {
        const uint32_t events = ((what & CURL_POLL_IN) ? EPOLLIN : 0) |
                                ((what & CURL_POLL_OUT) ? EPOLLOUT : 0);

        if (sock_info == NULL) {
            DEBUG("multi_socket_callback: adding data: %s", whatstr[what]);

            struct sock_info *info = xcalloc(1, sizeof(*info));
            info->global = global_info;
            info->sockfd = fd;
            info->action = what;
            info->easy = easy;
            info->callback = pollen_loop_add_fd(global_info->loop, fd, events,
                                                false, event_callback, global_info);

            curl_multi_assign(global_info->multi, fd, info);
        } else {
            DEBUG("multi_socket_callback: changing action from %s to %s",
                  whatstr[sock_info->action], whatstr[what]);

            pollen_fd_modify_events(sock_info->callback, events);
        }
    }

    return 0;
}

/* CURLOPT_WRITEFUNCTION */
static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *data) {
    struct conn_info *info = data;
    ARRAY_EXTEND(&info->received, (uint8_t *)ptr, nmemb);

    return size * nmemb;
}

/* CURLOPT_PROGRESSFUNCTION */
static int progress_callback(void *p, curl_off_t dltotal, curl_off_t dlnow,
                             curl_off_t ultotal, curl_off_t ulnow) {
    struct conn_info *conn = (struct conn_info *)p;

    TRACE("progress: %s (%li/%li)", conn->url, dlnow, dltotal);
    return 0;
}

/* Create a new easy handle, and add it to the global curl_multi */
static void new_connection(const char *url, struct global_info *g) {
    struct conn_info *conn;
    CURLMcode rc;

    conn = calloc(1, sizeof(*conn));
    conn->error[0] = '\0';

    conn->easy = curl_easy_init();
    if (!conn->easy) {
        ERROR("curl_easy_init() failed, exiting!");
        exit(2);
    }
    conn->global = g;
    conn->url = strdup(url);
    curl_easy_setopt(conn->easy, CURLOPT_URL, conn->url);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, conn);
    curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(conn->easy, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(conn->easy, CURLOPT_XFERINFODATA, conn);
    curl_easy_setopt(conn->easy, CURLOPT_ERRORBUFFER, conn->error);
    curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
    curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_TIME, 3L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_LIMIT, 10L);

    DEBUG("Adding easy %p to multi %p (%s)", conn->easy, g->multi, url);
    rc = curl_multi_add_handle(g->multi, conn->easy);
    mcode_or_die("new_conn: curl_multi_add_handle", rc);

    /* note that the add_handle() sets a timeout to trigger soon so that the
     * necessary socket_action() call gets called by this app */
}

/* This gets called whenever data is received from the fifo */
static int fifo_callback(struct pollen_callback *callback, int fd, uint32_t events, void *data) {
    struct global_info *g = data;
    char s[1024];
    long int rv = 0;
    int n = 0;

    do {
        s[0] = '\0';
        rv = fscanf(g->input, "%1023s%n", s, &n);
        s[n] = '\0';
        if (n && s[0]) {
            new_connection(s, g); /* if we read a URL, go get it! */
        } else {
            break;
        }
    } while (rv != EOF);

    return 0;
}

/* Create a named pipe and tell libevent to monitor it */
static int init_fifo(struct global_info *g) {
    struct stat st;
    curl_socket_t sockfd;

    INFO("Creating named pipe \"%s\"", fifo);
    if (lstat(fifo, &st) == 0) {
        if ((st.st_mode & S_IFMT) == S_IFREG) {
            errno = EEXIST;
            perror("lstat");
            return 1;
        }
    }
    unlink(fifo);
    if (mkfifo(fifo, 0600) == -1) {
        perror("mkfifo");
        return 1;
    }
    sockfd = open(fifo, O_RDWR | O_NONBLOCK, 0);
    if (sockfd == -1) {
        perror("open");
        return 1;
    }

    g->input = fdopen(sockfd, "r");

    pollen_loop_add_fd(g->loop, sockfd, EPOLLIN, false, fifo_callback, g);

    INFO("Now, pipe some URL's into > %s", fifo);
    return 0;
}

static void clean_fifo(struct global_info *g) {
    fclose(g->input);
    unlink(fifo);
}

int sigint_handler(struct pollen_callback *callback, int signum, void *data) {
    pollen_loop_quit(pollen_callback_get_loop(callback), 0);
    return 0;
}

int main(int argc, char **argv) {
    struct global_info g = {0};

    log_init(stderr, LOG_TRACE, false);

    g.loop = pollen_loop_create();
    g.timer = pollen_loop_add_timer(g.loop, timer_callback, &g);
    pollen_loop_add_signal(g.loop, SIGINT, sigint_handler, &g);

    if (init_fifo(&g) != 0) {
        return 1;
    }
    g.multi = curl_multi_init();

    /* setup the generic multi interface options we want */
    curl_multi_setopt(g.multi, CURLMOPT_SOCKETFUNCTION, multi_socket_callback);
    curl_multi_setopt(g.multi, CURLMOPT_SOCKETDATA, &g);
    curl_multi_setopt(g.multi, CURLMOPT_TIMERFUNCTION, multi_timer_callback);
    curl_multi_setopt(g.multi, CURLMOPT_TIMERDATA, &g);

    /* we do not call any curl_multi_socket*() function yet as we have no
       handles added! */

    pollen_loop_run(g.loop);

    curl_multi_cleanup(g.multi);
    clean_fifo(&g);
    pollen_loop_cleanup(g.loop);
    return 0;
}

