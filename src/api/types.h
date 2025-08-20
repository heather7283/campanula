#ifndef SRC_API_TYPES_H
#define SRC_API_TYPES_H

#include <stdint.h>

#include "collections/vec.h"
#include "log.h"

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

struct api_type_artist {
    /* required */
    char *id;
    char *name;
    /* optional */
};

struct api_type_artist_id3 {
    /* required */
    char *id;
    char *name;
    int32_t album_count;
    /* optional */
};

struct api_type_album_id3 {
    /* required */
    char *id;
    char *name;
    int32_t song_count;
    int32_t duration;
    char *created;
    /* optional */
    char *artist;
    char *artist_id;
    int32_t year;
};

struct api_type_songs {
    VEC(struct api_type_child) song;
};

struct api_type_album_list {
    VEC(struct api_type_child) album;
};

struct api_type_search_result_2 {
    VEC(struct api_type_artist) artist;
    VEC(struct api_type_child) album;
    VEC(struct api_type_child) song;
};

struct api_type_search_result_3 {
    VEC(struct api_type_artist_id3) artist;
    VEC(struct api_type_album_id3) album;
    VEC(struct api_type_child) song;
};

enum subsonic_response_status {
    RESPONSE_STATUS_FAILED,
    RESPONSE_STATUS_OK,
};

enum subsonic_response_inner_object_type {
    API_TYPE_ERROR,
    API_TYPE_SONGS,
    API_TYPE_ALBUM_LIST,
    API_TYPE_SEARCH_RESULT_2,
    API_TYPE_SEARCH_RESULT_3,

    SUBSONIC_RESPONSE_INNER_OBJECT_TYPE_COUNT
};

union subsonic_response_inner_object {
    struct api_type_error error;
    struct api_type_songs songs;
    struct api_type_album_list album_list;
    struct api_type_search_result_2 search_result_2;
    struct api_type_search_result_3 search_result_3;
};

struct subsonic_response {
    enum subsonic_response_status status;
    char *version;

    enum subsonic_response_inner_object_type inner_object_type;
    union subsonic_response_inner_object inner_object;
};

void subsonic_response_free(struct subsonic_response *response);
void subsonic_response_print(const struct subsonic_response *resp, enum log_level lvl);

void print_child(const struct api_type_child *c, enum log_level lvl, int indent);
void print_artist(const struct api_type_artist *a, enum log_level lvl, int indent);
void print_artist_id3(const struct api_type_artist_id3 *a, enum log_level lvl, int indent);
void print_album_id3(const struct api_type_album_id3 *a, enum log_level lvl, int indent);

#endif /* #ifndef SRC_API_TYPES_H */

