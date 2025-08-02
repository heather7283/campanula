#ifndef SRC_LOG_H
#define SRC_LOG_H

#include <stdio.h>
#include <stdbool.h>

enum log_level {
    LOG_INVALID,
    LOG_QUIET,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
    LOG_TRACE,
};

enum log_level log_str_to_loglevel(const char *str);

void log_init(FILE *stream, enum log_level level, bool force_colors);

[[gnu::format(printf, 2, 3)]]
void log_print(enum log_level level, char *msg, ...);
[[gnu::format(printf, 2, 3)]]
void log_println(enum log_level level, char *msg, ...);

#define TRACE(fmt, ...) \
    do { \
        log_println(LOG_TRACE, \
                    "%s:%-3d " fmt, \
                    __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__); \
    } while (0)

#define DEBUG(fmt, ...) \
    do { \
        log_println(LOG_DEBUG, \
                    "%s:%-3d " fmt, \
                    __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__); \
    } while (0)

#define INFO(fmt, ...) \
    do { \
        log_println(LOG_INFO, \
                    "%s:%-3d " fmt, \
                    __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__); \
    } while (0)


#define WARN(fmt, ...) \
    do { \
        log_println(LOG_WARN, \
                    "%s:%-3d " fmt, \
                    __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__); \
    } while (0)

#define ERROR(fmt, ...) \
    do { \
        log_println(LOG_ERROR, \
                    "%s:%-3d " fmt, \
                    __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__); \
    } while (0)

#undef PRINTF

#endif /* #ifndef SRC_LOG_H */

