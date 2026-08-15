#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtTypes>

#include <cstdint>
#include <vector>

namespace marknote::markdown {

enum class BlockKind {
    Paragraph,
    Heading,
    BlockQuote,
    ListItem,
    CodeBlock,
    HtmlBlock,
    Table,
    ThematicBreak,
    FrontMatter,
    Unknown,
};

struct SourceSpan {
    qsizetype beginUtf16 = 0;
    qsizetype endUtf16 = 0;
};

struct AstBlock {
    std::uint64_t id = 0;
    BlockKind kind = BlockKind::Paragraph;
    int level = 0;
    bool ordered = false;
    bool task = false;
    bool taskChecked = false;
    QString language;
    QString source;
    QString displayText;
    SourceSpan sourceSpan;
};

class DocumentAst final
{
public:
    // Parses Markdown into top-level / list-item project AST blocks with
    // UTF-16 source spans. Unknown nodes stay as non-destructive source blocks.
    [[nodiscard]] static std::vector<AstBlock> parse(const QString &markdown);

    // Reassigns stable IDs when kind and content identity still match nearby
    // previous blocks; otherwise allocates fresh IDs.
    [[nodiscard]] static std::vector<AstBlock> reconcile(
        const std::vector<AstBlock> &previous,
        const QString &markdown);

    [[nodiscard]] static QString kindName(BlockKind kind);
    [[nodiscard]] static BlockKind kindFromName(const QString &name);

    // Rebuild a single block's Markdown source from edited display text.
    [[nodiscard]] static QString serializeBlock(const AstBlock &block);

    // Replace one block's source slice inside the full document text.
    [[nodiscard]] static QString replaceBlockSource(
        const QString &document,
        const AstBlock &block,
        const QString &newSource);

    [[nodiscard]] static QVariantList toVariantList(const std::vector<AstBlock> &blocks);
    [[nodiscard]] static QVariantMap toVariantMap(const AstBlock &block);

    // Outline entries: title, level, position (UTF-16 offset).
    [[nodiscard]] static QVariantList outline(const std::vector<AstBlock> &blocks);
};

} // namespace marknote::markdown
