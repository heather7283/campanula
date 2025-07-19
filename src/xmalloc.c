#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmalloc.h"

static void *check_alloc(void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "memory allocation failed, buy more ram lol\n");
        fflush(stderr);
        abort();
    }

    return ptr;
}

void *xmalloc(size_t size) {
    return check_alloc(malloc(size));
}

void *xcalloc(size_t n, size_t size) {
    return check_alloc(calloc(n, size));
}

void *xrealloc(void *ptr, size_t size) {
    return check_alloc(realloc(ptr, size));
}

void *xreallocarray(void *ptr, size_t nmemb, size_t size) {
    return check_alloc(reallocarray(ptr, nmemb, size));
}

char *xstrdup(const char *s) {
    return (s == NULL) ? NULL : check_alloc(strdup(s));
}

int xvasprintf(char **restrict strp, const char *restrict fmt, va_list ap) {
	va_list ap_copy;

	va_copy(ap_copy, ap);
	int length = vsnprintf(NULL, 0, fmt, ap_copy);
    va_end(ap_copy);

    if (length < 0) {
        return -1;
    }
    *strp = check_alloc(malloc(length + 1));

	return vsnprintf(*strp, length + 1, fmt, ap);
}

int xasprintf(char **restrict strp, const char *restrict fmt, ...) {
	va_list args;

	va_start(args, fmt);
	int ret = xvasprintf(strp, fmt, args);
	va_end(args);

	return ret;
}

