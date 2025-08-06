#include <ctype.h>

#include "commands/parse.h"
#include "collections/array.h"

size_t command_into_argv(char *cmd, char ***argv) {
    ARRAY(char *) args;

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
                ARRAY_APPEND(&args, &arg_start);
            }
        }
        p += 1;
    }
    if (!prev_space) {
        ARRAY_APPEND(&args, &arg_start);
    }

    arg_start = NULL;
    ARRAY_APPEND(&args, &arg_start);

    *argv = ARRAY_DATA(&args);
    return ARRAY_SIZE(&args);
}

