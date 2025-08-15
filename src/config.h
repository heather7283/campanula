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

    /* used for stream api call */
    char *preferred_audio_format;
    int preferred_audio_bitrate;

    /* ~/.config/campanula/ */
    const char *config_dir;
    /* ~/.cache/campanula/ */
    const char *cache_dir;
    /* ~/.local/share/campanula/ */
    const char *data_dir;
};

extern struct config config;

bool load_config(void);

#endif /* #ifndef SRC_CONFIG_H */

