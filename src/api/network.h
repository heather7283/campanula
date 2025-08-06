#ifndef SRC_NETWORK_H
#define SRC_NETWORK_H

#include <stdint.h>

#include <curl/curl.h>

struct response_header {
    bool present;
    union {
        char *str;
        size_t size;
    };
};

struct response_headers {
    struct response_header content_type; /* str */
    struct response_header content_length; /* size */
};

/* for stream, return false to cancel transfer, no more callbacks will be called after that */
typedef bool (*request_callback_t)(const char *errmsg,
                                   const struct response_headers *headers,
                                   const void *data, ssize_t size,
                                   void *userdata);

bool make_request(const char *url, bool stream, request_callback_t callback, void *callback_data);

bool network_init(void);
void network_cleanup(void);

#endif /* #ifndef SRC_NETWORK_H */

