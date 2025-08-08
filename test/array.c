#include <wchar.h>
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

    struct person *new = ARRAY_EMPLACE_BACK(&arr);
    snprintf(new->name, sizeof(new->name), "Neru");
    new->age = 99;

    assert(ARRAY_SIZE(&arr) == 3);

    print_person(ARRAY_AT(&arr, 0));
    assert(ARRAY_AT(&arr, 0)->age == 31);
    assert(strcmp(ARRAY_AT(&arr, 0)->name, "Teto") == 0);

    print_person(ARRAY_AT(&arr, 1));
    assert(ARRAY_AT(&arr, 1)->age == 16);
    assert(strcmp(ARRAY_AT(&arr, 1)->name, "Miku") == 0);

    print_person(ARRAY_AT(&arr, 2));
    assert(ARRAY_AT(&arr, 2)->age == 99);
    assert(strcmp(ARRAY_AT(&arr, 2)->name, "Neru") == 0);

    ARRAY_FREE(&arr);

    assert(ARRAY_SIZE(&arr) == 0);

    ARRAY(wchar_t) str = ARRAY_INITALISER;

    ARRAY_APPEND(&str, L"a");
    assert(ARRAY_SIZE(&str) == 1);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"a", ARRAY_SIZE(&str)) == 0);

    ARRAY_APPEND_N(&str, L"bcd", 3);
    assert(ARRAY_SIZE(&str) == 4);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"abcd", ARRAY_SIZE(&str)) == 0);

    ARRAY_INSERT(&str, 0, L"e");
    assert(ARRAY_SIZE(&str) == 5);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"eabcd", ARRAY_SIZE(&str)) == 0);

    ARRAY_INSERT(&str, 2, L"f");
    assert(ARRAY_SIZE(&str) == 6);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"eafbcd", ARRAY_SIZE(&str)) == 0);

    ARRAY_INSERT_N(&str, 4, L"ghi", 3);
    assert(ARRAY_SIZE(&str) == 9);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"eafbghicd", ARRAY_SIZE(&str)) == 0);

    ARRAY_ERASE(&str, 4);
    assert(ARRAY_SIZE(&str) == 8);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"eafbhicd", ARRAY_SIZE(&str)) == 0);

    ARRAY_ERASE(&str, -1);
    assert(ARRAY_SIZE(&str) == 7);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"eafbhic", ARRAY_SIZE(&str)) == 0);

    ARRAY_ERASE_N(&str, 3, 4);
    assert(ARRAY_SIZE(&str) == 3);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"eaf", ARRAY_SIZE(&str)) == 0);

    wchar_t *a = ARRAY_EMPLACE_BACK(&str);
    *a = L'j';
    assert(ARRAY_SIZE(&str) == 4);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"eafj", ARRAY_SIZE(&str)) == 0);

    wchar_t *b = ARRAY_EMPLACE(&str, 0);
    *b = L'k';
    assert(ARRAY_SIZE(&str) == 5);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"keafj", ARRAY_SIZE(&str)) == 0);

    wchar_t *c = ARRAY_EMPLACE(&str, -2);
    *c = L'l';
    assert(ARRAY_SIZE(&str) == 6);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"kealfj", ARRAY_SIZE(&str)) == 0);

    ARRAY_INSERT(&str, -6, L"m");
    assert(ARRAY_SIZE(&str) == 7);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"mkealfj", ARRAY_SIZE(&str)) == 0);

    assert(*ARRAY_AT(&str, 0) == L'm'); assert(*ARRAY_AT(&str, -1) == L'j');
    assert(*ARRAY_AT(&str, 1) == L'k'); assert(*ARRAY_AT(&str, -2) == L'f');
    assert(*ARRAY_AT(&str, 2) == L'e'); assert(*ARRAY_AT(&str, -3) == L'l');
    assert(*ARRAY_AT(&str, 3) == L'a'); assert(*ARRAY_AT(&str, -4) == L'a');
    assert(*ARRAY_AT(&str, 4) == L'l'); assert(*ARRAY_AT(&str, -5) == L'e');
    assert(*ARRAY_AT(&str, 5) == L'f'); assert(*ARRAY_AT(&str, -6) == L'k');
    assert(*ARRAY_AT(&str, 6) == L'j'); assert(*ARRAY_AT(&str, -7) == L'm');

    ARRAY_EMPLACE_N_ZEROED(&str, 2, 3);
    assert(ARRAY_SIZE(&str) == 10);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"mk\0\0\0ealfj", ARRAY_SIZE(&str)) == 0);

    ARRAY_ERASE_N(&str, 2, 3);
    assert(ARRAY_SIZE(&str) == 7);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"mkealfj", ARRAY_SIZE(&str)) == 0);

    wchar_t *d = ARRAY_EMPLACE_N(&str, -2, 3);
    d[0] = L'0';
    d[1] = L'1';
    d[2] = L'2';
    assert(ARRAY_SIZE(&str) == 10);
    fprintf(stderr, "%.*ls\n", (int)ARRAY_SIZE(&str), ARRAY_DATA(&str)); fflush(stderr);
    assert(wmemcmp(ARRAY_DATA(&str), L"mkeal012fj", ARRAY_SIZE(&str)) == 0);

    ARRAY_FREE(&str);
}

