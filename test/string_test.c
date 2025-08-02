#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "collections/string.h"

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

    string_append_urlencode(&str, "ГОООООООООЛ!!! 🐻⚽");
    puts(str.str); fflush(stdout);
    assert(STREQ(str.str, "%D0%93%D0%9E%D0%9E%D0%9E%D0%9E%D0%9E%D0%9E%D0%9E%D0%9E%D0%9E%D0%9B%21%21%21%20%F0%9F%90%BB%E2%9A%BD"));

    string_clear(&str);
    puts(str.str); fflush(stdout);
    assert(STREQ(str.str, ""));

    string_append_urlencode(&str, "aboba");
    puts(str.str); fflush(stdout);
    assert(STREQ(str.str, "aboba"));

    string_append_urlencode(&str, "абоба");
    puts(str.str); fflush(stdout);
    assert(STREQ(str.str, "aboba%D0%B0%D0%B1%D0%BE%D0%B1%D0%B0"));

    string_free(&str);
}

