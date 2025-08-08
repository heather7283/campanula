#include "db/query.h"
#include "db/internal.h"

size_t db_get_albums(struct album **albums, size_t page, size_t albums_per_page);

size_t db_search_albums(struct album **albums, const char *query,
                        size_t page, size_t albums_per_page);

