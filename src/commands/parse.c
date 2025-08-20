#include <ctype.h>

#include "commands/parse.h"
#include "collections/vec.h"

size_t command_into_argv(char *cmd, char ***argv) {
    VEC(char *) args;

    char *p = cmd;
    char *arg_start;
    bool prev_space = true;
    while (*p != '\0') {
        if (!isspace(*p)) {
            if (prev_space) {
                prev_space = false;
                arg_start = p;
            }
        } else {
            if (!prev_space) {
                prev_space = true;
                *p = '\0';
                VEC_APPEND(&args, &arg_start);
            }
        }
        p += 1;
    }
    if (!prev_space) {
        VEC_APPEND(&args, &arg_start);
    }

    arg_start = NULL;
    VEC_APPEND(&args, &arg_start);

    *argv = VEC_DATA(&args);
    return VEC_SIZE(&args);
}

