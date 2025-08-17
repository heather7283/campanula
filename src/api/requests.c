#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "api/requests.h"
#include "api/json.h"
#include "network/request.h"
#include "collections/string.h"
#include "xmalloc.h"
#include "config.h"
#include "auth.h"
#include "log.h"

static const char *api_endpoints[] = {
    [API_REQUEST_GET_RANDOM_SONGS] = "getRandomSongs",
    [API_REQUEST_GET_ALBUM_LIST] = "getAlbumList",
    [API_REQUEST_STREAM] = "stream",
    [API_REQUEST_SEARCH2] = "search2",
    [API_REQUEST_SEARCH3] = "search3",
    [API_REQUEST_SCROBBLE] = "scrobble",
};
static_assert(SIZEOF_ARRAY(api_endpoints) == API_REQUEST_TYPE_COUNT);

struct url_arg {
    const char *key;
    enum { STRING, NUMBER, BOOLEAN } type;
    union {
        const char *str;
        int64_t num;
        bool boolean;
    } val;
};

#define ARG_BUILDER(n_args) \
    struct { \
        unsigned int count; \
        struct url_arg args[n_args]; \
    }

#define ARG_BUILDER_ADD_INT(builder, k, v) \
    do { \
        assert(builder.count < SIZEOF_ARRAY(builder.args)); \
        builder.args[builder.count++] = (struct url_arg){ \
            .key = (k), .type = NUMBER, .val.num = (v), \
        }; \
    } while (0)

#define ARG_BUILDER_ADD_STR(builder, k, v) \
    do { \
        assert(builder.count < SIZEOF_ARRAY(builder.args)); \
        builder.args[builder.count++] = (struct url_arg){ \
            .key = (k), .type = STRING, .val.str = (v), \
        }; \
    } while (0)

#define ARG_BUILDER_ADD_BOOL(builder, k, v) \
    do { \
        assert(builder.count < SIZEOF_ARRAY(builder.args)); \
        builder.args[builder.count++] = (struct url_arg){ \
            .key = (k), .type = BOOLEAN, .val.boolean = (v), \
        }; \
    } while (0)

static void url_append_key_value_str(struct string *str, const char *k, const char *v) {
    string_append(str, k);
    string_append(str, "=");
    string_append_urlencode(str, v);
    string_append(str, "&");
}

static void url_append_key_value_int(struct string *str, const char *k, int64_t v) {
    string_appendf(str, "%s=%"PRIi64"&", k, v);
}

struct api_request_callback_data {
    enum api_request_type request_type;
    api_response_callback_t callback;
    void *callback_data;
};

struct api_stream_callback_data {
    enum api_request_type request_type;

    bool checked_content_type;
    bool error;
    ARRAY(char) error_data;

    api_stream_callback_t callback;
    void *callback_data;
};

static const char *error_code_to_string(int32_t code) {
    switch (code) {
    case  0: return "Generic error.";
    case 10: return "Required parameter is missing.";
    case 20: return "Incompatible Subsonic REST protocol version. Client must upgrade.";
    case 30: return "Incompatible Subsonic REST protocol version. Server must upgrade.";
    case 40: return "Wrong username or password.";
    case 41: return "Token authentication not supported for LDAP users.";
    case 50: return "User is not authorized for the given operation.";
    case 60: return "The trial period for the Subsonic server is over.";
    case 70: return "The requested data was not found.";
    default: return "Unknown error.";
    }
}

static bool on_api_stream_data(const char *errmsg, const struct response_headers *headers,
                               const void *data, ssize_t size, void *userdata) {
    struct api_stream_callback_data *d = userdata;
    const size_t expected_size = headers->content_length.present ? headers->content_length.size : 0;
    bool ret = true;

    switch (size) {
    case -1: /* error */
        d->callback(errmsg, expected_size, NULL, -1, d->callback_data);

        goto out_free;
    case 0: /* EOF */
        if (d->error) {
            struct subsonic_response *r = api_parse_response(d->request_type,
                                                             ARRAY_DATA(&d->error_data),
                                                             ARRAY_SIZE(&d->error_data));

            if (r == NULL || r->inner_object_type != API_TYPE_ERROR) {
                d->callback("Failed to parse server response",
                            expected_size, NULL, -1, d->callback_data);
            } else {
                const struct api_type_error *err = &r->inner_object.error;
                const char *errmsg;
                if (err->message != NULL) {
                    errmsg = err->message;
                } else {
                    errmsg = error_code_to_string(err->code);
                }
                d->callback(errmsg, expected_size, NULL, -1, d->callback_data);
            }

            subsonic_response_free(r);
        } else {
            d->callback(NULL, expected_size, NULL, 0, d->callback_data);
        }

        goto out_free;
    default: /* data */
        if (!d->checked_content_type) {
            const char *content_type = headers->content_type.str;
            d->error = (content_type != NULL) && STREQ(content_type, "application/json");
            d->checked_content_type = true;
        }

        if (d->error) {
            ARRAY_APPEND_N(&d->error_data, (char *)data, size);
        } else if (!d->callback(NULL, expected_size, data, size, d->callback_data)) {
            ret = false;
            goto out_free;
        }

        goto out;
    }

out_free:
    ARRAY_FREE(&d->error_data);
    free(d);

out:
    return ret;
}

