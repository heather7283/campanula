#include "api.h"
#include "requests.h"
#include "config.h"
#include "auth.h"
#include "collections.h"
#include "log.h"

#define PROTOCOL_VERSION "1.16.6"

static void request_callback(CURLcode res, const void *response, void *data) {
    const ARRAY(uint8_t) *response_data = response;

    INFO("request_callback: request finished with result %d, recevied %lu bytes",
         res, ARRAY_SIZE(response_data));

    fwrite(ARRAY_DATA(response_data), 1, ARRAY_SIZE(response_data), stdout);
    fputc('\n', stdout);
    fflush(stdout);
}

[[gnu::sentinel]] /* argument list must be NULL-terminated */
static struct string build_api_request(const char *endpoint, ...) {
    struct string str = {0};

    /* TODO: all this needs to be properly URL encoded */
    string_appendf(&str, "%s/rest/%s?", config.server_address, endpoint);

    const struct auth_data *auth = get_auth_data(config.password);

    string_appendf(&str, "%s=%s&", "u", config.username);
    string_appendf(&str, "%s=%s&", "v", PROTOCOL_VERSION);
    string_appendf(&str, "%s=%s&", "c", config.application_name);
    string_appendf(&str, "%s=%s&", "f", "json");
    string_appendf(&str, "%s=%s&", "t", auth->token);
    string_appendf(&str, "%s=%s&", "s", auth->salt);

    va_list args;
    va_start(args, endpoint);
    const char *k, *v;
    while ((k = va_arg(args, const char *)) != NULL && (v = va_arg(args, const char *)) != NULL) {
        string_appendf(&str, "%s=%s&", k, v);
    }
    va_end(args);

    return str;
}

bool api_ping(void) {
    struct string url = build_api_request("ping", NULL);

    bool ret = make_request(url.str, request_callback, NULL);

    string_free(&url);
    return ret;
}

