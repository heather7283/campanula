#include <assert.h>
#include <string.h>

#include <json-c/json.h>

#include "api/json.h"
#include "api/types.h"
#include "xmalloc.h"
#include "macros.h"
#include "log.h"

#define JSON_ERROR(error, ...) \
    ({ \
        ERROR("error parsing server response: " error __VA_OPT__(,) __VA_ARGS__); \
        goto err; \
    })

#define JSON_WARN(message, ...) \
    WARN("while parsing server response: " message __VA_OPT__(,) __VA_ARGS__)

#define JSON_ERROR_IF(cond, error, ...) \
    if (cond) JSON_ERROR(error __VA_OPT__(,) __VA_ARGS__)

#define JSON_ERROR_IF_NULL(val, error, ...) \
    JSON_ERROR_IF((val) == NULL, error __VA_OPT__(,) __VA_ARGS__)

#define JSON_CHECK_TYPE_OR_FAIL(obj, type) \
    ({ \
        if (!json_object_is_type((obj), json_type_##type)) { \
            JSON_ERROR("got type %s where %s was expected", \
                       json_type_to_name(json_type_##type), \
                       json_type_to_name(json_object_get_type(obj))); \
        } \
    })

#define JSON_GET_INTERNAL(obj, type, key, fail) \
    ({ \
        struct json_object *tmp = NULL; \
        if (!json_object_object_get_ex((obj), (key), &tmp)) { \
            if (fail) { \
                JSON_ERROR("did not find key \"%s\"", (key)); \
                goto err; \
            } \
        } else { \
            const enum json_type t = json_object_get_type(tmp); \
            if (t != json_type_##type) { \
                if (fail) { \
                    JSON_ERROR("\"%s\" is of unexpected type %s, %s expected", \
                               (key), json_type_to_name(t), json_type_to_name(json_type_##type)); \
                    goto err; \
                } \
            } \
        } \
        tmp; \
    })

#define JSON_GET_OR_FAIL(obj, type, key) \
    JSON_GET_INTERNAL(obj, type, key, true)

#define JSON_GET(obj, type, key) \
    JSON_GET_INTERNAL(obj, type, key, false)

#define JSON_GET_VALUE_OR_FAIL(obj, type, key) \
    ({ \
        struct json_object *tmp = JSON_GET_OR_FAIL(obj, type, key); \
        json_object_get_##type(tmp); \
    })

#define JSON_GET_VALUE(obj, type, key) \
    ({ \
        struct json_object *tmp = JSON_GET(obj, type, key); \
        json_object_get_##type(tmp); \
    })

static bool parse_child(struct api_type_child *child, const struct json_object *json) {
    /* required */
    child->id = xstrdup(JSON_GET_VALUE_OR_FAIL(json, string, "id"));
    child->is_dir = JSON_GET_VALUE_OR_FAIL(json, boolean, "isDir");
    child->title = xstrdup(JSON_GET_VALUE_OR_FAIL(json, string, "title"));

    child->parent = xstrdup(JSON_GET_VALUE(json, string, "parent"));
    child->album = xstrdup(JSON_GET_VALUE(json, string, "album"));
    child->artist = xstrdup(JSON_GET_VALUE(json, string, "artist"));
    child->track = JSON_GET_VALUE(json, int, "track");
    child->year = JSON_GET_VALUE(json, int, "year");
    child->size = JSON_GET_VALUE(json, int, "size");
    /* TODO: enum */
    child->content_type = xstrdup(JSON_GET_VALUE(json, string, "contentType"));
    child->suffix = xstrdup(JSON_GET_VALUE(json, string, "suffix"));
    child->duration = JSON_GET_VALUE(json, int, "duration");
    child->bit_rate = JSON_GET_VALUE(json, int, "bitRate");
    child->album_id = xstrdup(JSON_GET_VALUE(json, string, "albumId"));
    child->artist_id = xstrdup(JSON_GET_VALUE(json, string, "artistId"));

    return true;

err:
    return false;
}

static bool parse_artist(struct api_type_artist *artist, const struct json_object *json) {
    /* required */
    artist->id = xstrdup(JSON_GET_VALUE_OR_FAIL(json, string, "id"));
    artist->name = xstrdup(JSON_GET_VALUE_OR_FAIL(json, string, "name"));

    return true;

err:
    return false;
}

static bool parse_type_songs(struct api_type_songs *songs,
                             const struct json_object *json) {
    const struct json_object *song = JSON_GET_OR_FAIL(json, array, "song");

    ARRAY_INIT(&songs->song);

    const size_t array_len = json_object_array_length(song);
    for (size_t i = 0; i < array_len; i++) {
        const struct json_object *elem = json_object_array_get_idx(song, i);
        JSON_CHECK_TYPE_OR_FAIL(elem, object);

        struct api_type_child *child = ARRAY_EMPLACE_BACK_ZEROED(&songs->song);
        if (!parse_child(child, elem)) {
            return false;
        }
    }

    return true;

err:
    return false;
}

static bool parse_type_album_list(struct api_type_album_list *album_list,
                                  const struct json_object *json) {
    const struct json_object *album = JSON_GET_OR_FAIL(json, array, "album");

    ARRAY_INIT(&album_list->album);

    const size_t array_len = json_object_array_length(album);
    for (size_t i = 0; i < array_len; i++) {
        const struct json_object *elem = json_object_array_get_idx(album, i);
        JSON_CHECK_TYPE_OR_FAIL(elem, object);

        struct api_type_child *child = ARRAY_EMPLACE_BACK_ZEROED(&album_list->album);
        if (!parse_child(child, elem)) {
            return false;
        }
    }

    return true;

err:
    return false;
}

static bool parse_type_search_result_2(struct api_type_search_result_2 *sr2,
                                       const struct json_object *json) {
    ARRAY_INIT(&sr2->artist);
    ARRAY_INIT(&sr2->album);
    ARRAY_INIT(&sr2->song);

    const struct json_object *artist = NULL;
    if ((artist = JSON_GET(json, array, "artist"))) {
        for (size_t i = 0; i < json_object_array_length(artist); i++) {
            const struct json_object *elem = json_object_array_get_idx(artist, i);
            JSON_CHECK_TYPE_OR_FAIL(elem, object);

            struct api_type_artist *artist = ARRAY_EMPLACE_BACK_ZEROED(&sr2->artist);
            if (!parse_artist(artist, elem)) {
                goto err;
            }
        }
    }

    const struct json_object *album = NULL;
    if ((album = JSON_GET(json, array, "album"))) {
        for (size_t i = 0; i < json_object_array_length(album); i++) {
            const struct json_object *elem = json_object_array_get_idx(album, i);
            JSON_CHECK_TYPE_OR_FAIL(elem, object);

            struct api_type_child *child = ARRAY_EMPLACE_BACK_ZEROED(&sr2->album);
            if (!parse_child(child, elem)) {
                goto err;
            }
        }
    }

    const struct json_object *song = NULL;
    if ((song = JSON_GET(json, array, "song"))) {
        for (size_t i = 0; i < json_object_array_length(song); i++) {
            const struct json_object *elem = json_object_array_get_idx(song, i);
            JSON_CHECK_TYPE_OR_FAIL(elem, object);

            struct api_type_child *child = ARRAY_EMPLACE_BACK_ZEROED(&sr2->song);
            if (!parse_child(child, elem)) {
                goto err;
            }
        }
    }

    return true;

err:
    return false;
}

static bool parse_response_search2(struct subsonic_response *resp,
                                   const struct json_object *json) {
    resp->inner_object_type = API_TYPE_SEARCH_RESULT_2;
    return parse_type_search_result_2(&resp->inner_object.search_result_2, json);
}

static bool parse_response_random_songs(struct subsonic_response *resp,
                                        const struct json_object *json) {
    resp->inner_object_type = API_TYPE_SONGS;
    return parse_type_songs(&resp->inner_object.songs, json);
}

static bool parse_response_album_list(struct subsonic_response *resp,
                                      const struct json_object *json) {
    resp->inner_object_type = API_TYPE_ALBUM_LIST;
    return parse_type_album_list(&resp->inner_object.album_list, json);
}

static bool parse_error(struct api_type_error *err, const struct json_object *json) {
    err->message = xstrdup(JSON_GET_VALUE_OR_FAIL(json, string, "message"));
    err->code = JSON_GET_VALUE_OR_FAIL(json, int, "code");

    return true;

err:
    return false;
}

typedef bool (*inner_object_parser_t)(struct subsonic_response *resp,
                                      const struct json_object *json);

static const inner_object_parser_t inner_object_parsers[] = {
    [API_REQUEST_GET_RANDOM_SONGS] = parse_response_random_songs,
    [API_REQUEST_GET_ALBUM_LIST] = parse_response_album_list,
    [API_REQUEST_STREAM] = NULL, /* special */
    [API_REQUEST_SEARCH2] = parse_response_search2,
};
static_assert(SIZEOF_ARRAY(inner_object_parsers) == API_REQUEST_TYPE_COUNT);

static const char *inner_object_names[] = {
    [API_REQUEST_GET_RANDOM_SONGS] = "randomSongs",
    [API_REQUEST_GET_ALBUM_LIST] = "albumList",
    [API_REQUEST_STREAM] = NULL, /* special */
    [API_REQUEST_SEARCH2] = "searchResult2",
};
static_assert(SIZEOF_ARRAY(inner_object_names) == API_REQUEST_TYPE_COUNT);

struct subsonic_response *api_parse_response(enum api_request_type request,
                                             const char *data, size_t data_size) {
    struct subsonic_response *resp = xcalloc(1, sizeof(*resp));

    struct json_tokener *parser = json_tokener_new();
    struct json_object *json = json_tokener_parse_ex(parser, data, data_size);
    const enum json_tokener_error err = json_tokener_get_error(parser);
    JSON_ERROR_IF(err != json_tokener_success, "%s", json_tokener_error_desc(err));
    JSON_CHECK_TYPE_OR_FAIL(json, object);

    const struct json_object *root = JSON_GET_OR_FAIL(json, object, "subsonic-response");

    resp->version = xstrdup(JSON_GET_VALUE_OR_FAIL(root, string, "version"));

    const char *status_str = JSON_GET_VALUE_OR_FAIL(root, string, "status");
    if (STREQ(status_str, "failed")) {
        resp->status = RESPONSE_STATUS_FAILED;
    } else if (STREQ(status_str, "ok")) {
        resp->status = RESPONSE_STATUS_OK;
    } else {
        JSON_ERROR("invalid status: %s", status_str);
    }

    switch (resp->status) {
    case RESPONSE_STATUS_FAILED:
        const struct json_object *error = JSON_GET_OR_FAIL(root, object, "error");

        JSON_ERROR_IF(!parse_error(&resp->inner_object.error, error), "failed to parse \"error\"");
        break;
    case RESPONSE_STATUS_OK:
        const char *inner_object_name = inner_object_names[request];
        const inner_object_parser_t inner_object_parser = inner_object_parsers[request];

        const struct json_object *inner = JSON_GET_OR_FAIL(root, object, inner_object_name);

        JSON_ERROR_IF(!inner_object_parser(resp, inner),
                      "failed to parse \"%s\"", inner_object_name);
        break;
    }

    json_object_put(json);
    json_tokener_free(parser);

    return resp;

err:
    ERROR("raw json: %.*s", (int)data_size, data);
    json_object_put(json);
    json_tokener_free(parser);
    subsonic_response_free(resp);
    return NULL;
}

