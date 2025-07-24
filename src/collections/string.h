#ifndef SRC_COLLECTIONS_STRING_H
#define SRC_COLLECTIONS_STRING_H

#include <stddef.h>

struct string {
    size_t len; /* without null terminator */
    size_t capacity; /* with null terminator */
    char *str; /* always null terminated (unless NULL) */
};

void string_clear(struct string *str);
void string_free(struct string *str);

size_t string_append(struct string *str, const char *suffix);
size_t string_append_urlencode(struct string *str, const char *suffix);
[[gnu::format(printf, 2, 3)]]
int string_appendf(struct string *str, const char *fmt, ...);

#endif /* #ifndef SRC_COLLECTIONS_STRING_H */

