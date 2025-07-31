#include <strings.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>

#include "log.h"

#define LOG_ANSI_COLORS_ERROR "\033[1;31m"
#define LOG_ANSI_COLORS_WARN  "\033[1;33m"
#define LOG_ANSI_COLORS_INFO  "\033[1;34m"
#define LOG_ANSI_COLORS_TRACE "\033[2m"
#define LOG_ANSI_COLORS_RESET "\033[0m"

struct log_config {
    FILE *stream;
    enum log_loglevel loglevel;
    bool colors;
};

static struct log_config log_config = {
    .stream = NULL,
    .loglevel = LOG_INFO,
    .colors = false,
};

enum log_loglevel log_str_to_loglevel(const char *str) {
    if (strcasecmp(str, "trace") == 0) {
        return LOG_TRACE;
    } else if (strcasecmp(str, "debug") == 0) {
        return LOG_DEBUG;
    } else if (strcasecmp(str, "info") == 0) {
        return LOG_INFO;
    } else if (strcasecmp(str, "warn") == 0) {
        return LOG_WARN;
    } else if (strcasecmp(str, "error") == 0) {
        return LOG_ERROR;
    } else if (strcasecmp(str, "quiet") == 0) {
        return LOG_QUIET;
    } else {
        return LOG_INVALID;
    }
}

void log_init(FILE *stream, enum log_loglevel level, bool force_colors) {
    log_config.stream = stream;
    log_config.loglevel = level;
    log_config.colors = force_colors ? true : isatty(fileno(stream));
}

static void log_print_internal(enum log_loglevel level, bool newline, char *message, va_list args) {
    if (log_config.stream == NULL) {
        return;
    }

    if (level > log_config.loglevel) {
        return;
    }

    char level_char = '?';
    switch (level) {
    case LOG_ERROR:
        if (log_config.colors) {
            fprintf(log_config.stream, LOG_ANSI_COLORS_ERROR);
        }
        level_char = 'E';
        break;
    case LOG_WARN:
        if (log_config.colors) {
            fprintf(log_config.stream, LOG_ANSI_COLORS_WARN);
        }
        level_char = 'W';
        break;
    case LOG_INFO:
        if (log_config.colors) {
            fprintf(log_config.stream, LOG_ANSI_COLORS_INFO);
        }
        level_char = 'I';
        break;
    case LOG_DEBUG:
        /* no special color here... */
        level_char = 'D';
        break;
    case LOG_TRACE:
        if (log_config.colors) {
            fprintf(log_config.stream, LOG_ANSI_COLORS_TRACE);
        }
        level_char = 'T';
        break;
    default:
        fprintf(stderr, "logger error: unknown loglevel %d\n", level);
        abort();
    }

    fprintf(log_config.stream, "[%c] ", level_char);

    vfprintf(log_config.stream, message, args);

    if (log_config.colors) {
        fprintf(log_config.stream, LOG_ANSI_COLORS_RESET);
    }
    if (newline) {
        fprintf(log_config.stream, "\n");
    }

    fflush(log_config.stream);
}

void log_print(enum log_loglevel level, char *message, ...) {
    va_list args;

    va_start(args, message);
    log_print_internal(level, false, message, args);
    va_end(args);
}

void log_println(enum log_loglevel level, char *message, ...) {
    va_list args;

    va_start(args, message);
    log_print_internal(level, true, message, args);
    va_end(args);
}

