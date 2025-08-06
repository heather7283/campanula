#ifndef SRC_COMMANDS_PARSE_H
#define SRC_COMMANDS_PARSE_H

#include <stddef.h>

/*
 * for now simply splits on whitespace with no quoting/escaping. TODO: write an actual parser
 * modifies string in place (repalces spaces will null terminators basically)
 * returned argv array must be freed, but args themselves point somewhere inside str's memory
 */
size_t command_into_argv(char *cmd, char ***argv);

#endif /* #ifndef SRC_COMMANDS_PARSE_H */

