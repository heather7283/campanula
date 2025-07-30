#include "log.h"
//#define POLLEN_LOG_DEBUG(fmt, ...) TRACE("pollen: " fmt __VA_OPT__(,) __VA_ARGS__)
//#define POLLEN_LOG_INFO(fmt, ...) TRACE("pollen: " fmt __VA_OPT__(,) __VA_ARGS__)
#define POLLEN_LOG_WARN(fmt, ...) WARN("pollen: " fmt __VA_OPT__(,) __VA_ARGS__)
#define POLLEN_LOG_ERR(fmt, ...) ERROR("pollen: " fmt __VA_OPT__(,) __VA_ARGS__)

#include <stdlib.h>
#include "xmalloc.h"
#define POLLEN_CALLOC(n, size) xcalloc(n, size)
#define POLLEN_FREE(ptr) free(ptr)

#define POLLEN_EPOLL_MAX_EVENTS 256

#define POLLEN_IMPLEMENTATION
#include <pollen.h>

struct pollen_loop *event_loop = NULL;

