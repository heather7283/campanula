#ifndef SRC_COLLECTIONS_LIST_H
#define SRC_COLLECTIONS_LIST_H

#include "macros.h"

struct list {
    struct list *next;
    struct list *prev;
};

#define LIST_HEAD struct list
#define LIST_ENTRY struct list

#define LIST_INITIALISER(phead) { .next = phead, .prev = phead }
#define LIST_INIT(phead) ((phead)->next = (phead)->prev = (phead))

#define LIST_IS_EMPTY(phead) ((phead)->next == (phead) && (phead)->prev == (phead))
#define LIST_IS_FIRST(phead, pelem) ((phead)->next == (pelem))
#define LIST_IS_LAST(phead, pelem) ((phead)->prev == (pelem))

#define LIST_GET(pvar, pelem, member) ((pvar) = CONTAINER_OF(pelem, pvar, member))
#define LIST_GET_FIRST(pvar, phead, member) ((pvar) = CONTAINER_OF((phead)->next, pvar, member))
#define LIST_GET_LAST(pvar, phead, member) ((pvar) = CONTAINER_OF((phead)->prev, pvar, member))

/* Inserts new after elem. */
#define LIST_INSERT_AFTER(pelem, pnew) \
    do { \
        (pnew)->next = (pelem)->next; \
        (pnew)->prev = (pelem); \
        (pelem)->next->prev = (pnew); \
        (pelem)->next = (pnew); \
    } while (0)

/* Inserts new before elem. */
#define LIST_INSERT_BEFORE(pelem, pnew) \
    do { \
        (pnew)->prev = (pelem)->prev; \
        (pnew)->next = (pelem); \
        (pelem)->prev->next = (pnew); \
        (pelem)->prev = (pnew); \
    } while (0)

#define LIST_APPEND(phead, pnew) \
    LIST_INSERT_AFTER(phead, pnew)

#define LIST_PREPEND(phead, pnew) \
    LIST_INSERT_BEFORE(phead, pnew)

#define LIST_REMOVE(pelem) \
    do { \
        (pelem)->prev->next = (pelem)->next; \
        (pelem)->next->prev = (pelem)->prev; \
    } while (0)

#define LIST_POP(pvar, pelem, member) \
    do { \
        LIST_REMOVE(pelem); \
        LIST_GET(pvar, pelem, member); \
    } while (0)

#define LIST_FOREACH_AFTER_INTERNAL_DO_NOT_USE(pvar, phead, pelem, member, dir) \
    for ( \
        struct { struct list *c, *n; } iter = { .c = (pelem)->dir, .n = (pelem)->dir->dir }; \
        (iter.c == (phead)) ? false : (((pvar) = CONTAINER_OF(iter.c, (pvar), member)), true); \
        iter.c = iter.n, iter.n = iter.n->dir \
    )

#define LIST_FOREACH_AFTER(pvar, phead, pelem, member) \
    LIST_FOREACH_AFTER_INTERNAL_DO_NOT_USE(pvar, phead, pelem, member, next)

#define LIST_FOREACH(pvar, phead, member) \
    LIST_FOREACH_AFTER(pvar, phead, phead, member)

#define LIST_FOREACH_REVERSE_BEFORE(pvar, phead, pelem, member) \
    LIST_FOREACH_AFTER_INTERNAL_DO_NOT_USE(pvar, phead, pelem, member, prev)

#define LIST_FOREACH_REVERSE(pvar, phead, member) \
    LIST_FOREACH_REVERSE_BEFORE(pvar, phead, phead, member)

#endif /* #ifndef SRC_COLLECTIONS_LIST_H */

