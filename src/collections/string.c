#include <stdio.h>
#include <string.h>

#include "collections/string.h"
#include "macros.h"
#include "xmalloc.h"

#define GROWTH_FACTOR 1.5

/*
 * If you stumbled upon this code, try putting this function into godbolt with gcc -O3
 * and look at assembly it generates. Isn't it crazy how smart compilers are?
 */
static bool needs_url_encoding(char c) {
    return !((c >= 'a' && c <= 'z')
             || (c >= 'A' && c <= 'Z')
             || (c >= '0' && c <= '9')
             || c == '-' || c == '_' || c == '~' || c == '.');
}

/*
 * Null-terminates dst, returns length without null terimnator.
 * Make sure that dst is at least (src_len * 3) + 1 bytes in size.
 */
static size_t urlencode(char dst[], const char src[], size_t src_len) {
    size_t out_len = 0;
    for (size_t i = 0; i < src_len; i++) {
        const unsigned char c = src[i];
        if (needs_url_encoding(c)) {
            const unsigned char low = (c & 0x0F);
            const unsigned char high = (c >> 4);
            dst[out_len++] = '%';
            dst[out_len++] = (high < 10) ? (high + '0') : (high - 10 + 'A');
            dst[out_len++] = (low < 10) ? (low + '0') : (low - 10 + 'A');
        } else {
            dst[out_len++] = c;
        }
    }

    return out_len;
}

void string_clear(struct string *str) {
    str->len = 0;
    if (str->str != NULL) {
        str->str[0] = '\0';
    }
}

void string_free(struct string *str) {
    str->len = str->capacity = 0;
    free(str->str);
    str->str = NULL;
}

static void string_ensure_capacity(struct string *str, size_t cap) {
    if (str->capacity < cap) {
        const size_t new_cap = MAX(cap, str->capacity * GROWTH_FACTOR);
        str->str = xrealloc(str->str, new_cap);
        str->capacity = new_cap;
    }
}

size_t string_append(struct string *str, const char *suffix) {
    size_t len = strlen(suffix);

    string_ensure_capacity(str, str->len + len + 1);

    memcpy(str->str + str->len, suffix, len);
    str->len += len;
    str->str[str->len] = '\0';

    return len;
}

size_t string_append_urlencode(struct string *str, const char *suffix) {
    size_t suffix_len = strlen(suffix);

    string_ensure_capacity(str, str->len + (suffix_len * 3) + 1);

    size_t url_len = urlencode(str->str + str->len, suffix, suffix_len);
    str->len += url_len;

    return url_len;
}

int string_appendf(struct string *str, const char *fmt, ...) {
    va_list args, args_copy;
    va_start(args, fmt);

    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    string_ensure_capacity(str, str->len + len + 1);

    len = vsnprintf(str->str + str->len, str->capacity - str->len, fmt, args);
    str->len += len;

    va_end(args);

    return len;
}

