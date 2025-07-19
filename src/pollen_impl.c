#include <stdio.h>
#define POLLEN_LOG_DEBUG(fmt, ...) \
    fprintf(stderr, "\033[2mevent loop: " fmt "\033[m\n" __VA_OPT__(,) __VA_ARGS__)
#define POLLEN_LOG_INFO(fmt, ...) \
    fprintf(stderr, "\033[32mevent loop: " fmt "\033[m\n" __VA_OPT__(,) __VA_ARGS__)
#define POLLEN_LOG_WARN(fmt, ...) \
    fprintf(stderr, "\033[33mevent loop: " fmt "\033[m\n" __VA_OPT__(,) __VA_ARGS__)
#define POLLEN_LOG_ERR(fmt, ...) \
    fprintf(stderr, "\033[31mevent loop: " fmt "\033[m\n" __VA_OPT__(,) __VA_ARGS__)

#include <stdlib.h>
#include "xmalloc.h"
#define POLLEN_CALLOC(n, size) xcalloc(n, size)
#define POLLEN_FREE(ptr) free(ptr)

#define POLLEN_IMPLEMENTATION
#include <pollen.h>

