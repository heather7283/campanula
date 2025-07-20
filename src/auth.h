#ifndef SRC_AUTH_H
#define SRC_AUTH_H

struct auth_data {
    const char *salt;
    const char *token;
};

const struct auth_data *get_auth_data(const char *password);

#endif /* #ifndef SRC_AUTH_H */

