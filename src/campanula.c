/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/
/* <DESC>
 * multi socket API usage with epoll and timerfd
 * </DESC>
 */
/* Example application source code using the multi socket interface to
 * download many files at once.
 *
 * This example features the same basic functionality as hiperfifo.c does,
 * but this uses epoll and timerfd instead of libevent.
 *
 * Written by Jeff Pohlmeyer, converted to use epoll by Josh Bialkowski

Requires a Linux system with epoll

When running, the program creates the named pipe "hiper.fifo"

Whenever there is input into the fifo, the program reads the input as a list
of URL's and creates some new easy handles to fetch each URL via the
curl_multi "hiper" API.


Thus, you can try a single URL:
  % echo http://www.yahoo.com > hiper.fifo

Or a whole bunch of them:
  % cat my-url-list > hiper.fifo

The fifo buffer is handled almost instantly, so you can even add more URL's
while the previous requests are still being downloaded.

Note:
  For the sake of simplicity, URL length is limited to 1023 char's !

This is purely a demo app, all retrieved data is simply discarded by the write
callback.

*/

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <curl/curl.h>
#include <pollen.h>

#define MSG_OUT stdout /* Send info to stdout, change to stderr if you want */

/* Global information, common to all connections */
struct GlobalInfo {
    struct pollen_loop *loop;
    int tfd;    /* timer filedescriptor */
    int fifofd; /* fifo filedescriptor */
    CURLM *multi;
    int still_running;
    FILE *input;
};

/* Information associated with a specific easy handle */
struct ConnInfo {
    CURL *easy;
    char *url;
    struct GlobalInfo *global;
    char error[CURL_ERROR_SIZE];
};

/* Information associated with a specific socket */
struct SockInfo {
    struct pollen_callback *callback;
    curl_socket_t sockfd;
    CURL *easy;
    int action;
    long timeout;
    struct GlobalInfo *global;
};

#define mycase(code) \
    case code: \
        s = __STRING(code)

/* Die if we get a bad CURLMcode somewhere */
static void mcode_or_die(const char *where, CURLMcode code) {
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
            fprintf(MSG_OUT, "ERROR: %s returns %s\n", where, s);
            /* ignore this error */
            return;
        }
        fprintf(MSG_OUT, "ERROR: %s returns %s\n", where, s);
        exit(code);
    }
}

static int timer_cb(struct pollen_callback *callback, int fd, unsigned int events, void *data);

/* Update the timer after curl_multi library does its thing. Curl informs the
 * application through this callback what it wants the new timeout to be,
 * after it does some work. */
static int multi_timer_cb(CURLM *multi, long timeout_ms, struct GlobalInfo *g) {
    struct itimerspec its;

    fprintf(MSG_OUT, "multi_timer_cb: Setting timeout to %ld ms\n", timeout_ms);

    if (timeout_ms > 0) {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        its.it_value.tv_sec = timeout_ms / 1000;
        its.it_value.tv_nsec = (timeout_ms % 1000) * 1000 * 1000;
    } else if (timeout_ms == 0) {
        /* libcurl wants us to timeout now, however setting both fields of
         * new_value.it_value to zero disarms the timer. The closest we can
         * do is to schedule the timer to fire in 1 ns. */
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        its.it_value.tv_sec = 0;
        its.it_value.tv_nsec = 1;
    } else {
        memset(&its, 0, sizeof(its));
    }

    timerfd_settime(g->tfd, /* flags= */ 0, &its, NULL);
    return 0;
}

