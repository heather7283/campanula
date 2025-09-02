#include <assert.h>

#include "db/internal.h"
#include "cleanup.h"
#include "xmalloc.h"
#include "macros.h"
#include "config.h"
#include "log.h"

struct sqlite_statement statements[] = {
    [STATEMENT_ENABLE_FOREIGN_KEYS] = { .src = "PRAGMA foreign_keys = ON" },

    [STATEMENT_BEGIN] = { .src = "BEGIN" },
    [STATEMENT_COMMIT] = { .src = "COMMIT" },
    [STATEMENT_ROLLBACK] = { .src = "ROLLBACK" },

    [STATEMENT_CREATE_TABLE_SERVERS] = { .src =
        "CREATE TABLE IF NOT EXISTS servers ( "
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "url TEXT NOT NULL UNIQUE, "
            "last_sync DATETIME NOT NULL DEFAULT 0 "
        ")"
    },
    [STATEMENT_CREATE_TABLE_ARTISTS] = { .src =
        "CREATE TABLE IF NOT EXISTS artists ( "
            "id TEXT NOT NULL, "
            "name TEXT NOT NULL, "

            "deleted BOOLEAN DEFAULT FALSE, "

            "server_id INTEGER NOT NULL, "

            "FOREIGN KEY ( server_id ) REFERENCES servers ( id ), "
            "PRIMARY KEY ( id, server_id ) "
        ")"
    },
    [STATEMENT_CREATE_TABLE_ALBUMS] = { .src =
        "CREATE TABLE IF NOT EXISTS albums ( "
            "id TEXT NOT NULL, "
            "name TEXT NOT NULL, "
            "artist TEXT NOT NULL, "
            "song_count INTEGER NOT NULL, "
            "duration INTEGER NOT NULL, "
            "created DATETIME NOT NULL, "

            "deleted BOOLEAN DEFAULT FALSE, "

            "server_id INTEGER NOT NULL, "
            "artist_id TEXT, "

            "FOREIGN KEY ( server_id ) REFERENCES servers ( id ), "
            "FOREIGN KEY ( artist_id, server_id ) REFERENCES artists ( id, server_id ), "
            "PRIMARY KEY ( id, server_id ) "
        ")"
    },
    [STATEMENT_CREATE_TABLE_SONGS] = { .src =
        "CREATE TABLE IF NOT EXISTS songs ( "
            "id TEXT NOT NULL, "
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

            "server_id INTEGER NOT NULL, "
            "artist_id TEXT, "
            "album_id TEXT, "

            "FOREIGN KEY ( server_id ) REFERENCES servers ( id ), "
            "FOREIGN KEY ( artist_id, server_id ) REFERENCES artists ( id, server_id ), "
            "FOREIGN KEY ( album_id, server_id ) REFERENCES albums ( id, server_id ), "
            "PRIMARY KEY ( id, server_id ) "
        ")"
    },
    [STATEMENT_CREATE_TABLE_CACHED_SONGS] = { .src =
        "CREATE TABLE IF NOT EXISTS cached_songs ( "
            "id TEXT NOT NULL, "
            "filename TEXT NOT NULL, "
            "filetype TEXT NOT NULL, "
            "bitrate INTEGER NOT NULL, "
            "size INTEGER NOT NULL, "
            "accessed DATETIME NOT NULL DEFAULT (unixepoch('now')), "

            "server_id INTEGER NOT NULL, "

            "FOREIGN KEY ( id, server_id ) REFERENCES songs ( id, server_id ) ON DELETE CASCADE "
            "PRIMARY KEY ( id, server_id ) "
        ")"
    },

    [STATEMENT_INSERT_SERVER] = { .src =
        "INSERT INTO servers ( url ) VALUES ( $url ) RETURNING id"
    },
    [STATEMENT_GET_SERVER_ID] = { .src =
        "SELECT id FROM servers WHERE url = $url"
    },
    [STATEMENT_GET_SERVER_LAST_SYNC] = { .src =
        "SELECT last_sync FROM servers WHERE id = $id"
    },

    [STATEMENT_UPDATE_SERVER_LAST_SYNC] = { .src =
        "UPDATE servers SET last_sync = unixepoch('now') WHERE id = $id"
    },

