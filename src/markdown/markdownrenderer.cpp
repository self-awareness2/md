#include "markdown/markdownrenderer.h"

#include <cmark-gfm-core-extensions.h>
#include <cmark-gfm-extension_api.h>
#include <cmark-gfm.h>

#include <QByteArray>

#include <array>
#include <memory>

namespace marknote::markdown {
namespace {

struct ParserDeleter {
    void operator()(cmark_parser *parser) const
    {
        cmark_parser_free(parser);
    }
};

struct NodeDeleter {
    void operator()(cmark_node *node) const
    {
        cmark_node_free(node);
    }
};

struct BufferDeleter {
    void operator()(char *buffer) const
    {
        free(buffer);
    }
};

using ParserPtr = std::unique_ptr<cmark_parser, ParserDeleter>;
using NodePtr = std::unique_ptr<cmark_node, NodeDeleter>;
using BufferPtr = std::unique_ptr<char, BufferDeleter>;

void attachGfmExtensions(cmark_parser *parser)
{
    cmark_gfm_core_extensions_ensure_registered();

    constexpr std::array extensionNames {
        "autolink",
        "strikethrough",
        "table",
        "tagfilter",
        "tasklist",
    };

    for (const char *name : extensionNames) {
        if (cmark_syntax_extension *extension = cmark_find_syntax_extension(name)) {
            cmark_parser_attach_syntax_extension(parser, extension);
        }
    }
}

} // namespace

QString MarkdownRenderer::toHtml(const QString &markdown)
{
    const QByteArray utf8 = markdown.toUtf8();
    ParserPtr parser(cmark_parser_new(CMARK_OPT_DEFAULT));
    if (!parser) {
        return {};
    }

    attachGfmExtensions(parser.get());
    cmark_parser_feed(parser.get(), utf8.constData(), static_cast<size_t>(utf8.size()));

    NodePtr document(cmark_parser_finish(parser.get()));
    if (!document) {
        return {};
    }

    // CMARK_OPT_SAFE suppresses raw HTML and unsafe URL schemes.
    BufferPtr html(cmark_render_html(document.get(), CMARK_OPT_SAFE,
                                     cmark_parser_get_syntax_extensions(parser.get())));
    return html ? QString::fromUtf8(html.get()) : QString();
}

} // namespace marknote::markdown

