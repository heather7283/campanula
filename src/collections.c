#include <stdlib.h>
#include <string.h>

#include "collections.h"
#include "xmalloc.h"

#define GROWTH_FACTOR 1.5

void array_extend_generic(struct array_generic *arr, void *elems,
                          size_t elem_size, size_t elem_count) {
    if ((arr->size + elem_count) > arr->capacity) {
        size_t new_capacity = MAX((arr->size + elem_count), arr->capacity * GROWTH_FACTOR);
        arr->data = xreallocarray(arr->data, new_capacity, elem_size);
        arr->capacity = new_capacity;
    }

    memcpy((char *)arr->data + (arr->size * elem_size), elems, elem_size * elem_count);
    arr->size += elem_count;
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

