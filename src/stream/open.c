#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "stream/open.h"
#include "stream/file.h"
#include "stream/network.h"
#include "db/cache.h"
#include "cleanup.h"
#include "xmalloc.h"
#include "config.h"
#include "log.h"

bool stream_open(const char *song_id, int bitrate, const char *filetype,
                 struct stream_functions *functions, void **userdata) {
    [[gnu::cleanup(cleanup_free)]] char *filepath = NULL;
    [[gnu::cleanup(cached_song_free_contents)]] struct cached_song cached_song = {0};
    size_t filesize = 0;
    int fd = -1;

    TRACE("opening stream for song %s, bitrate %d filetype %s", song_id, bitrate, filetype);

    if (!db_get_cached_song(&cached_song, song_id)) {
        DEBUG("song %s is not in cache, will fetch from server", song_id);
        goto from_network;
    }

    /* TODO: check bitrate and filetype of cached song */

    xasprintf(&filepath, "%s/%s", config.music_cache_dir, cached_song.filename);
    TRACE("opening file %s", filepath);
    fd = open(filepath, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        ERROR("failed to open direcory %s: %m", config.music_cache_dir);
        goto from_network;
    }

    /* perform basic integrity check (size) */
    struct stat stat;
    if (fstatat(fd, "", &stat, AT_EMPTY_PATH) < 0) {
        ERROR("failed to stat %s / %s: %m", config.music_cache_dir, cached_song.filename);
        goto from_network;
    }
    filesize = stat.st_size;
    if (filesize != cached_song.size) {
        ERROR("file has size %li (%lu expected), not using it", stat.st_size, cached_song.size);
        goto from_network;
    }

    /* all ok, can use this file to stream music to mpv */
    goto from_file;

from_network:
    if (fd > 0) {
        close(fd);
    }
    return stream_open_from_network(song_id, bitrate, filetype, functions, userdata);

from_file:
    DEBUG("opening song %s from cache at %s", song_id, filepath);
    db_touch_cached_song(song_id);
    return stream_open_from_fd(fd, filesize, functions, userdata);
}

