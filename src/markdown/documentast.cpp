#include "markdown/documentast.h"

#include <cmark-gfm-core-extensions.h>
#include <cmark-gfm-extension_api.h>
#include <cmark-gfm.h>

#include <QByteArray>
#include <QRegularExpression>
#include <QStringList>

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

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

using ParserPtr = std::unique_ptr<cmark_parser, ParserDeleter>;
using NodePtr = std::unique_ptr<cmark_node, NodeDeleter>;

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

struct LineIndex {
    QStringList lines;
    std::vector<qsizetype> starts;

    explicit LineIndex(const QString &text)
    {
        starts.push_back(0);
        qsizetype cursor = 0;
        while (cursor <= text.size()) {
            const qsizetype next = text.indexOf(QLatin1Char('\n'), cursor);
            if (next < 0) {
                lines.push_back(text.mid(cursor));
                break;
            }
            lines.push_back(text.mid(cursor, next - cursor));
            cursor = next + 1;
            starts.push_back(cursor);
            if (cursor == text.size()) {
                lines.push_back(QString());
                break;
            }
        }
    }

    [[nodiscard]] qsizetype offset(int line1Based, int column1Based) const
    {
        if (line1Based < 1 || line1Based > static_cast<int>(lines.size())) {
            return 0;
        }
        const qsizetype lineStart = starts[static_cast<size_t>(line1Based - 1)];
        const QString &line = lines[line1Based - 1];
        // cmark columns are 1-based UTF-8 byte offsets into the line.
        const QByteArray utf8 = line.toUtf8();
        const int byteIndex = qBound(0, column1Based - 1, static_cast<int>(utf8.size()));
        return lineStart + QString::fromUtf8(utf8.left(byteIndex)).size();
    }
};

QString sliceSource(const QString &markdown, const LineIndex &index, cmark_node *node)
{
    const int startLine = cmark_node_get_start_line(node);
    const int startColumn = cmark_node_get_start_column(node);
    const int endLine = cmark_node_get_end_line(node);
    const int endColumn = cmark_node_get_end_column(node);
    if (startLine <= 0 || endLine <= 0) {
        return {};
    }

    qsizetype begin = index.offset(startLine, startColumn);
    // end_column is inclusive in cmark sourcepos.
    qsizetype end = index.offset(endLine, endColumn + 1);
    begin = qBound(qsizetype{0}, begin, markdown.size());
    end = qBound(begin, end, markdown.size());
    return markdown.mid(begin, end - begin);
}

SourceSpan spanFor(const LineIndex &index, cmark_node *node, qsizetype sourceSize)
{
    SourceSpan span;
    const int startLine = cmark_node_get_start_line(node);
    const int startColumn = cmark_node_get_start_column(node);
    const int endLine = cmark_node_get_end_line(node);
    const int endColumn = cmark_node_get_end_column(node);
    if (startLine <= 0 || endLine <= 0) {
        return span;
    }
    span.beginUtf16 = qBound(qsizetype{0}, index.offset(startLine, startColumn), sourceSize);
    span.endUtf16 = qBound(span.beginUtf16, index.offset(endLine, endColumn + 1), sourceSize);
    return span;
}

QString stripHeadingMarkers(const QString &source, int *levelOut)
{
    static const QRegularExpression atx(QStringLiteral(R"(^(#{1,6})\s+(.*?)(?:\s+#*)?\s*$)"));
    const QRegularExpressionMatch match = atx.match(source.trimmed());
    if (match.hasMatch()) {
        if (levelOut) {
            *levelOut = match.captured(1).size();
        }
        return match.captured(2);
    }
    return source;
}

QString stripQuoteMarkers(const QString &source)
{
    const QStringList parts = source.split(QLatin1Char('\n'));
    QStringList cleaned;
    cleaned.reserve(parts.size());
    for (const QString &line : parts) {
        if (line.startsWith(QLatin1String("> "))) {
            cleaned.push_back(line.mid(2));
        } else if (line == QLatin1String(">")) {
            cleaned.push_back(QString());
        } else {
            cleaned.push_back(line);
        }
    }
    return cleaned.join(QLatin1Char('\n'));
}

