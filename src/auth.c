#include <sys/random.h>
#include <string.h>

#include <openssl/md5.h>

#include "auth.h"

#define SALT_BYTES 8

static void bin2hex(char dst[], const uint8_t src[], size_t src_len) {
    char *p = dst;
    for (size_t i = 0; i < src_len; i++) {
        unsigned char c = src[i];
        unsigned char low = (c & 0x0F);
        unsigned char high = (c >> 4);
        *(p++) = (high < 10) ? (high + '0') : (high - 10 + 'a');
        *(p++) = (low < 10) ? (low + '0') : (low - 10 + 'a');
    }
    *p = '\0';
}

const struct auth_data *get_auth_data(const char *password) {
    static uint8_t salt[SALT_BYTES];
    static char salt_string[(SALT_BYTES * 2) + 1];

    static uint8_t token[MD5_DIGEST_LENGTH];
    static char token_string[(MD5_DIGEST_LENGTH * 2) + 1];

    static struct auth_data auth_data;

    /* get random salt bytes and hex encode them */
    getrandom(salt, sizeof(salt), 0);
    bin2hex(salt_string, salt, sizeof(salt));

    /* MD5 encode password + salt */
    MD5_CTX context;
    MD5_Init(&context);
    MD5_Update(&context, password, strlen(password));
    MD5_Update(&context, salt_string, strlen(salt_string));
    MD5_Final(token, &context);

    /* hex encode MD5 hash */
    bin2hex(token_string, token, sizeof(token));

    auth_data.salt = salt_string;
    auth_data.token = token_string;

    return &auth_data;
}

