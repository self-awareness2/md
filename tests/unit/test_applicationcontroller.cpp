#include "../../src/app/applicationcontroller.h"

#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QTextStream>
#include <QFileInfo>

class ApplicationControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void startsWithoutAnOpenFile();
    void opensUtf8Markdown();
    void opensUtf16Markdown();
    void resolvesPreviewImagePath();
    void createsAndClosesDocument();
    void buildsAstBlocksAndOutline();
};

void ApplicationControllerTest::startsWithoutAnOpenFile()
{
    ApplicationController controller;

    QCOMPARE(controller.currentFile(), QString());
    QVERIFY(!controller.version().isEmpty());
}

void ApplicationControllerTest::opensUtf8Markdown()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("# Hello\n\nMarkdown content\n");
    file.close();

    ApplicationController controller;
    QVERIFY(controller.openPath(file.fileName()));
    QVERIFY(controller.hasDocument());
    QCOMPARE(controller.documentText(), QStringLiteral("# Hello\n\nMarkdown content\n"));
    QVERIFY(controller.recentFiles().contains(QFileInfo(file.fileName()).absoluteFilePath()));
}

void ApplicationControllerTest::opensUtf16Markdown()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray utf16 = QByteArray::fromHex("FFFE23002000480069000A00");
    file.write(utf16);
    file.close();

    ApplicationController controller;
    QVERIFY(controller.openPath(file.fileName()));
    QCOMPARE(controller.documentText(), QStringLiteral("# Hi\n"));
}

void ApplicationControllerTest::resolvesPreviewImagePath()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("![preview](assets/image.png)\n");
    file.close();

    ApplicationController controller;
    QVERIFY(controller.openPath(file.fileName()));
    QVERIFY(controller.documentPreviewHtml().contains(QStringLiteral("file:///")));
}

void ApplicationControllerTest::createsAndClosesDocument()
{
    ApplicationController controller;
    controller.newDocument();
    QVERIFY(controller.hasDocument());
    controller.setDocumentText(QStringLiteral("draft"));
    QVERIFY(controller.modified());
    controller.closeDocument();
    QVERIFY(!controller.hasDocument());
    QVERIFY(controller.documentText().isEmpty());
}

void ApplicationControllerTest::buildsAstBlocksAndOutline()
{
    ApplicationController controller;
    controller.newDocument();
    controller.setDocumentText(QStringLiteral("# Title\n\n- [ ] task\n\nParagraph\n"));
    QVERIFY(controller.documentBlocks().size() >= 3);
    QVERIFY(controller.documentOutline().size() >= 1);
    QCOMPARE(controller.documentOutline().at(0).toMap().value(QStringLiteral("title")).toString(),
             QStringLiteral("Title"));
    const QVariantMap first = controller.documentBlocks().at(0).toMap();
    QVERIFY(controller.updateBlockDisplay(first.value(QStringLiteral("id")).toULongLong(),
                                          QStringLiteral("Renamed")));
    QVERIFY(controller.documentText().contains(QStringLiteral("# Renamed")));
}

QTEST_MAIN(ApplicationControllerTest)
#include "test_applicationcontroller.moc"
