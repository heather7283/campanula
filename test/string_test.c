#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "collections.h"

#define STREQ(a, b) (strcmp((a), (b)) == 0)

int main(void) {
    struct string str = {0};

    string_append(&str, "aboba");
    puts(str.str); fflush(stdout);
    assert(STREQ(str.str, "aboba"));

    string_appendf(&str, " %d %d", 228, 100500);
    puts(str.str); fflush(stdout);
    assert(STREQ(str.str, "aboba 228 100500"));

    string_appendf(&str, " %s", "skibidi toilet");
    puts(str.str); fflush(stdout);
    assert(STREQ(str.str, "aboba 228 100500 skibidi toilet"));

    string_clear(&str);
    puts(str.str); fflush(stdout);
    assert(STREQ(str.str, ""));

    string_free(&str);
}

