#include <stdio.h>
#include <string.h>

#include "collections/vec.h"
#include "macros.h"
#include "xmalloc.h"

#define GROWTH_FACTOR 1.5

static void vec_ensure_capacity(struct vec_generic *arr, size_t elem_size, size_t cap) {
    if (cap > arr->capacity) {
        const size_t new_cap = MAX(cap, arr->capacity * GROWTH_FACTOR);
        arr->data = xreallocarray(arr->data, new_cap, elem_size);
        arr->capacity = new_cap;
    }
}

static size_t vec_bound_check(const struct vec_generic *arr, size_t index) {
    if (index >= arr->size) {
        fprintf(stderr, "Index %zu is out of bounds of vec of size %zu", index, arr->size);
        fflush(stderr);
        abort();
    }
    return index;
}

static size_t vec_normalise_index(const struct vec_generic *arr, ptrdiff_t index) {
    if (index < 0) {
        return vec_bound_check(arr, arr->size - -index);
    } else {
        return vec_bound_check(arr, index);
    }
}

void *vec_insert_generic(struct vec_generic *arr, ptrdiff_t _index,
                         const void *elems, size_t elem_size, size_t elem_count,
                         bool zero_init) {
    size_t index = vec_normalise_index(arr, _index);
    vec_ensure_capacity(arr, elem_size, arr->size + elem_count);

    /* shift existing elements to make space for new ones */
    memmove((char *)arr->data + ((index + elem_count) * elem_size),
            (char *)arr->data + (index * elem_size),
            (arr->size - index) * elem_size);

    /* copy new elements to vec */
    if (elems != NULL) {
        memcpy((char *)arr->data + (index * elem_size), elems, elem_size * elem_count);
    } else if (zero_init) {
        memset((char *)arr->data + (index * elem_size), '\0', elem_size * elem_count);
    }
    arr->size += elem_count;

    return (char *)arr->data + (index * elem_size);
}

void *vec_append_generic(struct vec_generic *arr, const void *elems,
                         size_t elem_size, size_t elem_count, bool zero_init) {
    vec_ensure_capacity(arr, elem_size, arr->size + elem_count);

    /* append new elements to the end */
    if (elems != NULL) {
        memcpy((char *)arr->data + (arr->size * elem_size), elems, elem_size * elem_count);
    } else if (zero_init) {
        memset((char *)arr->data + (arr->size * elem_size), '\0', elem_size * elem_count);
    }
    arr->size += elem_count;

    return (char *)arr->data + ((arr->size - elem_count) * elem_size);
}

void vec_erase_generic(struct vec_generic *arr, ptrdiff_t _index,
                       size_t elem_size, size_t elem_count) {
    const size_t index = vec_normalise_index(arr, _index);
    vec_bound_check(arr, index + elem_count - 1);

    memmove((char *)arr->data + (index * elem_size),
            (char *)arr->data + ((index + elem_count) * elem_size),
            (arr->size - index - elem_count) * elem_size);
    arr->size -= elem_count;
}

void *vec_at_generic(struct vec_generic *arr, ptrdiff_t _index, size_t elem_size) {
    size_t index = vec_normalise_index(arr, _index);
    return (char *)arr->data + (index * elem_size);
}

void vec_clear_generic(struct vec_generic *arr) {
    arr->size = 0;
}

void vec_free_generic(struct vec_generic *arr) {
    arr->size = 0;
    arr->capacity = 0;
    free(arr->data);
    arr->data = NULL;
}

void vec_reserve_generic(struct vec_generic *arr, size_t elem_size, size_t elem_count) {
    vec_ensure_capacity(arr, elem_size, elem_count);
}

