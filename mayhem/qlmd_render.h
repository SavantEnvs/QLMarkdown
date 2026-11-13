#ifndef QLMD_RENDER_H
#define QLMD_RENDER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Config byte: bits select the cmark options (and the emoji mode) for one render.
enum {
    QLMD_UNSAFE = 1 << 0,
    QLMD_HARDBREAKS = 1 << 1,
    QLMD_NOBREAKS = 1 << 2,
    QLMD_SMART = 1 << 3,
    QLMD_FOOTNOTES = 1 << 4,
    QLMD_STRIKE_DOUBLE_TILDE = 1 << 5,
    QLMD_SOURCEPOS = 1 << 6,
    QLMD_EMOJI_CHARS = 1 << 7,
};

// Registers the cmark-gfm core + QLMarkdown extra extensions (once).
void qlmd_register(void);

// Parses `data` with every extension attached and renders HTML, CommonMark and plain text.
// Returns the HTML (malloc'd; caller frees) or NULL if the parser could not be created.
char *qlmd_render(const unsigned char *data, size_t size, unsigned config, int render_all);

#ifdef __cplusplus
}
#endif

#endif
