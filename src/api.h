#ifndef SRC_API_H
#define SRC_API_H

#include <stdint.h>

#include "collections.h"

enum api_response_status { FAILED, OK };

enum api_response_type {
    PING,
    RANDOM_SONGS,
};

enum api_media_type { MUSIC, PODCAST, AUDIOBOOK, VIDEO };

struct api_child {
    char *id;
    bool is_dir;
    char *title;
    char *album;
    char *artist;
    int32_t track;
    int32_t year;
    int32_t duration;
    int32_t bitrate;
    enum api_media_type type;
    char *album_id;
    char *artist_id;
};

struct api_response {
    enum api_response_status status;
    char *version;
    char *type;
    char *server_version;
    bool open_subsonic;

    enum api_response_type subsonic_response_type;
    union {
        /* randomSongs, songsByGenre */
        ARRAY(struct api_child) songs;
    } subsonic_response;
};

typedef void (*api_response_callback_t)(struct api_response *response, void *data);

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
bool api_get_random_songs(int size, const char *genre, int from_year,
                          int to_year, const char *music_folder_id,
                          api_response_callback_t callback, void *callback_data);

#endif /* #ifndef SRC_API_H */

