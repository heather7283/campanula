#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "collections.h"
#include "urlencode.h"
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


void string_clear(struct string *str) {
    str->len = 0;
    if (str->str != NULL) {
        str->str[0] = '\0';
    }
}

void string_free(struct string *str) {
    str->len = str->capacity = 0;
    free(str->str);
    str->str = NULL;
}

static void string_ensure_capacity(struct string *str, size_t cap) {
    if (str->capacity < cap) {
        size_t new_cap = MAX(cap, str->capacity * GROWTH_FACTOR);
        str->str = xrealloc(str->str, new_cap);
        str->capacity = cap;
    }
}

size_t string_append(struct string *str, const char *suffix) {
    size_t len = strlen(suffix);

    string_ensure_capacity(str, str->len + len + 1);

    memcpy(str->str + str->len, suffix, len);
    str->len += len;
    str->str[str->len] = '\0';

    return len;
}

size_t string_append_urlencode(struct string *str, const char *suffix) {
    size_t suffix_len = strlen(suffix);

    string_ensure_capacity(str, str->len + (suffix_len * 3) + 1);

    size_t url_len = urlencode(str->str + str->len, suffix, suffix_len);
    str->len += url_len;

    return url_len;
}

int string_appendf(struct string *str, const char *fmt, ...) {
    va_list args, args_copy;
    va_start(args, fmt);

    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    string_ensure_capacity(str, str->len + len + 1);

    len = vsnprintf(str->str + str->len, str->capacity - str->len, fmt, args);
    str->len += len;

    va_end(args);

    return len;
}

