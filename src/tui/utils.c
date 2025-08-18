#include <stdio.h>
#include <wctype.h>

#include "tui/utils.h"
#include "macros.h"
#include "log.h"

const char *key_name_from_key_code(int code) {
    static char codepoint[5];
    if (iswgraph(code)) {
        snprintf(codepoint, sizeof(codepoint), "%lc", code);
        return codepoint;
    }

    switch (code) {
    case 0: return "NUL";
    case 1: return "SOH";
    case 2: return "STX";
    case 3: return "ETX";
    case 4: return "EOT";
    case 5: return "ENQ";
    case 6: return "ACK";
    case 7: return "BEL";
    case 8: return "BS";
    case 9: return "HT";
    case 10: return "LF";
    case 11: return "VT";
    case 12: return "FF";
    case 13: return "CR";
    case 14: return "SO";
    case 15: return "SI";
    case 16: return "DLE";
    case 17: return "DC1";
    case 18: return "DC2";
    case 19: return "DC3";
    case 20: return "DC4";
    case 21: return "NAK";
    case 22: return "SYN";
    case 23: return "ETB";
    case 24: return "CAN";
    case 25: return "EM";
    case 26: return "SUB";
    case 27: return "ESC";
    case 28: return "FS";
    case 29: return "GS";
    case 30: return "RS";
    case 31: return "US";
    case 32: return "SPACE";
    /* Fallback */
    default: return "?????";
    }
}

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

PAD *tui_set_pad_size(PAD *pad,
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

