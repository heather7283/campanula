#include <sys/random.h>
#include <string.h>

#include <openssl/md5.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include "auth.h"

#define SALT_BYTES 8
#define SALT_LENGTH (4 * ((SALT_BYTES + 2) / 3))

const struct auth_data *get_auth_data(const char *password) {
    static uint8_t salt[SALT_BYTES];
    static char salt_string[SALT_LENGTH + 1 /* null terminator */];

    static uint8_t token[MD5_DIGEST_LENGTH];
    static char token_string[(MD5_DIGEST_LENGTH * 2) + 1];

    static struct auth_data auth_data;

    /* get random salt bytes and base64 encode them */
    getrandom(salt, sizeof(salt), 0);
    EVP_EncodeBlock((uint8_t *)salt_string, salt, sizeof(salt));

    /* MD5 encode password + salt */
    MD5_CTX context;
    MD5_Init(&context);
    MD5_Update(&context, password, strlen(password));
    MD5_Update(&context, salt_string, strlen(salt_string));
    MD5_Final(token, &context);

    /* ASCII-encode MD5 hash */
    for (size_t i = 0; i < sizeof(token); i++) {
        unsigned char c = token[i];
        unsigned char low = (c & 0x0F);
        unsigned char high = (c >> 4);
        token_string[i * 2] = (high < 10) ? (high + '0') : (high - 10 + 'a');
        token_string[(i * 2) + 1] = (low < 10) ? (low + '0') : (low - 10 + 'a');
    }

    token_string[sizeof(token_string) - 1] = '\0';

    auth_data.salt = salt_string;
    auth_data.token = token_string;

    return &auth_data;
}