    [STATEMENT_MARK_ARTISTS_AS_DELETED] = { .src =
        "UPDATE artists SET deleted = TRUE WHERE server_id = $server_id"
    },
    [STATEMENT_MARK_ALBUMS_AS_DELETED] = { .src =
        "UPDATE albums SET deleted = TRUE WHERE server_id = $server_id"
    },
    [STATEMENT_MARK_SONGS_AS_DELETED] = { .src =
        "UPDATE songs SET deleted = TRUE WHERE server_id = $server_id"
    },
    [STATEMENT_DELETE_DELETED_ARTISTS] = { .src =
        "DELETE FROM artists WHERE ( deleted = TRUE AND server_id = $server_id )"
    },
    [STATEMENT_DELETE_DELETED_ALBUMS] = { .src =
        "DELETE FROM albums WHERE ( deleted = TRUE AND server_id = $server_id )"
    },
    [STATEMENT_DELETE_DELETED_SONGS] = { .src =
        "DELETE FROM songs WHERE ( deleted = TRUE AND server_id = $server_id )"
    },

    /* The following will break if, let's say, a song "foo" with ID "123" is deleted
     * on the remote server, and then another song "bar" is added with the same ID.
     * The old song will remain in our database and new one will never be picked up. */
    [STATEMENT_INSERT_ARTIST] = { .src =
        "INSERT INTO artists ( "
            "id, name, server_id "
        ") VALUES ( "
            "$id, $name, $server_id "
        ") ON CONFLICT DO UPDATE SET deleted = FALSE"
    },
    [STATEMENT_INSERT_ALBUM] = { .src =
        "INSERT INTO albums ( "
            "id, name, artist, song_count, duration, created, "
            "artist_id, server_id "
        ") VALUES ( "
            "$id, $name, $artist, $song_count, $duration, unixepoch($created), "
            "$artist_id, $server_id "
        ") ON CONFLICT DO UPDATE SET deleted = FALSE"
    },
    [STATEMENT_INSERT_SONG] = { .src =
        "INSERT INTO songs ( "
            "id, title, artist, album, "
            "track, year, duration, bitrate, size, filetype, "
            "artist_id, album_id, server_id "
        ") VALUES ( "
            "$id, $title, $artist, $album, "
            "$track, $year, $duration, $bitrate, $size, $filetype, "
            "$artist_id, $album_id, $server_id "
        ") ON CONFLICT DO UPDATE SET deleted = FALSE"
    },

    [STATEMENT_GET_ARTISTS_WITH_PAGINATION] = { .src =
        "SELECT id, name "
        "FROM artists "
        "WHERE server_id = $server_id "
        "ORDER BY name ASC "
        "LIMIT $select_count OFFSET $select_offset"
    },
    [STATEMENT_SEARCH_ARTISTS_WITH_PAGINATION] = { .src =
        "SELECT id, name "
        "FROM artists "
        "WHERE ( ( server_id = $server_id ) AND ( name LIKE '%' || $query || '%' ) )"
        "ORDER BY name ASC "
        "LIMIT $select_count OFFSET $select_offset"
    },
    [STATEMENT_GET_ALBUMS_WITH_PAGINATION] = { .src =
        "SELECT id, name, artist, artist_id, song_count, duration "
        "FROM albums "
        "WHERE server_id = $server_id "
        "ORDER BY created DESC "
        "LIMIT $select_count OFFSET $select_offset"
    },
    [STATEMENT_SEARCH_ALBUMS_WITH_PAGINATION] = { .src =
        "SELECT id, name, artist, artist_id, song_count, duration "
        "FROM albums "
        "WHERE ( ( server_id = $server_id ) AND ( name LIKE '%' || $query || '%' ) ) "
        "ORDER BY created DESC "
        "LIMIT $select_count OFFSET $select_offset"
    },
    [STATEMENT_GET_SONGS_WITH_PAGINATION] = { .src =
        "SELECT "
            "id, title, artist, album, "
            "track, year, duration, bitrate, size, filetype, "
            "artist_id, album_id  "
        "FROM songs "
        "WHERE server_id = $server_id "
        "ORDER BY title ASC "
        "LIMIT $select_count OFFSET $select_offset"
    },

    [STATEMENT_GET_SONGS_IN_ALBUM] = { .src =
        "SELECT "
            "id, title, artist, album, "
            "track, year, duration, bitrate, size, filetype, "
            "artist_id, album_id  "
        "FROM songs "
        "WHERE ( server_id = $server_id AND album_id = $album_id ) "
        "ORDER BY track ASC"
    },

