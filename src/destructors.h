#ifndef SRC_DESTRUCTORS_H
#define SRC_DESTRUCTORS_H

#define CLEANUP(func) [[gnu::cleanup(func)]]

void cleanup_cstr(char **pcstr);
void cleanup_ptr(void **pptr);

#endif /* #ifndef SRC_DESTRUCTORS_H */

