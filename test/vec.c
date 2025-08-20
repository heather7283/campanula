#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "collections/vec.h"

struct person {
    char name[32];
    int age;
};

static void print_person(const struct person *const p) {
    fprintf(stderr, "person(name=%s, age=%d)\n", p->name, p->age);
}

int main(void) {
    VEC(struct person) arr = VEC_INITALISER;

    VEC_APPEND(&arr, (&(struct person){.age = 31, .name = "Teto"}));
    VEC_APPEND(&arr, (&(struct person){.age = 16, .name = "Miku"}));

    struct person *new = VEC_EMPLACE_BACK(&arr);
    snprintf(new->name, sizeof(new->name), "Neru");
    new->age = 99;

    assert(VEC_SIZE(&arr) == 3);

    print_person(VEC_AT(&arr, 0));
    assert(VEC_AT(&arr, 0)->age == 31);
    assert(strcmp(VEC_AT(&arr, 0)->name, "Teto") == 0);

    print_person(VEC_AT(&arr, 1));
    assert(VEC_AT(&arr, 1)->age == 16);
    assert(strcmp(VEC_AT(&arr, 1)->name, "Miku") == 0);

    print_person(VEC_AT(&arr, 2));
    assert(VEC_AT(&arr, 2)->age == 99);
    assert(strcmp(VEC_AT(&arr, 2)->name, "Neru") == 0);

    VEC_FREE(&arr);

    assert(VEC_SIZE(&arr) == 0);

    VEC(wchar_t) str = VEC_INITALISER;

    VEC_APPEND(&str, L"a");
    assert(VEC_SIZE(&str) == 1);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"a", VEC_SIZE(&str)) == 0);

    VEC_APPEND_N(&str, L"bcd", 3);
    assert(VEC_SIZE(&str) == 4);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"abcd", VEC_SIZE(&str)) == 0);

    VEC_INSERT(&str, 0, L"e");
    assert(VEC_SIZE(&str) == 5);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"eabcd", VEC_SIZE(&str)) == 0);

    VEC_INSERT(&str, 2, L"f");
    assert(VEC_SIZE(&str) == 6);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"eafbcd", VEC_SIZE(&str)) == 0);

    VEC_INSERT_N(&str, 4, L"ghi", 3);
    assert(VEC_SIZE(&str) == 9);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"eafbghicd", VEC_SIZE(&str)) == 0);

    VEC_ERASE(&str, 4);
    assert(VEC_SIZE(&str) == 8);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"eafbhicd", VEC_SIZE(&str)) == 0);

    VEC_ERASE(&str, -1);
    assert(VEC_SIZE(&str) == 7);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"eafbhic", VEC_SIZE(&str)) == 0);

    VEC_ERASE_N(&str, 3, 4);
    assert(VEC_SIZE(&str) == 3);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"eaf", VEC_SIZE(&str)) == 0);

    wchar_t *a = VEC_EMPLACE_BACK(&str);
    *a = L'j';
    assert(VEC_SIZE(&str) == 4);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"eafj", VEC_SIZE(&str)) == 0);

    wchar_t *b = VEC_EMPLACE(&str, 0);
    *b = L'k';
    assert(VEC_SIZE(&str) == 5);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"keafj", VEC_SIZE(&str)) == 0);

    wchar_t *c = VEC_EMPLACE(&str, -2);
    *c = L'l';
    assert(VEC_SIZE(&str) == 6);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"kealfj", VEC_SIZE(&str)) == 0);

    VEC_INSERT(&str, -6, L"m");
    assert(VEC_SIZE(&str) == 7);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"mkealfj", VEC_SIZE(&str)) == 0);

    assert(*VEC_AT(&str, 0) == L'm'); assert(*VEC_AT(&str, -1) == L'j');
    assert(*VEC_AT(&str, 1) == L'k'); assert(*VEC_AT(&str, -2) == L'f');
    assert(*VEC_AT(&str, 2) == L'e'); assert(*VEC_AT(&str, -3) == L'l');
    assert(*VEC_AT(&str, 3) == L'a'); assert(*VEC_AT(&str, -4) == L'a');
    assert(*VEC_AT(&str, 4) == L'l'); assert(*VEC_AT(&str, -5) == L'e');
    assert(*VEC_AT(&str, 5) == L'f'); assert(*VEC_AT(&str, -6) == L'k');
    assert(*VEC_AT(&str, 6) == L'j'); assert(*VEC_AT(&str, -7) == L'm');

    VEC_EMPLACE_N_ZEROED(&str, 2, 3);
    assert(VEC_SIZE(&str) == 10);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"mk\0\0\0ealfj", VEC_SIZE(&str)) == 0);

    VEC_ERASE_N(&str, 2, 3);
    assert(VEC_SIZE(&str) == 7);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"mkealfj", VEC_SIZE(&str)) == 0);

    wchar_t *d = VEC_EMPLACE_N(&str, -2, 3);
    d[0] = L'0';
    d[1] = L'1';
    d[2] = L'2';
    assert(VEC_SIZE(&str) == 10);
    fprintf(stderr, "%.*ls\n", (int)VEC_SIZE(&str), VEC_DATA(&str)); fflush(stderr);
    assert(wmemcmp(VEC_DATA(&str), L"mkeal012fj", VEC_SIZE(&str)) == 0);

    VEC_FREE(&str);
}

