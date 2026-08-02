#include "markdown/markdownrenderer.h"

#include <QtTest/QtTest>

class MarkdownRendererTest final : public QObject
{
    Q_OBJECT

private slots:
    void rendersCommonMark();
    void rendersGfmExtensions();
    void rendersTaskListAndAutolink();
    void blocksRawHtml();
};

void MarkdownRendererTest::rendersCommonMark()
{
    const QString html = marknote::markdown::MarkdownRenderer::toHtml(
        QStringLiteral("# Heading\n\n**strong** text"));

    QVERIFY(html.contains(QStringLiteral("<h1>Heading</h1>")));
    QVERIFY(html.contains(QStringLiteral("<strong>strong</strong>")));
}

void MarkdownRendererTest::rendersGfmExtensions()
{
    const QString html = marknote::markdown::MarkdownRenderer::toHtml(
        QStringLiteral("~~removed~~\n\n| A | B |\n| - | - |\n| 1 | 2 |"));

    QVERIFY(html.contains(QStringLiteral("<del>removed</del>")));
    QVERIFY(html.contains(QStringLiteral("<table>")));
}

void MarkdownRendererTest::rendersTaskListAndAutolink()
{
    const QString html = marknote::markdown::MarkdownRenderer::toHtml(
        QStringLiteral("- [x] done\n- [ ] todo\n\nhttps://example.com"));

    QVERIFY(html.contains(QStringLiteral("task-list-item")));
    QVERIFY(html.contains(QStringLiteral("href=\"https://example.com\"")));
}

void MarkdownRendererTest::blocksRawHtml()
{
    const QString html = marknote::markdown::MarkdownRenderer::toHtml(
        QStringLiteral("<script>alert('unsafe')</script>"));

    QVERIFY(!html.contains(QStringLiteral("<script>")));
}

QTEST_MAIN(MarkdownRendererTest)
#include "test_markdownrenderer.moc"
