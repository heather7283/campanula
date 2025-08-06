#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "commands/parse.h"
#include "xmalloc.h"

static const struct {
    const char *string;
    const char **args;
} strings[] = {
    {
        .string = "Great holes secretly are digged",
        .args = (const char *[]){ "Great", "holes", "secretly", "are", "digged", NULL },
    },
    {
        .string = "where earths pores ought to suffice",
        .args = (const char *[]){ "where", "earths", "pores", "ought", "to", "suffice", NULL },
    },
    {
        .string = "and things have learnt to walk",
        .args = (const char *[]){ "and", "things", "have", "learnt", "to", "walk", NULL },
    },
    {
        .string = "that ought to crawl",
        .args = (const char *[]){ "that", "ought", "to", "crawl", NULL },
    },
};

bool streq(const char *s1, const char *s2) {
    if (s1 == NULL || s2 == NULL) {
        return s1 == s2;
    } else {
        return strcmp(s1, s2) == 0;
    }
}

int main(void) {
    for (size_t i = 0; i < (sizeof(strings) / sizeof(strings[0])); i++) {
        char *str = xstrdup(strings[i].string);
        const char **args = strings[i].args;

        fprintf(stderr, "Parsing string: %s\n", str);

        char **argv;
        size_t argc = command_into_argv(str, &argv);

        fprintf(stderr, "Got %zu args\n", argc);
        for (size_t j = 0; j < argc; j++) {
            fprintf(stderr, "\tArg %zu: expected ┤%s├, got ┤%s├\n", j, args[j], argv[j]);
            assert(streq(args[j], argv[j]));
        }

        free(argv);
        free(str);
    }
}

