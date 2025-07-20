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

#endif /* #ifndef SRC_COLLECTIONS_H */

