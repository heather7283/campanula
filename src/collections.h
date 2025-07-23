#ifndef SRC_COLLECTIONS_H
#define SRC_COLLECTIONS_H

#include <stddef.h>

#define MAX(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })
#define MIN(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })

#define TYPEOF(x) __typeof__(x)

#define TYPECHECK(type, x) \
    ({ \
        type dummy; \
        TYPEOF(x) dummy2; \
        (void)(&dummy == &dummy2); \
        1; \
    })

/* ############################## ARRAY ############################## */

struct array_generic {
    size_t size, capacity;
    void *data;
};

#define ARRAY(type) \
    struct { \
        size_t size, capacity; \
        type *data; \
    }

#define ARRAY_INITALISER {0}

#define TYPECHECK_ARRAY(parray) \
    ({ \
        (void)(parray)->size; (void)(parray)->capacity; (void)(parray)->data; 1; \
    })

#define ARRAY_AT(parray, index) ((parray)->data[index])

#define ARRAY_SIZE(parray) ((parray)->size)
#define ARRAY_DATA(parray) ((parray)->data)

void array_extend_generic(struct array_generic *arr, void *elem,
                          size_t elem_size, size_t elem_count);

#define ARRAY_EXTEND(parray, pelem, nelem) \
    ({ \
        TYPECHECK_ARRAY(parray); \
        TYPECHECK(TYPEOF(*(parray)->data), *(pelem)); \
        array_extend_generic((struct array_generic *)(parray), (pelem), sizeof(*(pelem)), (nelem)); \
    })

#define ARRAY_APPEND(parray, pelem) ARRAY_EXTEND(parray, pelem, 1)

void *array_emplace_generic(struct array_generic *arr, size_t elem_size);

#define ARRAY_EMPLACE(parray) \
    ({ \
        TYPECHECK_ARRAY(parray); \
        (TYPEOF((parray)->data))array_emplace_generic((struct array_generic *)(parray), \
                                                      sizeof(*(parray)->data)); \
    })

void array_clear_generic(struct array_generic *arr);

#define ARRAY_CLEAR(parray) \
    ({ \
        TYPECHECK_ARRAY(parray); \
        array_clear_generic((struct array_generic *)(parray)); \
    })

void array_free_generic(struct array_generic *arr);

#define ARRAY_FREE(parray) \
    ({ \
        TYPECHECK_ARRAY(parray); \
        array_free_generic((struct array_generic *)(parray)); \
    })

#define ARRAY_FOREACH(parray, iter) for (size_t iter = 0; iter < (parray)->size; iter++)

/* ############################## STRING ############################## */

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

#endif /* #ifndef SRC_COLLECTIONS_H */

