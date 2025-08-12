#ifndef SRC_CONFIG_H
#define SRC_CONFIG_H

struct config {
    /* server url (without /rest) */
    char *server_address;
    /* server username */
    char *username;
    /* server passord */
    char *password;

    /* name that will be used in API requests and reported to the audio system */
    char *application_name;

    char *database_path;
};

extern struct config config;

#endif /* #ifndef SRC_CONFIG_H */

