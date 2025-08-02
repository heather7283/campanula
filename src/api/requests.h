#ifndef SRC_API_REQUESTS_H
#define SRC_API_REQUESTS_H

#include <sys/types.h>
#include <assert.h>

#include "api/types.h"

#define API_PROTOCOL_VERSION "1.16.6"

enum api_request_type {
    API_REQUEST_GET_RANDOM_SONGS,
    API_REQUEST_GET_ALBUM_LIST,
    API_REQUEST_STREAM,
    API_REQUEST_SEARCH2,

    /* put new types before this one, TO THE END OR EVERYTHING WILL EXPLODE */
    API_REQUEST_TYPE_COUNT,
};

typedef void (*api_response_callback_t)(const char *errmsg,
                                        const struct subsonic_response *response,
                                        void *userdata);

/* return false to cancel transfer, no more callbacks will be called after that */
typedef bool (*api_stream_callback_t)(const char *errmsg,
                                      const void *data, ssize_t data_size,
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

/*
 * Returns a list of random, newest, highest rated etc. albums.
 *
 * Parameter     Required              Default Comment
 * type          Yes                           The list type. Must be one of the following:
 *                                             random, newest, highest, frequent, recent,
 *                                             alphabeticalByName, alphabeticalByArtist,
 *                                             starred, byYear, byGenre.
 * size          No                    10      The number of albums to return. Max 500.
 * offset        No                    0       The list offset.
 * fromYear      Yes (type == byYear)          The first year in the range.
 * toYear        Yes (type == byYear)          The last year in the range.
 * genre         Yes (type == byGenre)         The name of the genre, e.g., "Rock".
 * musicFolderId No                            Only return albums in the music folder with given ID.
 */
bool api_get_album_list(const char *type,
                        uint32_t size, uint32_t offset,
                        uint32_t from_year, uint32_t to_year,
                        const char *genre, const char *music_folder_id,
                        api_response_callback_t callback, void *callback_data);

/*
 * Returns albums, artists and songs matching the given search criteria.
 * Supports paging through the result.
 *
 * Parameter     Required Default Comment
 * query         Yes              Search query.
 * artistCount   No       20      Maximum number of artists to return.
 * artistOffset  No       0       Search result offset for artists. Used for paging.
 * albumCount    No       20      Maximum number of albums to return.
 * albumOffset   No       0       Search result offset for albums. Used for paging.
 * songCount     No       20      Maximum number of songs to return.
 * songOffset    No       0       Search result offset for songs. Used for paging.
 * musicFolderId No               Self-explanatory.
 */
bool api_search2(const char *query,
                 uint32_t artist_count, uint32_t artist_offset,
                 uint32_t album_count, uint32_t album_offset,
                 uint32_t song_count, uint32_t song_offset,
                 const char *music_folder_id,
                 api_response_callback_t callback, void *callback_data);

/*
 * Streams a given media file.
 *
 * Parameter             Required Default Comment
 * id                    Yes              A string which uniquely identifies the file to stream.
 * maxBitRate            No               Limit bitrate to this value in kbps (0 for no limit).
 * format                No               Preferred target format, "raw" for no transcoding.
 * estimateContentLength No       false   Set Content-Length HTTP header to an estimated value.
 */
bool api_stream(const char *id, uint32_t max_bit_rate,
                const char *format, bool estimate_content_length,
                api_stream_callback_t callback, void *callback_data);

#endif /* #ifndef SRC_API_REQUESTS_H */

