#ifndef SRC_DB_QUERY_H
#define SRC_DB_QUERY_H

#include <stddef.h>

#include "types/album.h"

size_t db_get_albums(struct album **albums, size_t page, size_t albums_per_page);

size_t db_search_albums(struct album **albums, const char *query,
                        size_t page, size_t albums_per_page);

#endif /* #ifndef SRC_DB_QUERY_H */

