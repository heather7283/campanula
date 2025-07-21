#include "urlencode.h"

/*
 * If you stumbled upon this code, try putting this function into godbolt
 * with gcc -O3 and look at assembly it generates. Isn't it crazy? I think
 * it's pretty crazy. Compilers are so insanely fucking smart nowadays.
 */
static bool needs_encoding(char c) {
    return !((c >= 'a' && c <= 'z')
             || (c >= 'A' && c <= 'Z')
             || (c >= '0' && c <= '9')
             || c == '-' || c == '_' || c == '~' || c == '.');
}

size_t urlencode(char dst[], const char src[], size_t src_len) {
    size_t out_len = 0;
    for (size_t i = 0; i < src_len; i++) {
        const unsigned char c = src[i];
        if (needs_encoding(c)) {
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

