#ifndef SRC_NETWORK_H
#define SRC_NETWORK_H

#include <stdint.h>

#include <curl/curl.h>

typedef void (*request_callback_t)(CURLcode res, const char *data, size_t size, void *userdata);

bool make_request(const char *url, request_callback_t callback, void *callback_data);

bool curl_init(void);
void curl_cleanup(void);

#endif /* #ifndef SRC_NETWORK_H */

