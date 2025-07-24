#include <inttypes.h>
#include <assert.h>

#include "api/requests.h"
#include "api/json.h"
#include "xmalloc.h"
#include "config.h"
#include "collections.h"
#include "auth.h"
#include "network.h"

struct url_arg {
    const char *key;
    enum { STRING, NUMBER } type;
    union {
        const char *str;
        int64_t num;
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

static void on_api_request_done(CURLcode res, const char *data, size_t size, void *userdata) {
    struct api_request_callback_data *d = userdata;

    struct subsonic_response *response = api_parse_response(d->request_type, data, size);

    subsonic_response_free(response);
    free(d);
}

static bool api_make_request(enum api_request_type request,
                             const struct url_arg *args, int args_count,
                             api_response_callback_t callback, void *callback_userdata) {
    struct string url = {0};
    const struct auth_data *auth = get_auth_data(config.password);

    string_appendf(&url, "%s/rest/%s?", config.server_address, api_endpoints[request]);
    url_append_key_value_str(&url, "u", config.username);
    url_append_key_value_str(&url, "v", API_PROTOCOL_VERSION);
    url_append_key_value_str(&url, "c", config.application_name);
    url_append_key_value_str(&url, "f", "json");
    url_append_key_value_str(&url, "t", auth->token);
    url_append_key_value_str(&url, "s", auth->salt);

    for (int i = 0; i < args_count; i++) {
        const struct url_arg *arg = &args[i];
        switch (arg->type) {
        case STRING:
            url_append_key_value_str(&url, arg->key, arg->val.str);
            break;
        case NUMBER:
            url_append_key_value_int(&url, arg->key, arg->val.num);
            break;
        }
    }

    struct api_request_callback_data *data = xcalloc(1, sizeof(*data));
    data->request_type = request;
    data->callback = callback;
    data->callback_data = callback_userdata;

    bool res = make_request(url.str, on_api_request_done, data);

    string_free(&url);

    return res;
}

bool api_get_random_songs(uint32_t size, const char *genre,
                          uint32_t from_year, uint32_t to_year,
                          const char *music_folder_id,
                          api_response_callback_t callback, void *callback_data) {
    ARG_BUILDER(5) args = {0};
    if (size > 0) ARG_BUILDER_ADD_INT(args, "size", size);
    if (genre != NULL) ARG_BUILDER_ADD_STR(args, "genre", genre);
    if (from_year > 0) ARG_BUILDER_ADD_INT(args, "fromYear", from_year);
    if (to_year > 0) ARG_BUILDER_ADD_INT(args, "toYear", to_year);
    if (music_folder_id != NULL) ARG_BUILDER_ADD_STR(args, "musicFolderId", music_folder_id);

    return api_make_request(API_REQUEST_GET_RANDOM_SONGS,
                            args.args, args.count,
                            callback, callback_data);

}

