#include "tui/pad.h"
#include "macros.h"
#include "log.h"

PAD *tui_resize_pad(PAD *pad, int y, int x, bool keep_contents) {
    TRACE("tui_resize_pad: y %d x %d", y, x);

    PAD *new_pad = newpad(y, x);

    nodelay(new_pad, TRUE);
    keypad(new_pad, TRUE);

    if (pad != NULL) {
        if (keep_contents) {
            copywin(pad, new_pad, 0, 0, 0, 0, y, x, FALSE);
        }
        delwin(pad);
    }

    return new_pad;
}

PAD *tui_pad_ensure_size(PAD *pad,
                         enum tui_set_pad_size_policy y_policy, int y,
                         enum tui_set_pad_size_policy x_policy, int x,
                         bool keep_contents) {
    TRACE("tui_set_pad_size: y %s %d x %s %d",
          y_policy == EXACTLY ? "exactly" : "at least", y,
          x_policy == EXACTLY ? "exactly" : "at least", x);

    PAD *newpad = NULL;

    if (pad == NULL) {
        newpad = tui_resize_pad(pad, y, x, keep_contents);
    } else {
        bool need_resize = false;
        int new_y, new_x;

        const int max_y = getmaxy(pad);
        const int max_x = getmaxx(pad);

        switch (y_policy) {
        case EXACTLY:
            if (y != max_y) {
                need_resize = true;
            }
            new_y = y;
            break;
        case AT_LEAST:
            if (y > max_y) {
                need_resize = true;
            }
            new_y = MAX(max_y, y);
            break;
        }

        switch (x_policy) {
        case EXACTLY:
            if (x != max_x) {
                need_resize = true;
            }
            new_x = x;
            break;
        case AT_LEAST:
            if (x > max_x) {
                need_resize = true;
            }
            new_x = MAX(max_x, x);
            break;
        }

        if (need_resize) {
            newpad = tui_resize_pad(pad, new_y, new_x, keep_contents);
        } else {
            newpad = pad;
        }
    }

    return newpad;
}

