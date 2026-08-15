#include "markdown/documentast.h"

#include <QtTest/QtTest>

using marknote::markdown::AstBlock;
using marknote::markdown::BlockKind;
using marknote::markdown::DocumentAst;

class DocumentAstTest final : public QObject
{
    Q_OBJECT

private slots:
    void parsesCoreBlocks();
    void parsesListItemsAndTasks();
    void preservesFrontMatter();
    void serializesEditedDisplayText();
    void reconcilesStableIds();
    void buildsOutline();
};

void DocumentAstTest::parsesCoreBlocks()
{
    const auto blocks = DocumentAst::parse(
        QStringLiteral("# Title\n\nHello **world**\n\n```js\nconst x = 1\n```\n\n---\n"));

    QVERIFY(blocks.size() >= 4);
    QCOMPARE(blocks[0].kind, BlockKind::Heading);
    QCOMPARE(blocks[0].displayText, QStringLiteral("Title"));
    QCOMPARE(blocks[0].level, 1);
    QCOMPARE(blocks[1].kind, BlockKind::Paragraph);
    QVERIFY(blocks[1].source.contains(QStringLiteral("Hello")));
    QCOMPARE(blocks[2].kind, BlockKind::CodeBlock);
    QCOMPARE(blocks[2].language, QStringLiteral("js"));
    QCOMPARE(blocks[2].displayText.trimmed(), QStringLiteral("const x = 1"));
    QCOMPARE(blocks[3].kind, BlockKind::ThematicBreak);
}

void DocumentAstTest::parsesListItemsAndTasks()
{
    const auto blocks = DocumentAst::parse(
        QStringLiteral("- alpha\n- [x] done\n- [ ] todo\n\n1. first\n"));

    QVERIFY(blocks.size() >= 4);
    QCOMPARE(blocks[0].kind, BlockKind::ListItem);
    QCOMPARE(blocks[0].displayText, QStringLiteral("alpha"));
    QVERIFY(blocks[1].task);
    QVERIFY(blocks[1].taskChecked);
    QVERIFY(blocks[2].task);
    QVERIFY(!blocks[2].taskChecked);
    QVERIFY(blocks[3].ordered);
    QCOMPARE(blocks[3].displayText, QStringLiteral("first"));
}

void DocumentAstTest::preservesFrontMatter()
{
    const auto blocks = DocumentAst::parse(
        QStringLiteral("---\ntitle: Demo\n---\n\n# Body\n"));

    QVERIFY(blocks.size() >= 2);
    QCOMPARE(blocks[0].kind, BlockKind::FrontMatter);
    QVERIFY(blocks[0].source.contains(QStringLiteral("title: Demo")));
    QCOMPARE(blocks[1].kind, BlockKind::Heading);
    QCOMPARE(blocks[1].displayText, QStringLiteral("Body"));
}

void DocumentAstTest::serializesEditedDisplayText()
{
    AstBlock heading;
    heading.kind = BlockKind::Heading;
    heading.level = 2;
    heading.displayText = QStringLiteral("Edited");
    QCOMPARE(DocumentAst::serializeBlock(heading), QStringLiteral("## Edited"));

    AstBlock task;
    task.kind = BlockKind::ListItem;
    task.task = true;
    task.taskChecked = true;
    task.displayText = QStringLiteral("ship it");
    QCOMPARE(DocumentAst::serializeBlock(task), QStringLiteral("- [x] ship it"));
}

void DocumentAstTest::reconcilesStableIds()
{
    const QString original = QStringLiteral("# A\n\npara\n");
    auto first = DocumentAst::parse(original);
    QVERIFY(first.size() >= 2);
    const auto headingId = first[0].id;
    const auto paraId = first[1].id;

    auto second = DocumentAst::reconcile(first, QStringLiteral("# A\n\npara changed\n"));
    QVERIFY(second.size() >= 2);
    QCOMPARE(second[0].id, headingId);
    QCOMPARE(second[1].id, paraId);
}

void DocumentAstTest::buildsOutline()
{
    const auto blocks = DocumentAst::parse(QStringLiteral("# One\n\n## Two\n\ntext\n"));
    const QVariantList outline = DocumentAst::outline(blocks);
    QCOMPARE(outline.size(), 2);
    QCOMPARE(outline[0].toMap().value(QStringLiteral("title")).toString(), QStringLiteral("One"));
    QCOMPARE(outline[1].toMap().value(QStringLiteral("level")).toInt(), 2);
}

QTEST_MAIN(DocumentAstTest)
#include "test_documentast.moc"
