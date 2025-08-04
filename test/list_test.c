#include <stdio.h>
#include <assert.h>

#include "collections/list.h"
#include "xmalloc.h"

struct foo {
    int n;
    LIST_ENTRY link;
};

static void assert_list(LIST_HEAD *l, const int a[], size_t s) {
    fprintf(stderr, "----------------------------------------------\n");
    size_t i = 0;
    const struct foo *pfoo;
    LIST_FOREACH_REVERSE(pfoo, l, link) {
        fprintf(stderr, "expected %d, found %d\n", a[i], pfoo->n);
        assert(a[i] == pfoo->n);
        i += 1;
    }
    assert(i == s);
}

#define ASSERT_LIST(phead, ...) \
    assert_list((phead), (int[]){__VA_ARGS__}, sizeof((int[]){__VA_ARGS__}) / sizeof(int))

int main(void) {
    LIST_HEAD l = LIST_INITIALISER(&l);

    struct foo *foo1 = xcalloc(1, sizeof(struct foo));
    foo1->n = 1;
    LIST_APPEND(&l, &foo1->link);
    ASSERT_LIST(&l, 1);

    struct foo *foo2 = xcalloc(1, sizeof(struct foo));
    foo2->n = 2;
    LIST_APPEND(&l, &foo2->link);
    ASSERT_LIST(&l, 1, 2);

    struct foo *foo3 = xcalloc(1, sizeof(struct foo));
    foo3->n = 3;
    LIST_APPEND(&l, &foo3->link);
    ASSERT_LIST(&l, 1, 2, 3);

    struct foo *foo4 = xcalloc(1, sizeof(struct foo));
    foo4->n = 4;
    LIST_APPEND(&l, &foo4->link);
    ASSERT_LIST(&l, 1, 2, 3, 4);

    LIST_REMOVE(&foo3->link);
    ASSERT_LIST(&l, 1, 2, 4);

    LIST_REMOVE(&foo1->link);
    ASSERT_LIST(&l, 2, 4);

    LIST_APPEND(&l, &foo1->link);
    ASSERT_LIST(&l, 2, 4, 1);

    LIST_PREPEND(&l, &foo3->link);
    ASSERT_LIST(&l, 3, 2, 4, 1);

    struct foo *foo;
    LIST_FOREACH(foo, &l, link) {
        free(foo);
    }

    return 0;
}