static bool on_api_request_done(const char *errmsg, const struct response_headers *headers,
                                const void *data, ssize_t size, void *userdata) {
    struct api_request_callback_data *d = userdata;

    if (errmsg != NULL) {
        d->callback(errmsg, NULL, d->callback_data);
    } else {
        struct subsonic_response *response = api_parse_response(d->request_type, data, size);

        if (response == NULL) {
            d->callback("failed to parse server response", NULL, d->callback_data);
        } else if (response->inner_object_type == API_TYPE_ERROR) {
            const struct api_type_error *error = &response->inner_object.error;
            const char *error_message;

            if (error->message != NULL) {
                error_message = error->message;
            } else {
                error_message = error_code_to_string(error->code);
            }

            d->callback(error_message, NULL, d->callback_data);
        } else {
            d->callback(NULL, response, d->callback_data);
        }

        subsonic_response_free(response);
    }

    free(d);
    return true;
}

static void dummy_api_callback(const char *, const struct subsonic_response *, void *) {
    /* TODO: instead of doing this, make other functions handle NULL callback properly. */
}

static bool api_make_request(enum api_request_type request,
                             const struct url_arg *args, int args_count,
                             bool stream,
                             void *callback, void *callback_userdata) {
    struct string url = {0};

    string_appendf(&url, "%s/rest/%s?", config.server_address, api_endpoints[request]);
    url_append_key_value_str(&url, "v", API_PROTOCOL_VERSION);
    url_append_key_value_str(&url, "f", "json");
    url_append_key_value_str(&url, "c", config.application_name);

    for (int i = 0; i < args_count; i++) {
        const struct url_arg *arg = &args[i];
        switch (arg->type) {
        case STRING:
            url_append_key_value_str(&url, arg->key, arg->val.str);
            break;
        case NUMBER:
            url_append_key_value_int(&url, arg->key, arg->val.num);
            break;
        case BOOLEAN:
            url_append_key_value_str(&url, arg->key, arg->val.boolean ? "true" : "false");
            break;
        }
    }

    /* log url before adding auth data so it doesn't leak into logs */
    DEBUG("making API request: %s", url.str);

    const struct auth_data *auth = get_auth_data(config.password);
    url_append_key_value_str(&url, "u", config.username);
    url_append_key_value_str(&url, "t", auth->token);
    url_append_key_value_str(&url, "s", auth->salt);

    bool res;
    if (!stream) {
        struct api_request_callback_data *data = xcalloc(1, sizeof(*data));
        data->request_type = request;
        data->callback = callback;
        data->callback_data = callback_userdata;

        res = make_request(url.str, false, on_api_request_done, data);
    } else {
        struct api_stream_callback_data *data = xcalloc(1, sizeof(*data));
        data->request_type = request;
        data->callback = callback;
        data->callback_data = callback_userdata;

        res = make_request(url.str, true, on_api_stream_data, data);
    }

    string_free(&url);

    return res;
}

bool api_get_random_songs(int32_t size, const char *genre,
                          int32_t from_year, int32_t to_year,
                          const char *music_folder_id,
                          api_response_callback_t callback, void *callback_data) {
    ARG_BUILDER(5) args = {0};
    if (size >= 0) ARG_BUILDER_ADD_INT(args, "size", size);
    if (genre != NULL) ARG_BUILDER_ADD_STR(args, "genre", genre);
    if (from_year >= 0) ARG_BUILDER_ADD_INT(args, "fromYear", from_year);
    if (to_year >= 0) ARG_BUILDER_ADD_INT(args, "toYear", to_year);
    if (music_folder_id != NULL) ARG_BUILDER_ADD_STR(args, "musicFolderId", music_folder_id);

    return api_make_request(API_REQUEST_GET_RANDOM_SONGS,
                            args.args, args.count,
                            false,
                            callback, callback_data);

}

