// Fuzzes QLMarkdown's Markdown pipeline: cmark-gfm (core + the extension set QLMarkdown ships) rendering
// to HTML, CommonMark and plain text. The first byte selects the parser options (see qlmd_render.h).
#include <stdint.h>
#include <stdlib.h>

#include "qlmd_render.h"

int LLVMFuzzerInitialize(int *argc, char ***argv) {
    (void)argc;
    (void)argv;
    qlmd_register();
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 1) {
        return -1;
    }
    free(qlmd_render(data + 1, size - 1, data[0], 1));
    return 0;
}
