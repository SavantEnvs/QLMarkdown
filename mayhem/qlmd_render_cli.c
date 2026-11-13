// Test oracle driver: renders stdin Markdown to HTML on stdout with the given config byte (decimal argv[1]).
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qlmd_render.h"

int main(int argc, char **argv) {
    unsigned config = argc > 1 ? (unsigned)strtoul(argv[1], NULL, 10) : 0;
    size_t cap = 4096, len = 0;
    unsigned char *buf = malloc(cap);
    size_t n;
    while ((n = fread(buf + len, 1, cap - len, stdin)) > 0) {
        len += n;
        if (len == cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
    }
    qlmd_register();
    char *html = qlmd_render(buf, len, config, 0);
    if (!html) {
        return 2;
    }
    fputs(html, stdout);
    free(html);
    free(buf);
    return 0;
}
