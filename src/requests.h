#ifndef SRC_REQUESTS_H
#define SRC_REQUESTS_H

#include <stdint.h>

#include <curl/curl.h>

/* response is actually an ARRAY(uint8_t) but we can't have nice things in C */
typedef void (*request_callback_t)(CURLcode res, const void *response, void *data);

bool make_request(const char *url, request_callback_t callback, void *callback_data);

bool curl_init(void);
void curl_cleanup(void);

#endif /* #ifndef SRC_REQUESTS_H */

