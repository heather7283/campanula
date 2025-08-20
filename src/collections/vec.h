#ifndef SRC_COLLECTIONS_VEC_H
#define SRC_COLLECTIONS_VEC_H

#include <stddef.h>

#include "macros.h"

struct array_generic {
    size_t size, capacity;
    void *data;
};

#define VEC(type) \
    struct { \
        size_t size, capacity; \
        type *data; \
    }

#define VEC_INITALISER {0}

#define VEC_INIT(parray) \
    do { \
        (parray)->size = (parray)->capacity = 0; \
        (parray)->data = NULL; \
    } while (0)

#define TYPECHECK_VEC(parray) \
    ({ \
        (void)(parray)->size; (void)(parray)->capacity; (void)(parray)->data; 1; \
    })

#define TYPECHECK(a, b) \
    ({ \
        TYPEOF(a) dummy_a; \
        TYPEOF(b) dummy_b; \
        (void)(&dummy_a == &dummy_b); \
        1; \
    })

#define VEC_SIZE(parray) ((parray)->size)
#define VEC_DATA(parray) ((parray)->data)

/*
 * Insert elem_count elements, each of size elem_size, in arr at index.
 * If elems is not NULL, elements are initialised from elems.
 * If elems is NULL and zero_init is true, memory is zero-initialised.
 * If elems is NULL and zero_init is false, memory is NOT initialised.
 * Returns address of the first inserted element.
 * Supports python-style negative indexing.
 * Dumps core on OOB access.
 */
void *array_insert_generic(struct array_generic *arr, ptrdiff_t index,
                           const void *elems, size_t elem_size, size_t elem_count,
                           bool zero_init);

#define VEC_INSERT_N(parray, index, pelem, nelem) \
    ({ \
        TYPECHECK_VEC(parray); \
        TYPECHECK(*(parray)->data, *(pelem)); \
        array_insert_generic((struct array_generic *)(parray), (index), \
                             (pelem), sizeof(*(parray)->data), (nelem), false); \
    })

#define VEC_INSERT(parray, index, pelem) \
    VEC_INSERT_N(parray, index, pelem, 1)

#define VEC_EMPLACE_INTERNAL_DO_NOT_USE(parray, index, nelem, zeroed) \
    ({ \
        TYPECHECK_VEC(parray); \
        (TYPEOF((parray)->data))array_insert_generic((struct array_generic *)(parray), (index), \
                                                     NULL, sizeof(*(parray)->data), (nelem), \
                                                     (zeroed)); \
    })

#define VEC_EMPLACE_N(parray, index, nelem) \
    VEC_EMPLACE_INTERNAL_DO_NOT_USE(parray, index, nelem, false)

#define VEC_EMPLACE(parray, index) \
    VEC_EMPLACE_N(parray, index, 1)

#define VEC_EMPLACE_N_ZEROED(parray, index, nelem) \
    VEC_EMPLACE_INTERNAL_DO_NOT_USE(parray, index, nelem, true)

#define VEC_EMPLACE_ZEROED(parray, index) \
    VEC_EMPLACE_N_ZEROED(parray, index, 1)

/*
 * Appends elem_count elements, each of size elem_size, to the end of arr.
 * If elems is not NULL, elements are initialised from elems.
 * If elems is NULL and zero_init is true, memory is zero-initialised.
 * If elems is NULL and zero_init is false, memory is NOT initialised.
 * Returns address of the first appended element.
 */
void *array_append_generic(struct array_generic *arr, const void *elems,
                           size_t elem_size, size_t elem_count, bool zero_init);

#define VEC_APPEND_N(parray, pelem, nelem) \
    ({ \
        TYPECHECK_VEC(parray); \
        TYPECHECK(*(parray)->data, *(pelem)); \
        array_append_generic((struct array_generic *)(parray), (pelem), \
                             sizeof(*(parray)->data), (nelem), false); \
    })

#define VEC_APPEND(parray, pelem) \
    VEC_APPEND_N(parray, pelem, 1)

#define VEC_EMPLACE_BACK_INTERNAL_DO_NOT_USE(parray, nelem, zeroed) \
    ({ \
        TYPECHECK_VEC(parray); \
        (TYPEOF((parray)->data))array_append_generic((struct array_generic *)(parray), NULL, \
                                                     sizeof(*(parray)->data), (nelem), (zeroed)); \
    })

#define VEC_EMPLACE_BACK_N(parray, nelem) \
    VEC_EMPLACE_BACK_INTERNAL_DO_NOT_USE(parray, nelem, false)

#define VEC_EMPLACE_BACK(parray) \
    VEC_EMPLACE_BACK_N(parray, 1)

#define VEC_EMPLACE_BACK_N_ZEROED(parray, nelem) \
    VEC_EMPLACE_BACK_INTERNAL_DO_NOT_USE(parray, nelem, true)

#define VEC_EMPLACE_BACK_ZEROED(parray) \
    VEC_EMPLACE_BACK_N_ZEROED(parray, 1)

/*
 * Removes elem_count elements, each of size elem_size, at index from arr.
 * Support python-style negative indexing.
 * Dumps core on OOB access.
 */
void array_erase_generic(struct array_generic *arr, ptrdiff_t index,
                         size_t elem_size, size_t elem_count);

#define VEC_ERASE_N(parray, index, count) \
    ({ \
        TYPECHECK_VEC(parray); \
        array_erase_generic((struct array_generic *)(parray), (index), \
                            sizeof(*(parray)->data), (count)); \
    })

#define VEC_ERASE(parray, index) \
    VEC_ERASE_N(parray, index, 1)

/*
 * Returns pointer to element of arr at index.
 * Supports python-style negative indexing.
 * Dumps core on OOB access.
 */
void *array_at_generic(struct array_generic *arr, ptrdiff_t index, size_t elem_size);

#define VEC_AT(parray, index) \
    ({ \
        TYPECHECK_VEC(parray); \
        (TYPEOF((parray)->data))array_at_generic((struct array_generic *)(parray), (index), \
                                                 sizeof(*(parray)->data)); \
    })

/*
 * Sets size to 0 but does not free memory.
 */
void array_clear_generic(struct array_generic *arr);

#define VEC_CLEAR(parray) \
    ({ \
        TYPECHECK_VEC(parray); \
        array_clear_generic((struct array_generic *)(parray)); \
    })

/*
 * Frees all memory and makes array ready for reuse.
 */
void array_free_generic(struct array_generic *arr);

#define VEC_FREE(parray) \
    ({ \
        TYPECHECK_VEC(parray); \
        array_free_generic((struct array_generic *)(parray)); \
    })

/*
 * Reserves memory for elem_count elements, each of size elem_size.
 */
void array_reserve_generic(struct array_generic *arr, size_t elem_size, size_t elem_count);

#define VEC_RESERVE(parray, count) \
    ({ \
        TYPECHECK_VEC(parray); \
        array_reserve_generic((struct array_generic *)(parray), \
                              sizeof(*(parray)->data), (count)); \
    })

#define VEC_FOREACH(parray, iter) for (size_t iter = 0; iter < (parray)->size; iter++)

#endif /* #ifndef SRC_COLLECTIONS_VEC_H */