/* Check for completed transfers, and remove their easy handles */
static void check_multi_info(struct GlobalInfo *g) {
    char *eff_url;
    CURLMsg *msg;
    int msgs_left;
    struct ConnInfo *conn;
    CURL *easy;
    CURLcode res;

    fprintf(MSG_OUT, "REMAINING: %d\n", g->still_running);
    while ((msg = curl_multi_info_read(g->multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            easy = msg->easy_handle;
            res = msg->data.result;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
            fprintf(MSG_OUT, "DONE: %s => (%d) %s\n", eff_url, res,
                    conn->error);
            curl_multi_remove_handle(g->multi, easy);
            free(conn->url);
            curl_easy_cleanup(easy);
            free(conn);
        }
    }
}

/* Called by libevent when we get action on a multi socket filedescriptor */
static int event_cb(struct pollen_callback *callback, int fd, uint32_t events, void *data) {
    struct GlobalInfo *g = data;
    CURLMcode rc;
    struct itimerspec its;

    int action = ((events & EPOLLIN) ? CURL_CSELECT_IN : 0) |
                 ((events & EPOLLOUT) ? CURL_CSELECT_OUT : 0);

    rc = curl_multi_socket_action(g->multi, fd, action, &g->still_running);
    mcode_or_die("event_cb: curl_multi_socket_action", rc);

    check_multi_info(g);
    if (g->still_running <= 0) {
        fprintf(MSG_OUT, "last transfer done, kill timeout\n");
        memset(&its, 0, sizeof(its));
        timerfd_settime(g->tfd, 0, &its, NULL);
    }

    return 0;
}

/* Called by main loop when our timeout expires */
static int timer_cb(struct pollen_callback *callback, int fd, unsigned int events, void *data) {
    struct GlobalInfo *g = data;
    CURLMcode rc;
    uint64_t count = 0;
    ssize_t err = 0;

    err = read(fd, &count, sizeof(count));
    if (err == -1) {
        /* Note that we may call the timer callback even if the timerfd is not
         * readable. It's possible that there are multiple events stored in the
         * epoll buffer (i.e. the timer may have fired multiple times). The
         * event count is cleared after the first call so future events in the
         * epoll buffer fails to read from the timer. */
        if (errno == EAGAIN) {
            fprintf(MSG_OUT, "EAGAIN on tfd %d\n", g->tfd);
            return 0;
        }
    }
    if (err != sizeof(count)) {
        fprintf(stderr, "read(tfd) == %ld", err);
        perror("read(tfd)");
    }

    rc = curl_multi_socket_action(g->multi, CURL_SOCKET_TIMEOUT, 0,
                                  &g->still_running);
    mcode_or_die("timer_cb: curl_multi_socket_action", rc);
    check_multi_info(g);
    return 0;
}

/* Clean up the SockInfo structure */
static void remsock(struct SockInfo *f, struct GlobalInfo *g) {
    if (f) {
        if (f->callback) {
            pollen_loop_remove_callback(f->callback);
        }
        free(f);
    }
}

/* Assign information to a SockInfo structure */
static void setsock(struct SockInfo *f, curl_socket_t s, CURL *e, int act,
                    struct GlobalInfo *g) {
    int kind = ((act & CURL_POLL_IN) ? EPOLLIN : 0) |
               ((act & CURL_POLL_OUT) ? EPOLLOUT : 0);

    if (f->callback) {
        pollen_loop_remove_callback(f->callback);
    }

    f->sockfd = s;
    f->action = act;
    f->easy = e;
    f->callback = pollen_loop_add_fd(g->loop, s, kind, false, event_cb, g);
}

/* Initialize a new SockInfo structure */
static void addsock(curl_socket_t s, CURL *easy, int action,
                    struct GlobalInfo *g) {
    struct SockInfo *fdp = calloc(1, sizeof(struct SockInfo));

    fdp->global = g;
    setsock(fdp, s, easy, action, g);
    curl_multi_assign(g->multi, s, fdp);
}

/* CURLMOPT_SOCKETFUNCTION */
static int sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp) {
    struct GlobalInfo *g = (struct GlobalInfo *)cbp;
    struct SockInfo *fdp = (struct SockInfo *)sockp;
    const char *whatstr[] = {"none", "IN", "OUT", "INOUT", "REMOVE"};

    fprintf(MSG_OUT, "socket callback: s=%d e=%p what=%s ", s, e,
            whatstr[what]);
    if (what == CURL_POLL_REMOVE) {
        fprintf(MSG_OUT, "\n");
        remsock(fdp, g);
    } else {
        if (!fdp) {
            fprintf(MSG_OUT, "Adding data: %s\n", whatstr[what]);
            addsock(s, e, what, g);
        } else {
            fprintf(MSG_OUT, "Changing action from %s to %s\n",
                    whatstr[fdp->action], whatstr[what]);
            setsock(fdp, s, e, what, g);
        }
    }
    return 0;
}

/* CURLOPT_WRITEFUNCTION */
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data) {
    (void)ptr;
    (void)data;
    return size * nmemb;
}

