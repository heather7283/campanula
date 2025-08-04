#include <stdio.h>
#include <string.h>

#include "collections/array.h"
#include "macros.h"
#include "xmalloc.h"

#define GROWTH_FACTOR 1.5

static void array_ensure_capacity(struct array_generic *arr, size_t elem_size, size_t cap) {
    if (cap > arr->capacity) {
        const size_t new_cap = MAX(cap, arr->capacity * GROWTH_FACTOR);
        arr->data = xreallocarray(arr->data, new_cap, elem_size);
        arr->capacity = new_cap;
    }
}

static size_t array_bound_check(const struct array_generic *arr, size_t index) {
    if (index >= arr->size) {
        fprintf(stderr, "Index %zu is out of bounds of array of size %zu", index, arr->size);
        fflush(stderr);
        abort();
    }
    return index;
}

static size_t array_normalise_index(const struct array_generic *arr, ptrdiff_t index) {
    if (index < 0) {
        return array_bound_check(arr, arr->size - -index);
    } else {
        return array_bound_check(arr, index);
    }
}

void *array_insert_generic(struct array_generic *arr, ptrdiff_t _index,
                           const void *elems, size_t elem_size, size_t elem_count,
                           bool zero_init) {
    size_t index = array_normalise_index(arr, _index);
    array_ensure_capacity(arr, elem_size, arr->size + elem_count);

    /* shift existing elements to make space for new ones */
    memmove((char *)arr->data + ((index + elem_count) * elem_size),
            (char *)arr->data + (index * elem_size),
            (arr->size - index) * elem_size);

    /* copy new elements to array */
    if (elems != NULL) {
        memcpy((char *)arr->data + (index * elem_size), elems, elem_size * elem_count);
    } else if (zero_init) {
        memset((char *)arr->data + (index * elem_size), '\0', elem_size * elem_count);
    }
    arr->size += elem_count;

    return (char *)arr->data + (index * elem_size);
}

void *array_append_generic(struct array_generic *arr, const void *elems,
                           size_t elem_size, size_t elem_count, bool zero_init) {
    array_ensure_capacity(arr, elem_size, arr->size + elem_count);

    /* append new elements to the end */
    if (elems != NULL) {
        memcpy((char *)arr->data + (arr->size * elem_size), elems, elem_size * elem_count);
    } else if (zero_init) {
        memset((char *)arr->data + (arr->size * elem_size), '\0', elem_size * elem_count);
    }
    arr->size += elem_count;

    return (char *)arr->data + ((arr->size - elem_count) * elem_size);
}

void array_erase_generic(struct array_generic *arr, ptrdiff_t _index,
                         size_t elem_size, size_t elem_count) {
    const size_t index = array_normalise_index(arr, _index);
    array_bound_check(arr, index + elem_count - 1);

    memmove((char *)arr->data + (index * elem_size),
            (char *)arr->data + ((index + elem_count) * elem_size),
            (arr->size - index - elem_count) * elem_size);
    arr->size -= elem_count;
}

void *array_at_generic(struct array_generic *arr, ptrdiff_t _index, size_t elem_size) {
    size_t index = array_normalise_index(arr, _index);
    return (char *)arr->data + (index * elem_size);
}

void array_clear_generic(struct array_generic *arr) {
    arr->size = 0;
}

void array_free_generic(struct array_generic *arr) {
    arr->size = 0;
    arr->capacity = 0;
    free(arr->data);
    arr->data = NULL;
}

