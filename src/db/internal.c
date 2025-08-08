#include <assert.h>

#include "db/internal.h"
#include "macros.h"
#include "config.h"
#include "log.h"

struct sqlite_statement statements[] = {
    [STATEMENT_BEGIN] = { .source = "BEGIN" },
    [STATEMENT_COMMIT] = { .source = "COMMIT" },
    [STATEMENT_ROLLBACK] = { .source = "ROLLBACK" },

    [STATEMENT_ENABLE_FOREIGN_KEYS] = { .source = "PRAGMA foreign_keys = ON" },

    [STATEMENT_CREATE_TABLE_ARTISTS] = { .source =
        "CREATE TABLE IF NOT EXISTS artists ( "
            "id TEXT NOT NULL PRIMARY KEY, "
            "name TEXT NOT NULL, "

            "deleted BOOLEAN DEFAULT FALSE "
        ")"
    },
    [STATEMENT_CREATE_TABLE_ALBUMS] = { .source =
        "CREATE TABLE IF NOT EXISTS albums ( "
            "id TEXT NOT NULL PRIMARY KEY, "
            "name TEXT NOT NULL, "
            "artist TEXT NOT NULL, "
            "song_count INTEGER NOT NULL, "
            "duration INTEGER NOT NULL, "
            "created DATETIME NOT NULL, "

            "deleted BOOLEAN DEFAULT FALSE, "

            "artist_id TEXT, "
            "FOREIGN KEY (artist_id) REFERENCES artists (id) "
        ")"
    },
    [STATEMENT_CREATE_TABLE_SONGS] = { .source =
        "CREATE TABLE IF NOT EXISTS songs ( "
            "id TEXT NOT NULL PRIMARY KEY, "
            "title TEXT NOT NULL, "
            "artist TEXT NOT NULL, "
            "album TEXT NOT NULL, "

            "track INTEGER, "
            "year INTEGER, "
            "duration INTEGER, "
            "bitrate INTEGER, "
            "size INTEGER, "
            "filetype TEXT, "

            "deleted BOOLEAN DEFAULT FALSE, "

            "artist_id TEXT, "
            "album_id TEXT, "
            "FOREIGN KEY (artist_id) REFERENCES artists (id), "
            "FOREIGN KEY (album_id) REFERENCES albums (id) "
        ")"
    },

    [STATEMENT_MARK_ARTISTS_AS_DELETED] = { .source = "UPDATE artists SET deleted = TRUE" },
    [STATEMENT_MARK_ALBUMS_AS_DELETED] = { .source = "UPDATE albums SET deleted = TRUE" },
    [STATEMENT_MARK_SONGS_AS_DELETED] = { .source = "UPDATE songs SET deleted = TRUE" },
    [STATEMENT_DELETE_DELETED_ARTISTS] = { .source = "DELETE FROM artists WHERE deleted = TRUE" },
    [STATEMENT_DELETE_DELETED_ALBUMS] = { .source = "DELETE FROM albums WHERE deleted = TRUE" },
    [STATEMENT_DELETE_DELETED_SONGS] = { .source = "DELETE FROM songs WHERE deleted = TRUE" },

    [STATEMENT_INSERT_ARTIST] = { .source =
        "INSERT INTO artists ( "
            "id, name "
        ") VALUES ( "
            "$id, $name "
        ") ON CONFLICT (id) DO UPDATE SET "
            "id = excluded.id, name = excluded.name, "
            "deleted = FALSE"
    },
    [STATEMENT_INSERT_ALBUM] = { .source =
        "INSERT INTO albums ( "
            "id, name, artist, artist_id, song_count, duration, created "
        ") VALUES ( "
            "$id, $name, $artist, $artist_id, $song_count, $duration, unixepoch($created) "
        ") ON CONFLICT (id) DO UPDATE SET "
            "id = excluded.id, name = excluded.name, "
            "artist = excluded.artist, artist_id = excluded.artist_id, "
            "song_count = excluded.song_count, duration = excluded.duration, "
            "deleted = FALSE "
    },
    [STATEMENT_INSERT_SONG] = { .source =
        "INSERT INTO songs ( "
            "id, title, artist, album, "
            "track, year, duration, bitrate, size, filetype, "
            "artist_id, album_id "
        ") VALUES ( "
            "$id, $title, $artist, $album, "
            "$track, $year, $duration, $bitrate, $size, $filetype, "
            "$artist_id, $album_id "
        ") ON CONFLICT (id) DO UPDATE SET "
            "title = excluded.title, "
            "artist = excluded.artist, "
            "album = excluded.album, "
            "track = excluded.track, "
            "year = excluded.year, "
            "duration = excluded.duration, "
            "bitrate = excluded.bitrate, "
            "size = excluded.size, "
            "filetype = excluded.filetype, "
            "artist_id = excluded.artist_id, "
            "album_id = excluded.album_id, "
            "deleted = FALSE "
    },

