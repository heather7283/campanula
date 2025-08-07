#ifndef SRC_CONFIG_H
#define SRC_CONFIG_H

struct config {
    char *server_address;
    char *application_name;
    char *username;
    char *password;
    char *database_path;
};

extern struct config config;

#endif /* #ifndef SRC_CONFIG_H */

