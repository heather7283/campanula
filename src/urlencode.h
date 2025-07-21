#ifndef SRC_URLENCODE_H
#define SRC_URLENCODE_H

#include <stddef.h>

/*
 * Null-terminates dst, returns length without null terimnator.
 * Make sure that dst is at least (src_len * 3) + 1 bytes in size.
 */
size_t urlencode(char dst[], const char src[], size_t src_len);

#endif /* #ifndef SRC_URLENCODE_H */

