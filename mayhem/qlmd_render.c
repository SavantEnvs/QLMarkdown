// QLMarkdown's rendering pipeline (Settings+render.swift) without the macOS-only parts: the cmark-gfm
// core extensions plus the cmark-extra extensions. `inlineimage` (reads host files) and
// `syntaxhighlight` (needs the highlight library) are not registered.
#include "qlmd_render.h"

#include <stdlib.h>
#include <string.h>

#include "cmark-gfm.h"
#include "cmark-gfm-core-extensions.h"
#include "cmark-gfm-extension_api.h"
#include "registry.h"
#include "plugin.h"
#include "syntax_extension.h"
#include "node.h"
#include "table.h"

#include "mention.h"
#include "wikilink.h"
#include "hashtag.h"
#include "emoji.h"
#include "heads.h"
#include "defl.h"
#include "admonition.h"
#include "highlight.h"
#include "math_ext.h"
#include "sub_ext.h"
#include "sup_ext.h"
#include "alert.h"

static int extra_extensions_registration(cmark_plugin *plugin) {
    cmark_plugin_register_syntax_extension(plugin, create_mention_extension());
    cmark_plugin_register_syntax_extension(plugin, create_wikilink_extension());
    cmark_plugin_register_syntax_extension(plugin, create_hashtag_extension());
    cmark_plugin_register_syntax_extension(plugin, create_emoji_extension());
    cmark_plugin_register_syntax_extension(plugin, create_heads_extension());
    cmark_plugin_register_syntax_extension(plugin, create_definitionlist_extension());
    cmark_plugin_register_syntax_extension(plugin, create_admonition_extension());
    cmark_plugin_register_syntax_extension(plugin, create_highlight_extension());
    cmark_plugin_register_syntax_extension(plugin, create_math_extension());
    cmark_plugin_register_syntax_extension(plugin, create_sup_extension());
    cmark_plugin_register_syntax_extension(plugin, create_sub_extension());
    cmark_plugin_register_syntax_extension(plugin, create_alert_extension());
    return 1;
}

static cmark_can_contain_func table_can_contain;

// Same wrapper as cmark-extra/extensions/extra-extensions.c: sub/sup/highlight/math form inside table cells.
static int table_can_contain_extra_inlines(cmark_syntax_extension *extension, cmark_node *node, cmark_node_type child_type) {
    if (node->type == CMARK_NODE_TABLE_CELL && CMARK_NODE_TYPE_INLINE_P(child_type)) {
        return 1;
    }
    return table_can_contain(extension, node, child_type);
}

void qlmd_register(void) {
    static int registered = 0;
    if (registered) {
        return;
    }
    cmark_register_plugin(extra_extensions_registration);
    cmark_gfm_core_extensions_ensure_registered();
    cmark_syntax_extension *table = cmark_find_syntax_extension("table");
    table_can_contain = table->can_contain_func;
    cmark_syntax_extension_set_can_contain_func(table, table_can_contain_extra_inlines);
    registered = 1;
}

static const char *const kExtensions[] = {
    "table", "autolink", "tagfilter", "tasklist", "strikethrough",
    "mention", "wikilink", "hashtag", "heads", "definitionlist", "admonition",
    "highlight", "sub", "sup", "alert", "emoji", "math",
};

char *qlmd_render(const unsigned char *data, size_t size, unsigned config, int render_all) {
    int options = CMARK_OPT_DEFAULT;
    if (config & QLMD_UNSAFE) options |= CMARK_OPT_UNSAFE;
    if (config & QLMD_HARDBREAKS) options |= CMARK_OPT_HARDBREAKS;
    if (config & QLMD_NOBREAKS) options |= CMARK_OPT_NOBREAKS;
    if (config & QLMD_SMART) options |= CMARK_OPT_SMART;
    if (config & QLMD_FOOTNOTES) options |= CMARK_OPT_FOOTNOTES;
    if (config & QLMD_STRIKE_DOUBLE_TILDE) options |= CMARK_OPT_STRIKETHROUGH_DOUBLE_TILDE;
    if (config & QLMD_SOURCEPOS) options |= CMARK_OPT_SOURCEPOS;

    cmark_parser *parser = cmark_parser_new(options);
    if (!parser) {
        return NULL;
    }
    for (size_t i = 0; i < sizeof(kExtensions) / sizeof(kExtensions[0]); i++) {
        cmark_syntax_extension *ext = cmark_find_syntax_extension(kExtensions[i]);
        if (!ext) {
            abort();
        }
        if (strcmp(kExtensions[i], "emoji") == 0) {
            cmark_syntax_extension_emoji_set_use_characters(ext, (config & QLMD_EMOJI_CHARS) != 0);
        }
        // Per-render reset, as Settings+render.swift does: the slug counter would otherwise carry over between inputs.
        if (strcmp(kExtensions[i], "heads") == 0) {
            heads_reset_slut_counter(ext);
        }
        cmark_parser_attach_syntax_extension(parser, ext);
    }

    cmark_parser_feed(parser, (const char *)data, size);
    cmark_node *doc = cmark_parser_finish(parser);
    char *html = cmark_render_html(doc, options, cmark_parser_get_syntax_extensions(parser));
    if (render_all) {
        free(cmark_render_commonmark(doc, options, 72));
        free(cmark_render_plaintext(doc, options, 72));
    }
    cmark_node_free(doc);
    cmark_parser_free(parser);
    return html;
}
