#include <inttypes.h>
#include <assert.h>

#include "api/requests.h"
#include "api/json.h"
#include "collections/string.h"
#include "xmalloc.h"
#include "config.h"
#include "auth.h"
#include "log.h"
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

static void on_api_request_done(const char *errmsg, const char *data, size_t size, void *userdata) {
    struct api_request_callback_data *d = userdata;

    if (errmsg != NULL) {
        d->callback(errmsg, NULL, d->callback_data);
    } else {
        struct subsonic_response *response = api_parse_response(d->request_type, data, size);

        if (response == NULL) {
            d->callback("failed to parse server response", NULL, d->callback_data);
        } else if (response->inner_object_type == API_TYPE_ERROR) {
            if (response->inner_object.error.message != NULL) {
                d->callback(response->inner_object.error.message, NULL, d->callback_data);
            } else {
                d->callback(error_code_to_string(response->inner_object.error.code),
                            NULL, d->callback_data);
            }
        } else {
            d->callback(NULL, response, d->callback_data);
        }

        subsonic_response_free(response);
    }

    free(d);
}

static bool api_make_request(enum api_request_type request,
                             const struct url_arg *args, int args_count,
                             api_response_callback_t callback, void *callback_userdata) {
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
        }
    }

    /* log url before adding auth data so it doesn't leak into logs */
    DEBUG("making API request: %s", url.str);

    const struct auth_data *auth = get_auth_data(config.password);
    url_append_key_value_str(&url, "u", config.username);
    url_append_key_value_str(&url, "t", auth->token);
    url_append_key_value_str(&url, "s", auth->salt);

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

