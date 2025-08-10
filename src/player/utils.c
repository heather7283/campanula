#include <inttypes.h>

#include "player/utils.h"

void print_mpv_node_internal(const char *key, const struct mpv_node *n,
                             enum log_level lvl, int indent) {
    const char *k = key ? key : "";
    const char *s = key ? ": " : "";

    switch (n->format) {
    case MPV_FORMAT_NONE:
        log_println(lvl, "%*s%s%sNode { NONE }", indent, "", k, s);
        break;
    case MPV_FORMAT_STRING:
        log_println(lvl, "%*s%s%sNode { STRING: %s }", indent, "", k, s, n->u.string);
        break;
    case MPV_FORMAT_OSD_STRING:
        log_println(lvl, "%*s%s%sNode { OSD_STRING: %s }", indent, "", k, s, n->u.string);
        break;
    case MPV_FORMAT_FLAG:
        log_println(lvl, "%*s%s%sNode { FLAG: %s }", indent, "", k, s, n->u.flag ? "true" : "false");
        break;
    case MPV_FORMAT_INT64:
        log_println(lvl, "%*s%s%sNode { INT64: %"PRIi64" }", indent, "", k, s, n->u.int64);
        break;
    case MPV_FORMAT_DOUBLE:
        log_println(lvl, "%*s%s%sNode { DOUBLE: %f }", indent, "", k, s, n->u.double_);
        break;
    case MPV_FORMAT_NODE_ARRAY:
        log_println(lvl, "%*s%s%sNode { NODE_ARRAY(%d): [", indent, "", k, s, n->u.list->num);
        for (int i = 0; i < n->u.list->num; i++) {
            print_mpv_node_internal(NULL, &n->u.list->values[i], lvl, indent + 4);
        }
        log_println(lvl, "%*s]}", indent, "");
        break;
    case MPV_FORMAT_NODE_MAP:
        log_println(lvl, "%*s%s%sNode { NODE_MAP(%d): {", indent, "", k, s, n->u.list->num);
        for (int i = 0; i < n->u.list->num; i++) {
            print_mpv_node_internal(n->u.list->keys[i], &n->u.list->values[i], lvl, indent + 4);
        }
        log_println(lvl, "%*s}}", indent, "");
        break;
    default:
    }
}

void print_mpv_node(const struct mpv_node *n, enum log_level lvl, int indent) {
    print_mpv_node_internal(NULL, n, lvl, indent);
}