QString stripListMarkers(const QString &source, bool *ordered, bool *task, bool *checked)
{
    static const QRegularExpression marker(
        QStringLiteral(R"(^\s*(?:([-+*])|(\d+)[.)])\s+(?:\[([ xX])\]\s+)?(.*)$)"));
    const QStringList parts = source.split(QLatin1Char('\n'));
    if (parts.isEmpty()) {
        return source;
    }
    const QRegularExpressionMatch match = marker.match(parts.first());
    if (!match.hasMatch()) {
        return source;
    }
    if (ordered) {
        *ordered = !match.captured(2).isEmpty();
    }
    if (task) {
        *task = !match.captured(3).isEmpty();
    }
    if (checked) {
        *checked = match.captured(3).compare(QLatin1String("x"), Qt::CaseInsensitive) == 0;
    }
    QStringList cleaned;
    cleaned.push_back(match.captured(4));
    for (int i = 1; i < parts.size(); ++i) {
        QString line = parts.at(i);
        if (line.startsWith(QLatin1String("  "))) {
            line.remove(0, 2);
        }
        cleaned.push_back(line);
    }
    return cleaned.join(QLatin1Char('\n'));
}

QString stripCodeFence(const QString &source, QString *language)
{
    static const QRegularExpression open(QStringLiteral(R"(^(`{3,}|~{3,})([^\n]*)\n?)"));
    const QRegularExpressionMatch match = open.match(source);
    if (!match.hasMatch()) {
        if (source.startsWith(QLatin1String("    "))) {
            const QStringList parts = source.split(QLatin1Char('\n'));
            QStringList cleaned;
            for (const QString &line : parts) {
                cleaned.push_back(line.startsWith(QLatin1String("    ")) ? line.mid(4) : line);
            }
            return cleaned.join(QLatin1Char('\n'));
        }
        return source;
    }
    if (language) {
        *language = match.captured(2).trimmed();
    }
    QString body = source.mid(match.capturedLength());
    const QString fence = match.captured(1);
    if (body.endsWith(QLatin1Char('\n') + fence)) {
        body.chop(fence.size() + 1);
    } else if (body.endsWith(fence)) {
        body.chop(fence.size());
    }
    if (body.endsWith(QLatin1Char('\n'))) {
        body.chop(1);
    }
    return body;
}

AstBlock makeBlock(std::uint64_t id, BlockKind kind, const QString &source, SourceSpan span)
{
    AstBlock block;
    block.id = id;
    block.kind = kind;
    block.source = source;
    block.sourceSpan = span;
    block.displayText = source;

    switch (kind) {
    case BlockKind::Heading:
        block.displayText = stripHeadingMarkers(source, &block.level);
        if (block.level <= 0) {
            block.level = 1;
        }
        break;
    case BlockKind::BlockQuote:
        block.displayText = stripQuoteMarkers(source);
        break;
    case BlockKind::ListItem: {
        bool ordered = false;
        bool task = false;
        bool checked = false;
        block.displayText = stripListMarkers(source, &ordered, &task, &checked);
        block.ordered = ordered;
        block.task = task;
        block.taskChecked = checked;
        break;
    }
    case BlockKind::CodeBlock:
        block.displayText = stripCodeFence(source, &block.language);
        break;
    case BlockKind::ThematicBreak:
        block.displayText = QStringLiteral("---");
        break;
    case BlockKind::FrontMatter:
    case BlockKind::Paragraph:
    case BlockKind::HtmlBlock:
    case BlockKind::Table:
    case BlockKind::Unknown:
        break;
    }
    return block;
}

std::pair<QString, QString> splitFrontMatter(const QString &markdown)
{
    if (!markdown.startsWith(QLatin1String("---\n"))
        && !markdown.startsWith(QLatin1String("---\r\n"))) {
        return {{}, markdown};
    }
    const qsizetype firstBreak = markdown.indexOf(QLatin1Char('\n'));
    if (firstBreak < 0) {
        return {{}, markdown};
    }
    const qsizetype close = markdown.indexOf(QStringLiteral("\n---"), firstBreak + 1);
    if (close < 0) {
        return {{}, markdown};
    }
    qsizetype bodyStart = close + 4; // \n---
    if (bodyStart < markdown.size() && markdown.at(bodyStart) == QLatin1Char('\r')) {
        ++bodyStart;
    }
    if (bodyStart < markdown.size() && markdown.at(bodyStart) == QLatin1Char('\n')) {
        ++bodyStart;
    }
    return {markdown.left(bodyStart).trimmed(), markdown.mid(bodyStart)};
}

