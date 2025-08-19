#ifndef SRC_TUI_PAD_H
#define SRC_TUI_PAD_H

#include <curses.h>

typedef WINDOW PAD;

enum tui_set_pad_size_policy { EXACTLY, AT_LEAST };
PAD *tui_pad_ensure_size(PAD *pad,
                         enum tui_set_pad_size_policy y_policy, int y,
                         enum tui_set_pad_size_policy x_policy, int x,
                         bool keep_contents);

#endif /* #ifndef SRC_TUI_PAD_H */

