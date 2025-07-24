#ifndef SRC_API_JSON_H
#define SRC_API_JSON_H

#include "api/requests.h"

struct subsonic_response *api_parse_response(enum api_request_type request,
                                             const char *data, size_t data_size);

#endif /* #ifndef SRC_API_JSON_H */

