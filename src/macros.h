#ifndef SRC_MACROS_H
#define SRC_MACROS_H

#define STREQ(a, b) (strcmp((a), (b)) == 0)
#define STRSTARTSWITH(a, b) (strncmp((a), (b), strlen(b)) == 0)

#define SIZEOF_ARRAY(arr) (sizeof(arr) / sizeof((arr)[0]))

#define TYPEOF(x) __typeof__(x)

#define MAX(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })
#define MIN(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })

#endif /* #ifndef SRC_MACROS_H */