    [STATEMENT_GET_ALBUMS_FOR_ARTIST] = { .src =
        "SELECT id, name, artist, artist_id, song_count, duration "
        "FROM albums "
        "WHERE ( server_id = $server_id AND artist_id = $artist_id ) "
        "ORDER BY created DESC"
    },
    [STATEMENT_GET_SONGS_FOR_ARTIST] = { .src =
        "SELECT "
            "id, title, artist, album, "
            "track, year, duration, bitrate, size, filetype, "
            "artist_id, album_id  "
        "FROM songs "
        "WHERE ( server_id = $server_id AND artist_id = $artist_id )"
    },

    /* TODO: I don't think server_id is needed here, just make a separate primary key from id */
    [STATEMENT_GET_CACHED_SONG] = { .src =
        "SELECT id, filename, filetype, bitrate, size "
        "FROM cached_songs "
        "WHERE ( server_id = $server_id AND id = $id )"
    },
    [STATEMENT_DELETE_CACHED_SONG] = { .src =
        "DELETE FROM cached_songs WHERE ( server_id = $server_id AND id = $id )"
    },
    [STATEMENT_ADD_CACHED_SONG] = { .src =
        "INSERT OR REPLACE INTO cached_songs ( "
            "id, filename, filetype, bitrate, size, server_id "
        ") VALUES ( "
            "$id, $filename, $filetype, $bitrate, $size, $server_id "
        ")"
    },
    [STATEMENT_TOUCH_CACHED_SONG] = { .src =
        "UPDATE cached_songs "
        "SET accessed = unixepoch('now') "
        "WHERE ( id = $id AND server_id = $server_id )"
    },
};
static_assert(SIZEOF_VEC(statements) == SQLITE_STATEMENT_TYPE_COUNT);

struct sqlite3 *db = NULL;

void statement_resetp(struct sqlite3_stmt *const *pstmt) {
    struct sqlite3_stmt *const stmt = *pstmt;
    if (stmt != NULL) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
    }
}

bool statement_execute(enum sqlite_statement_type index) {
    [[gnu::cleanup(statement_resetp)]]
    struct sqlite3_stmt *const stmt = statements[index].stmt;

    int ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        return false;
    }

    return true;
}

bool db_init(void) {
    int ret = 0;
    [[gnu::cleanup(cleanup_free)]] char *db_path = NULL;

    xasprintf(&db_path, "%s/db.sqlite3", config.data_dir);
    ret = sqlite3_open(db_path, &db);
    if (ret != SQLITE_OK) {
        ERROR("failed to open db at %s: %s", db_path, sqlite3_errstr(ret));
        goto err;
    }

    ret = sqlite3_exec(db, statements[STATEMENT_ENABLE_FOREIGN_KEYS].src, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to enable foreign key support: %s", sqlite3_errmsg(db));
        goto err;
    }

    ret = sqlite3_exec(db, statements[STATEMENT_CREATE_TABLE_SERVERS].src, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to create servers table: %s", sqlite3_errmsg(db));
        goto err;
    }
    ret = sqlite3_exec(db, statements[STATEMENT_CREATE_TABLE_ARTISTS].src, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to create artists table: %s", sqlite3_errmsg(db));
        goto err;
    }
    ret = sqlite3_exec(db, statements[STATEMENT_CREATE_TABLE_ALBUMS].src, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to create albums table: %s", sqlite3_errmsg(db));
        goto err;
    }
    ret = sqlite3_exec(db, statements[STATEMENT_CREATE_TABLE_SONGS].src, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to create songs table: %s", sqlite3_errmsg(db));
        goto err;
    }
    ret = sqlite3_exec(db, statements[STATEMENT_CREATE_TABLE_CACHED_SONGS].src, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to create song cache table: %s", sqlite3_errmsg(db));
        goto err;
    }

    /* prepare statements */
    for (size_t i = 0; i < SIZEOF_VEC(statements); i++) {
        ret = sqlite3_prepare_v2(db, statements[i].src, -1, &statements[i].stmt, NULL);
        if (ret != SQLITE_OK) {
            ERROR("failed to prepare sql stmt: %s", sqlite3_errmsg(db));
            log_println(LOG_ERROR, "%s", statements[i].src);
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