bool api_get_album_list(const char *type,
                        int32_t size, int32_t offset,
                        int32_t from_year, int32_t to_year,
                        const char *genre, const char *music_folder_id,
                        api_response_callback_t callback, void *callback_data) {
    ARG_BUILDER(7) args = {0};
    if (type != NULL) ARG_BUILDER_ADD_STR(args, "type", type);
    if (size >= 0) ARG_BUILDER_ADD_INT(args, "size", size);
    if (offset >= 0) ARG_BUILDER_ADD_INT(args, "offset", offset);
    if (from_year >= 0) ARG_BUILDER_ADD_INT(args, "fromYear", from_year);
    if (to_year >= 0) ARG_BUILDER_ADD_INT(args, "toYear", to_year);
    if (genre != NULL) ARG_BUILDER_ADD_STR(args, "genre", genre);
    if (music_folder_id != NULL) ARG_BUILDER_ADD_STR(args, "musicFolderId", music_folder_id);

    return api_make_request(API_REQUEST_GET_ALBUM_LIST,
                            args.args, args.count,
                            false,
                            callback, callback_data);
}

bool api_search2(const char *query,
                 int32_t artist_count, int32_t artist_offset,
                 int32_t album_count, int32_t album_offset,
                 int32_t song_count, int32_t song_offset,
                 const char *music_folder_id,
                 api_response_callback_t callback, void *callback_data) {
    ARG_BUILDER(8) args = {0};

    if (query == NULL) {
        ERROR("did not pass required parameter \"query\" to search2 api method");
        return false;
    }
    ARG_BUILDER_ADD_STR(args, "query", query);

    if (artist_count >= 0) ARG_BUILDER_ADD_INT(args, "artistCount", artist_count);
    if (artist_offset >= 0) ARG_BUILDER_ADD_INT(args, "artistOffset", artist_offset);
    if (album_count >= 0) ARG_BUILDER_ADD_INT(args, "albumCount", album_count);
    if (album_offset >= 0) ARG_BUILDER_ADD_INT(args, "albumOffset", album_offset);
    if (song_count >= 0) ARG_BUILDER_ADD_INT(args, "songCount", song_count);
    if (song_offset >= 0) ARG_BUILDER_ADD_INT(args, "songOffset", song_offset);
    if (music_folder_id != NULL) ARG_BUILDER_ADD_STR(args, "musicFolderId", music_folder_id);

    return api_make_request(API_REQUEST_SEARCH2,
                            args.args, args.count,
                            false,
                            callback, callback_data);
}

bool api_search3(const char *query,
                 int32_t artist_count, int32_t artist_offset,
                 int32_t album_count, int32_t album_offset,
                 int32_t song_count, int32_t song_offset,
                 const char *music_folder_id,
                 api_response_callback_t callback, void *callback_data) {
    ARG_BUILDER(8) args = {0};

    if (query == NULL) {
        ERROR("did not pass required parameter \"query\" to search3 api method");
        return false;
    }
    ARG_BUILDER_ADD_STR(args, "query", query);

    if (artist_count >= 0) ARG_BUILDER_ADD_INT(args, "artistCount", artist_count);
    if (artist_offset >= 0) ARG_BUILDER_ADD_INT(args, "artistOffset", artist_offset);
    if (album_count >= 0) ARG_BUILDER_ADD_INT(args, "albumCount", album_count);
    if (album_offset >= 0) ARG_BUILDER_ADD_INT(args, "albumOffset", album_offset);
    if (song_count >= 0) ARG_BUILDER_ADD_INT(args, "songCount", song_count);
    if (song_offset >= 0) ARG_BUILDER_ADD_INT(args, "songOffset", song_offset);
    if (music_folder_id != NULL) ARG_BUILDER_ADD_STR(args, "musicFolderId", music_folder_id);

    return api_make_request(API_REQUEST_SEARCH3,
                            args.args, args.count,
                            false,
                            callback, callback_data);
}

bool api_scrobble(const char *id) {
    ARG_BUILDER(2) args = {0};

    if (id == NULL || strlen(id) == 0) {
        ERROR("did not pass required parameter \"id\" to scrobble api method");
        return false;
    }
    ARG_BUILDER_ADD_STR(args, "id", id);

    ARG_BUILDER_ADD_BOOL(args, "submission", true);

    return api_make_request(API_REQUEST_SCROBBLE,
                            args.args, args.count,
                            false,
                            dummy_api_callback, NULL);
}

bool api_stream(const char *id, int32_t max_bit_rate, const char *format,
                api_stream_callback_t callback, void *callback_data) {
    ARG_BUILDER(4) args = {0};

    if (id == NULL || strlen(id) == 0) {
        ERROR("did not pass required parameter \"id\" to stream api method");
        return false;
    }
    ARG_BUILDER_ADD_STR(args, "id", id);

    if (max_bit_rate >= 0) ARG_BUILDER_ADD_INT(args, "maxBitRate", max_bit_rate);
    if (format != NULL) ARG_BUILDER_ADD_STR(args, "format", format);
    ARG_BUILDER_ADD_BOOL(args, "estimateContentLength", true); /* unconditionally */

    return api_make_request(API_REQUEST_STREAM,
                            args.args, args.count,
                            true,
                            callback, callback_data);
}

