#include <string.h>
#include <assert.h>

#include "collections.h"

struct person {
    char name[32];
    int age;
};

int main(void) {
    ARRAY(struct person) arr = ARRAY_INITALISER;

    ARRAY_APPEND(&arr, (&(struct person){.age = 31, .name = "Teto"}));
    ARRAY_APPEND(&arr, (&(struct person){.age = 16, .name = "Miku"}));

    assert(ARRAY_SIZE(&arr) == 2);

    assert(ARRAY_AT(&arr, 0).age == 31);
    assert(strcmp(ARRAY_AT(&arr, 0).name, "Teto") == 0);

    assert(ARRAY_AT(&arr, 1).age == 16);
    assert(strcmp(ARRAY_AT(&arr, 1).name, "Miku") == 0);

    ARRAY_FREE(&arr);

    assert(ARRAY_SIZE(&arr) == 0);

    ARRAY(char) str = ARRAY_INITALISER;

    ARRAY_EXTEND(&str, "Hello, world!", sizeof("Hello, world!"));

    assert(ARRAY_SIZE(&str) == sizeof("Hello, world!"));
    assert(strcmp(ARRAY_DATA(&str), "Hello, world!") == 0);

    ARRAY_FREE(&str);
}

