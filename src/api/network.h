#ifndef SRC_NETWORK_H
#define SRC_NETWORK_H

#include <stdint.h>

#include <curl/curl.h>

typedef void (*request_callback_t)(const char *errmsg,
                                   const char *data, size_t size,
                                   void *userdata);

bool make_request(const char *url, request_callback_t callback, void *callback_data);

bool network_init(void);
void network_cleanup(void);

#endif /* #ifndef SRC_NETWORK_H */

