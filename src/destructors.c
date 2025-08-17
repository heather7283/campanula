#include <stdlib.h>

#include "destructors.h"

void cleanup_ptr(void **pptr) {
    free(*pptr);
}

void cleanup_cstr(char **pcstr) {
    free(*pcstr);
}

