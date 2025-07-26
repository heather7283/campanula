#ifndef SRC_API_TYPES_H
#define SRC_API_TYPES_H

#include <stdint.h>

#include "collections/array.h"

/*
 * Types here are more or less direct translation of subsonic schema into C,
 * converted to snake_case because I despise all other code styles,
 * with some members/types omitted to avoid too much brain damage.
 * See https://www.subsonic.org/pages/inc/api/schema/subsonic-rest-api-1.16.1.xsd
 */

struct api_type_error {
    int32_t code;
    char *message;
};

struct api_type_child {
    /* required */
    char *id;
    bool is_dir;
    char *title;
    /* optional */
    char *parent;
    char *album;
    char *artist;
    int32_t track;
    int32_t year;
    int64_t size;
    char *content_type;
    char *suffix;
    int32_t duration;
    int32_t bit_rate;
    char *album_id;
    char *artist_id;
}; /* why is it called "Child" ????? */

struct api_type_songs {
    ARRAY(struct api_type_child) song;
};

struct api_type_album_list {
    ARRAY(struct api_type_child) album;
};

enum subsonic_response_inner_object_type {
    API_TYPE_ERROR,
    API_TYPE_SONGS,
    API_TYPE_ALBUM_LIST,
};

enum subsonic_response_status {
    RESPONSE_STATUS_FAILED,
    RESPONSE_STATUS_OK,
};

struct subsonic_response {
    enum subsonic_response_status status;
    char *version;

    enum subsonic_response_inner_object_type inner_object_type;
    union {
        struct api_type_error error;
        struct api_type_songs random_songs;
        struct api_type_album_list album_list;
    } inner_object;
};

void subsonic_response_free(struct subsonic_response *response);

#endif /* #ifndef SRC_API_TYPES_H */