void appendListItems(
    cmark_node *list,
    const QString &markdown,
    const LineIndex &index,
    std::vector<AstBlock> &blocks,
    std::uint64_t &nextId)
{
    for (cmark_node *item = cmark_node_first_child(list); item; item = cmark_node_next(item)) {
        if (cmark_node_get_type(item) != CMARK_NODE_ITEM) {
            continue;
        }
        const SourceSpan span = spanFor(index, item, markdown.size());
        QString source = markdown.mid(span.beginUtf16, span.endUtf16 - span.beginUtf16);
        if (source.isEmpty()) {
            source = sliceSource(markdown, index, item);
        }
        AstBlock block = makeBlock(nextId++, BlockKind::ListItem, source, span);
        block.ordered = cmark_node_get_list_type(list) == CMARK_ORDERED_LIST;
        blocks.push_back(std::move(block));
    }
}

BlockKind mapNodeType(cmark_node_type type, cmark_node *node)
{
    switch (type) {
    case CMARK_NODE_PARAGRAPH:
        return BlockKind::Paragraph;
    case CMARK_NODE_HEADING:
        return BlockKind::Heading;
    case CMARK_NODE_BLOCK_QUOTE:
        return BlockKind::BlockQuote;
    case CMARK_NODE_CODE_BLOCK:
        return BlockKind::CodeBlock;
    case CMARK_NODE_HTML_BLOCK:
        return BlockKind::HtmlBlock;
    case CMARK_NODE_THEMATIC_BREAK:
        return BlockKind::ThematicBreak;
    default:
        if (node && QString::fromUtf8(cmark_node_get_type_string(node)) == QLatin1String("table")) {
            return BlockKind::Table;
        }
        return BlockKind::Unknown;
    }
}

std::uint64_t contentKey(const AstBlock &block)
{
    return qHash(block.displayText)
           ^ (static_cast<std::uint64_t>(block.kind) << 1)
           ^ (static_cast<std::uint64_t>(block.level) << 8);
}

} // namespace

QString DocumentAst::kindName(BlockKind kind)
{
    switch (kind) {
    case BlockKind::Paragraph:
        return QStringLiteral("paragraph");
    case BlockKind::Heading:
        return QStringLiteral("heading");
    case BlockKind::BlockQuote:
        return QStringLiteral("quote");
    case BlockKind::ListItem:
        return QStringLiteral("list");
    case BlockKind::CodeBlock:
        return QStringLiteral("code");
    case BlockKind::HtmlBlock:
        return QStringLiteral("html");
    case BlockKind::Table:
        return QStringLiteral("table");
    case BlockKind::ThematicBreak:
        return QStringLiteral("rule");
    case BlockKind::FrontMatter:
        return QStringLiteral("frontmatter");
    case BlockKind::Unknown:
        return QStringLiteral("unknown");
    }
    return QStringLiteral("unknown");
}

BlockKind DocumentAst::kindFromName(const QString &name)
{
    if (name == QLatin1String("heading")) {
        return BlockKind::Heading;
    }
    if (name == QLatin1String("quote")) {
        return BlockKind::BlockQuote;
    }
    if (name == QLatin1String("list")) {
        return BlockKind::ListItem;
    }
    if (name == QLatin1String("code")) {
        return BlockKind::CodeBlock;
    }
    if (name == QLatin1String("html")) {
        return BlockKind::HtmlBlock;
    }
    if (name == QLatin1String("table")) {
        return BlockKind::Table;
    }
    if (name == QLatin1String("rule")) {
        return BlockKind::ThematicBreak;
    }
    if (name == QLatin1String("frontmatter")) {
        return BlockKind::FrontMatter;
    }
    if (name == QLatin1String("unknown")) {
        return BlockKind::Unknown;
    }
    return BlockKind::Paragraph;
}

