#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "collections/array.h"

struct person {
    char name[32];
    int age;
};

static void print_person(const struct person *const p) {
    fprintf(stderr, "person(name=%s, age=%d)\n", p->name, p->age);
}

int main(void) {
    ARRAY(struct person) arr = ARRAY_INITALISER;

    ARRAY_APPEND(&arr, (&(struct person){.age = 31, .name = "Teto"}));
    ARRAY_APPEND(&arr, (&(struct person){.age = 16, .name = "Miku"}));

    struct person *new = ARRAY_EMPLACE(&arr);
    snprintf(new->name, sizeof(new->name), "Neru");
    new->age = 99;

    assert(ARRAY_SIZE(&arr) == 3);

    print_person(&ARRAY_AT(&arr, 0));
    assert(ARRAY_AT(&arr, 0).age == 31);
    assert(strcmp(ARRAY_AT(&arr, 0).name, "Teto") == 0);

    print_person(&ARRAY_AT(&arr, 1));
    assert(ARRAY_AT(&arr, 1).age == 16);
    assert(strcmp(ARRAY_AT(&arr, 1).name, "Miku") == 0);

    print_person(&ARRAY_AT(&arr, 2));
    assert(ARRAY_AT(&arr, 2).age == 99);
    assert(strcmp(ARRAY_AT(&arr, 2).name, "Neru") == 0);

    ARRAY_FREE(&arr);

    assert(ARRAY_SIZE(&arr) == 0);

    ARRAY(char) str = ARRAY_INITALISER;

    ARRAY_EXTEND(&str, "Hello, world!", sizeof("Hello, world!"));

    assert(ARRAY_SIZE(&str) == sizeof("Hello, world!"));
    assert(strcmp(ARRAY_DATA(&str), "Hello, world!") == 0);

    ARRAY_FREE(&str);
}

