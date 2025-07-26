#ifndef SRC_API_REQUESTS_H
#define SRC_API_REQUESTS_H

#include <assert.h>

#include "api/types.h"
#include "macros.h"

#define API_PROTOCOL_VERSION "1.16.6"

enum api_request_type {
    API_REQUEST_GET_RANDOM_SONGS,
    API_REQUEST_TYPE_COUNT,
};

static const char *api_endpoints[] = {
    [API_REQUEST_GET_RANDOM_SONGS] = "getRandomSongs",
};
static_assert(SIZEOF_ARRAY(api_endpoints) == API_REQUEST_TYPE_COUNT);

typedef void (*api_response_callback_t)(const char *errmsg,
                                        const struct subsonic_response *response,
                                        void *userdata);

/*
 * Returns random songs matching the given criteria.
 *
 * Parameter 	 Required 	Default 	Comment
 * size          No         10          The maximum number of songs to return. Max 500.
 * genre         No                     Only returns songs belonging to this genre.
 * fromYear      No                     Only return songs published after or in this year.
 * toYear        No                     Only return songs published before or in this year.
 * musicFolderId No                     Only return songs in the music folder with the given ID.
 */
bool api_get_random_songs(uint32_t size, const char *genre,
                          uint32_t from_year, uint32_t to_year,
                          const char *music_folder_id,
                          api_response_callback_t callback, void *callback_data);

#endif /* #ifndef SRC_API_REQUESTS_H */