/* Create a new easy handle, and add it to the global curl_multi */
static void new_conn(const char *url, struct GlobalInfo *g) {
    struct ConnInfo *conn;
    CURLMcode rc;

    conn = (struct ConnInfo *)calloc(1, sizeof(*conn));
    conn->error[0] = '\0';

    conn->easy = curl_easy_init();
    if (!conn->easy) {
        fprintf(MSG_OUT, "curl_easy_init() failed, exiting!\n");
        exit(2);
    }
    conn->global = g;
    conn->url = strdup(url);
    curl_easy_setopt(conn->easy, CURLOPT_URL, conn->url);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, conn);
    curl_easy_setopt(conn->easy, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_ERRORBUFFER, conn->error);
    curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
    curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(conn->easy, CURLOPT_PROGRESSDATA, conn);
    curl_easy_setopt(conn->easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_TIME, 3L);
    curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_LIMIT, 10L);
    fprintf(MSG_OUT, "Adding easy %p to multi %p (%s)\n", conn->easy, g->multi,
            url);
    rc = curl_multi_add_handle(g->multi, conn->easy);
    mcode_or_die("new_conn: curl_multi_add_handle", rc);

    /* note that the add_handle() sets a timeout to trigger soon so that the
     * necessary socket_action() call gets called by this app */
}

/* This gets called whenever data is received from the fifo */
static int fifo_cb(struct pollen_callback *callback, int fd, uint32_t events, void *data) {
    struct GlobalInfo *g = data;
    char s[1024];
    long int rv = 0;
    int n = 0;

    do {
        s[0] = '\0';
        rv = fscanf(g->input, "%1023s%n", s, &n);
        s[n] = '\0';
        if (n && s[0]) {
            new_conn(s, g); /* if we read a URL, go get it! */
        } else {
            break;
        }
    } while (rv != EOF);

    return 0;
}

/* Create a named pipe and tell libevent to monitor it */
static const char *fifo = "hiper.fifo";
static int init_fifo(struct GlobalInfo *g) {
    struct stat st;
    curl_socket_t sockfd;

    fprintf(MSG_OUT, "Creating named pipe \"%s\"\n", fifo);
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

    g->fifofd = sockfd;
    g->input = fdopen(sockfd, "r");

    pollen_loop_add_fd(g->loop, sockfd, EPOLLIN, false, fifo_cb, g);

    fprintf(MSG_OUT, "Now, pipe some URL's into > %s\n", fifo);
    return 0;
}

static void clean_fifo(struct GlobalInfo *g) {
    fclose(g->input);
    unlink(fifo);
}

int sigint_handler(struct pollen_callback *callback, int signum, void *data) {
    pollen_loop_quit(pollen_callback_get_loop(callback), 0);
    return 0;
}

int main(int argc, char **argv) {
    struct GlobalInfo g;
    struct itimerspec its;

    memset(&g, 0, sizeof(g));

    g.tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (g.tfd == -1) {
        perror("timerfd_create failed");
        return 1;
    }

    memset(&its, 0, sizeof(its));
    its.it_interval.tv_sec = 0;
    its.it_value.tv_sec = 1;
    timerfd_settime(g.tfd, 0, &its, NULL);

    g.loop = pollen_loop_create();
    pollen_loop_add_signal(g.loop, SIGINT, sigint_handler, &g);
    pollen_loop_add_fd(g.loop, g.tfd, EPOLLIN, false, timer_cb, &g);

    if (init_fifo(&g) != 0) {
        return 1;
    }
    g.multi = curl_multi_init();

    /* setup the generic multi interface options we want */
    curl_multi_setopt(g.multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
    curl_multi_setopt(g.multi, CURLMOPT_SOCKETDATA, &g);
    curl_multi_setopt(g.multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
    curl_multi_setopt(g.multi, CURLMOPT_TIMERDATA, &g);

    /* we do not call any curl_multi_socket*() function yet as we have no
       handles added! */

    fprintf(MSG_OUT, "Entering wait loop\n");
    fflush(MSG_OUT);

    pollen_loop_run(g.loop);

    fprintf(MSG_OUT, "Exiting normally.\n");
    fflush(MSG_OUT);

    curl_multi_cleanup(g.multi);
    clean_fifo(&g);
    pollen_loop_cleanup(g.loop);
    return 0;
}