QString DocumentAst::serializeBlock(const AstBlock &block)
{
    switch (block.kind) {
    case BlockKind::Heading: {
        const int level = qBound(1, block.level <= 0 ? 1 : block.level, 6);
        return QString(level, QLatin1Char('#')) + QLatin1Char(' ') + block.displayText.trimmed();
    }
    case BlockKind::BlockQuote: {
        const QStringList parts = block.displayText.split(QLatin1Char('\n'));
        QStringList lines;
        for (const QString &line : parts) {
            lines.push_back(line.isEmpty() ? QStringLiteral(">") : QStringLiteral("> ") + line);
        }
        return lines.join(QLatin1Char('\n'));
    }
    case BlockKind::ListItem: {
        QString marker;
        if (block.task) {
            marker = QStringLiteral("- [%1] ")
                         .arg(block.taskChecked ? QLatin1Char('x') : QLatin1Char(' '));
        } else if (block.ordered) {
            marker = QStringLiteral("1. ");
        } else {
            marker = QStringLiteral("- ");
        }
        const QStringList parts = block.displayText.split(QLatin1Char('\n'));
        QStringList lines;
        for (int i = 0; i < parts.size(); ++i) {
            lines.push_back(i == 0 ? marker + parts.at(i) : QStringLiteral("  ") + parts.at(i));
        }
        return lines.join(QLatin1Char('\n'));
    }
    case BlockKind::CodeBlock: {
        const QString fence = QStringLiteral("```");
        return fence + block.language + QLatin1Char('\n') + block.displayText + QLatin1Char('\n')
               + fence;
    }
    case BlockKind::ThematicBreak:
        return QStringLiteral("---");
    case BlockKind::FrontMatter:
    case BlockKind::Paragraph:
    case BlockKind::HtmlBlock:
    case BlockKind::Table:
    case BlockKind::Unknown:
        return block.displayText;
    }
    return block.displayText;
}

QString DocumentAst::replaceBlockSource(
    const QString &document,
    const AstBlock &block,
    const QString &newSource)
{
    if (block.sourceSpan.beginUtf16 < 0 || block.sourceSpan.endUtf16 < block.sourceSpan.beginUtf16
        || block.sourceSpan.endUtf16 > document.size()) {
        return document;
    }
    QString result = document;
    result.replace(
        block.sourceSpan.beginUtf16,
        block.sourceSpan.endUtf16 - block.sourceSpan.beginUtf16,
        newSource);
    return result;
}

QVariantMap DocumentAst::toVariantMap(const AstBlock &block)
{
    QVariantMap map;
    map.insert(QStringLiteral("id"), QVariant::fromValue(static_cast<qulonglong>(block.id)));
    map.insert(QStringLiteral("kind"), kindName(block.kind));
    map.insert(QStringLiteral("level"), block.level);
    map.insert(QStringLiteral("ordered"), block.ordered);
    map.insert(QStringLiteral("task"), block.task);
    map.insert(QStringLiteral("taskChecked"), block.taskChecked);
    map.insert(QStringLiteral("language"), block.language);
    map.insert(QStringLiteral("source"), block.source);
    map.insert(QStringLiteral("displayText"), block.displayText);
    map.insert(QStringLiteral("begin"), static_cast<int>(block.sourceSpan.beginUtf16));
    map.insert(QStringLiteral("end"), static_cast<int>(block.sourceSpan.endUtf16));
    return map;
}

QVariantList DocumentAst::toVariantList(const std::vector<AstBlock> &blocks)
{
    QVariantList list;
    list.reserve(static_cast<int>(blocks.size()));
    for (const AstBlock &block : blocks) {
        list.push_back(toVariantMap(block));
    }
    return list;
}

QVariantList DocumentAst::outline(const std::vector<AstBlock> &blocks)
{
    QVariantList list;
    for (const AstBlock &block : blocks) {
        if (block.kind != BlockKind::Heading) {
            continue;
        }
        QVariantMap entry;
        entry.insert(QStringLiteral("title"), block.displayText);
        entry.insert(QStringLiteral("level"), block.level <= 0 ? 1 : block.level);
        entry.insert(QStringLiteral("position"), static_cast<int>(block.sourceSpan.beginUtf16));
        list.push_back(entry);
    }
    return list;
}

