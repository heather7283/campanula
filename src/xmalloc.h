#ifndef SRC_XMALLOC_H
#define SRC_XMALLOC_H

#include <stdarg.h>
#include <stddef.h>

/* malloc, but aborts on alloc fail */
[[gnu::malloc,gnu::returns_nonnull,gnu::warn_unused_result]]
void *xmalloc(size_t size);
/* calloc, but aborts on alloc fail */
[[gnu::malloc,gnu::returns_nonnull,gnu::warn_unused_result]]
void *xcalloc(size_t n, size_t size);
/* realloc, but aborts on alloc fail */
[[gnu::returns_nonnull,gnu::warn_unused_result]]
void *xrealloc(void *ptr, size_t size);
/* reallocarray, but aborts on alloc fail */
[[gnu::returns_nonnull,gnu::warn_unused_result]]
void *xreallocarray(void *ptr, size_t nmemb, size_t size);

/* strdup, but aborts on alloc fail and returns NULL when called on NULL */
[[gnu::malloc,gnu::warn_unused_result]]
char *xstrdup(const char *s);

/* vprintf to mallocd string and abort on allocation fail */
[[gnu::format(printf, 2, 0)]]
int xvasprintf(char **restrict strp, const char *restrict fmt, va_list ap);
/* printf to mallocd string and abort on allocation fail */
[[gnu::format(printf, 2, 3)]]
int xasprintf(char **restrict strp, const char *restrict fmt, ...);

#endif /* #ifndef SRC_XMALLOC_H */

