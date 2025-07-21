#include "api.h"
#include "requests.h"
#include "config.h"
#include "auth.h"
#include "xmalloc.h"
#include "collections.h"
#include "log.h"

#define PROTOCOL_VERSION "1.16.6"

struct api_request_callback_data {
    api_response_callback_t callback;
    void *callback_data;
};

static void api_request_callback(CURLcode res, const void *response, void *data) {
    const ARRAY(uint8_t) *response_data = response;
    struct api_request_callback_data *request_data = data;

    INFO("request_callback: request finished with result %d, recevied %lu bytes",
         res, ARRAY_SIZE(response_data));

    fwrite(ARRAY_DATA(response_data), 1, ARRAY_SIZE(response_data), stdout);
    fputc('\n', stdout);
    fflush(stdout);

    free(request_data);
}

static const char *int2str(int n) {
    static char str[12];
    snprintf(str, sizeof(str), "%d", n);
    return str;
}

static void url_append_key_value(struct string *str, const char *k, const char *v) {
    string_append(str, k);
    string_append(str, "=");
    string_append_urlencode(str, v);
    string_append(str, "&");
}

static struct string build_api_request_url(const char *endpoint) {
    struct string str = {0};

    string_appendf(&str, "%s/rest/%s?", config.server_address, endpoint);

    const struct auth_data *auth = get_auth_data(config.password);

    url_append_key_value(&str, "u", config.username);
    url_append_key_value(&str, "v", PROTOCOL_VERSION);
    url_append_key_value(&str, "c", config.application_name);
    url_append_key_value(&str, "f", "json");
    url_append_key_value(&str, "t", auth->token);
    url_append_key_value(&str, "s", auth->salt);

    return str;
}

bool api_get_random_songs(int size, const char *genre, int from_year,
                          int to_year, const char *music_folder_id,
                          api_response_callback_t callback, void *callback_data) {
    struct string url = build_api_request_url("getRandomSongs");
    if (size) {
        url_append_key_value(&url, "size", int2str(size));
    }
    if (genre) {
        url_append_key_value(&url, "genre", genre);
    }
    if (from_year) {
        url_append_key_value(&url, "fromYear", int2str(size));
    }
    if (to_year) {
        url_append_key_value(&url, "toYear", int2str(size));
    }
    if (music_folder_id) {
        url_append_key_value(&url, "musicFolderId", music_folder_id);
    }

    struct api_request_callback_data *data = xcalloc(1, sizeof(*data));
    data->callback = callback;
    data->callback_data = callback_data;

    bool ret = make_request(url.str, api_request_callback, data);

    string_free(&url);
    return ret;
}

