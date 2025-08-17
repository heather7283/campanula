#ifndef SRC_TUI_UTILS_H
#define SRC_TUI_UTILS_H

#include "tui/internal.h"

const char *key_name_from_key_code(int keycode);

/* 0 to leave dimension as is */
enum tui_set_pad_size_policy { EXACTLY, AT_LEAST };
PAD *tui_set_pad_size(PAD *pad,
                      enum tui_set_pad_size_policy y_policy, int y,
                      enum tui_set_pad_size_policy x_policy, int x,
                      bool keep_contents);


#endif /* #ifndef SRC_TUI_UTILS_H */