    [STATEMENT_GET_ARTISTS_WITH_PAGINATION] = { .source =
        "SELECT id, name "
        "FROM artists "
        "ORDER BY name ASC "
        "LIMIT $select_count OFFSET $select_offset"
    },
    [STATEMENT_SEARCH_ARTISTS_WITH_PAGINATION] = { .source =
        "SELECT id, name "
        "FROM artists "
        "WHERE name LIKE '%' || $query || '%' "
        "ORDER BY name ASC "
        "LIMIT $select_count OFFSET $select_offset"
    },
    [STATEMENT_GET_ALBUMS_WITH_PAGINATION] = { .source =
        "SELECT id, name, artist, artist_id, song_count, duration "
        "FROM albums "
        "ORDER BY name ASC "
        "LIMIT $select_count OFFSET $select_offset"
    },
    [STATEMENT_SEARCH_ALBUMS_WITH_PAGINATION] = { .source =
        "SELECT id, name, artist, artist_id, song_count, duration "
        "FROM albums "
        "WHERE name LIKE '%' || $query || '%' "
        "ORDER BY name ASC "
        "LIMIT $select_count OFFSET $select_offset"
    },

    [STATEMENT_GET_SONGS_IN_ALBUM] = { .source =
        "SELECT "
            "id, title, artist, album, "
            "track, year, duration, bitrate, size, filetype, "
            "artist_id, album_id  "
        "FROM songs "
        "WHERE album_id = $album_id "
        "ORDER BY track ASC"
    },
};
static_assert(SIZEOF_ARRAY(statements) == SQLITE_STATEMENT_TYPE_COUNT);

struct sqlite3 *db = NULL;

bool db_init(void) {
    int ret = 0;

    ret = sqlite3_open(config.database_path, &db);
    if (ret != SQLITE_OK) {
        ERROR("failed to open db at %s: %s", config.database_path, sqlite3_errstr(ret));
        goto err;
    }

    ret = sqlite3_exec(db, statements[STATEMENT_ENABLE_FOREIGN_KEYS].source, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to enable foreign key support: %s", sqlite3_errmsg(db));
        goto err;
    }

    ret = sqlite3_exec(db, statements[STATEMENT_CREATE_TABLE_ARTISTS].source, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to create artists table: %s", sqlite3_errmsg(db));
        goto err;
    }
    ret = sqlite3_exec(db, statements[STATEMENT_CREATE_TABLE_ALBUMS].source, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to create albums table: %s", sqlite3_errmsg(db));
        goto err;
    }
    ret = sqlite3_exec(db, statements[STATEMENT_CREATE_TABLE_SONGS].source, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to create songs table: %s", sqlite3_errmsg(db));
        goto err;
    }

    /* prepare statements */
    for (size_t i = 0; i < SIZEOF_ARRAY(statements); i++) {
        ret = sqlite3_prepare_v2(db, statements[i].source, -1, &statements[i].stmt, NULL);
        if (ret != SQLITE_OK) {
            ERROR("failed to prepare sql stmt: %s", sqlite3_errmsg(db));
            log_println(LOG_ERROR, "%s", statements[i].source);
            goto err;
        }
    }

    return true;

err:
    sqlite3_close(db);
    return false;
}

void db_cleanup(void) {
    sqlite3_close(db);
    db = NULL;
}

