#include <fcntl.h>
#include <sys/stat.h>

#include <pollen.h>

#include "collections.h"
#include "requests.h"
#include "log.h"
#include "xmalloc.h"

static const char fifo_name[] = "hiper.fifo";

struct pollen_loop *event_loop;

static void request_callback(CURLcode res, const void *response, void *data) {
    const ARRAY(uint8_t) *response_data = response;
    INFO("request_callback: request finished with result %d, recevied %lu bytes",
         res, ARRAY_SIZE(response_data));
}

/* This gets called whenever data is received from the fifo */
static int fifo_callback(struct pollen_callback *callback, int fd, uint32_t events, void *data) {
    FILE *fifo = data;
    char s[1024];
    long int rv = 0;
    int n = 0;

    do {
        s[0] = '\0';
        rv = fscanf(fifo, "%1023s%n", s, &n);
        s[n] = '\0';
        if (n && s[0]) {
            make_request(s, request_callback, NULL);
        } else {
            break;
        }
    } while (rv != EOF);

    return 0;
}

/* Create a named pipe and tell libevent to monitor it */
static FILE *fifo_init(const char *name) {
    INFO("creating named pipe at %s", name);

    struct stat st;
    if (lstat(name, &st) == 0) {
        if ((st.st_mode & S_IFMT) == S_IFREG) {
            ERROR("%s already exists", name);
            return NULL;
        }
    }
    unlink(name);
    if (mkfifo(name, 0600) < 0) {
        ERROR("failed to create fifo: %m");
        return NULL;
    }
    int fd = open(name, O_RDWR | O_NONBLOCK, 0);
    if (fd == -1) {
        ERROR("failed to open fifo: %m");
        return NULL;
    }

    FILE *f = fdopen(fd, "r");
    INFO("now, pipe some URL's into > %s", name);
    return f;
}

static void fifo_cleanup(FILE *stream, const char *name) {
    fclose(stream);
    unlink(name);
}

int sigint_handler(struct pollen_callback *callback, int signum, void *data) {
    pollen_loop_quit(pollen_callback_get_loop(callback), 0);
    return 0;
}

int main(int argc, char **argv) {
    log_init(stderr, LOG_TRACE, false);

    FILE *fifo = fifo_init(fifo_name);
    if (fifo == NULL) {
        return 1;
    }

    event_loop = pollen_loop_create();
    pollen_loop_add_signal(event_loop, SIGINT, sigint_handler, &event_loop);
    pollen_loop_add_fd(event_loop, fileno(fifo), EPOLLIN, false, fifo_callback, fifo);

    if (!curl_init()) {
        return 1;
    }

    pollen_loop_run(event_loop);

    curl_cleanup();
    fifo_cleanup(fifo, fifo_name);
    pollen_loop_cleanup(event_loop);
    return 0;
}