std::vector<AstBlock> DocumentAst::parse(const QString &markdown)
{
    std::vector<AstBlock> blocks;
    std::uint64_t nextId = 1;

    const auto [frontMatter, body] = splitFrontMatter(markdown);
    qsizetype bodyOffset = 0;
    if (!frontMatter.isEmpty()) {
        AstBlock meta = makeBlock(
            nextId++,
            BlockKind::FrontMatter,
            frontMatter,
            SourceSpan{0, frontMatter.size()});
        meta.displayText = frontMatter;
        blocks.push_back(std::move(meta));
        bodyOffset = markdown.size() - body.size();
    }

    const LineIndex index(markdown);
    const QByteArray utf8 = body.toUtf8();
    ParserPtr parser(cmark_parser_new(CMARK_OPT_DEFAULT | CMARK_OPT_SOURCEPOS));
    if (!parser) {
        return blocks;
    }
    attachGfmExtensions(parser.get());
    cmark_parser_feed(parser.get(), utf8.constData(), static_cast<size_t>(utf8.size()));
    NodePtr document(cmark_parser_finish(parser.get()));
    if (!document) {
        return blocks;
    }

    // When front matter was stripped, cmark positions are relative to `body`.
    // Remap by adjusting against the original markdown through line index on the
    // full text only when there is no front matter; otherwise rebuild spans from
    // extracted source search.
    const bool hasFrontMatter = bodyOffset > 0;
    LineIndex bodyIndex(body);

    for (cmark_node *child = cmark_node_first_child(document.get()); child;
         child = cmark_node_next(child)) {
        const cmark_node_type type = cmark_node_get_type(child);
        if (type == CMARK_NODE_LIST) {
            if (hasFrontMatter) {
                for (cmark_node *item = cmark_node_first_child(child); item;
                     item = cmark_node_next(item)) {
                    if (cmark_node_get_type(item) != CMARK_NODE_ITEM) {
                        continue;
                    }
                    QString source = sliceSource(body, bodyIndex, item);
                    SourceSpan span;
                    const int at = markdown.indexOf(source, static_cast<int>(bodyOffset));
                    if (at >= 0) {
                        span = SourceSpan{at, at + source.size()};
                    }
                    AstBlock block = makeBlock(nextId++, BlockKind::ListItem, source, span);
                    block.ordered = cmark_node_get_list_type(child) == CMARK_ORDERED_LIST;
                    blocks.push_back(std::move(block));
                }
            } else {
                appendListItems(child, markdown, index, blocks, nextId);
            }
            continue;
        }

        BlockKind kind = mapNodeType(type, child);
        QString source;
        SourceSpan span;
        if (hasFrontMatter) {
            source = sliceSource(body, bodyIndex, child);
            const int at = markdown.indexOf(source, static_cast<int>(bodyOffset));
            if (at >= 0) {
                span = SourceSpan{at, at + source.size()};
            }
        } else {
            span = spanFor(index, child, markdown.size());
            source = markdown.mid(span.beginUtf16, span.endUtf16 - span.beginUtf16);
            if (source.isEmpty()) {
                source = sliceSource(markdown, index, child);
            }
        }

        AstBlock block = makeBlock(nextId++, kind, source, span);
        if (kind == BlockKind::Heading) {
            block.level = cmark_node_get_heading_level(child);
            if (block.level <= 0) {
                stripHeadingMarkers(source, &block.level);
            }
            block.displayText = stripHeadingMarkers(source, nullptr);
        }
        if (kind == BlockKind::CodeBlock) {
            if (const char *lang = cmark_node_get_fence_info(child)) {
                block.language = QString::fromUtf8(lang).trimmed();
            }
            block.displayText = stripCodeFence(source, &block.language);
        }
        blocks.push_back(std::move(block));
    }

    return blocks;
}

std::vector<AstBlock> DocumentAst::reconcile(
    const std::vector<AstBlock> &previous,
    const QString &markdown)
{
    std::vector<AstBlock> next = parse(markdown);
    if (previous.empty()) {
        return next;
    }

    std::vector<bool> used(previous.size(), false);
    std::uint64_t maxId = 0;
    for (const AstBlock &block : previous) {
        maxId = std::max(maxId, block.id);
    }

    for (AstBlock &block : next) {
        int best = -1;
        int bestDistance = 1'000'000;
        const std::uint64_t key = contentKey(block);
        for (size_t i = 0; i < previous.size(); ++i) {
            if (used[i] || previous[i].kind != block.kind) {
                continue;
            }
            if (contentKey(previous[i]) != key && previous[i].displayText != block.displayText) {
                continue;
            }
            const int distance = static_cast<int>(
                std::abs(previous[i].sourceSpan.beginUtf16 - block.sourceSpan.beginUtf16));
            if (distance < bestDistance) {
                bestDistance = distance;
                best = static_cast<int>(i);
            }
        }
        if (best >= 0 && bestDistance < 4000) {
            block.id = previous[static_cast<size_t>(best)].id;
            used[static_cast<size_t>(best)] = true;
        } else {
            block.id = ++maxId;
        }
    }
    return next;
}

} // namespace marknote::markdown
