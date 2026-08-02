#include "applicationcontroller.h"
#include "markdown/markdownsyntaxhighlighter.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <qqml.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qmlRegisterType<marknote::markdown::MarkdownSyntaxHighlighter>(
        "Marknote", 1, 0, "MarkdownSyntaxHighlighter");
    app.setApplicationName(QStringLiteral("Marknote"));
    app.setApplicationDisplayName(QStringLiteral("Marknote"));
    app.setOrganizationName(QStringLiteral("Marknote"));

    ApplicationController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);

    const QUrl mainUrl(QStringLiteral("qrc:/qt/qml/Marknote/qml/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);
    engine.load(mainUrl);

    // File associations and shell launches pass the document path as argv[1].
    // Loading it after the QML engine is ready keeps the same notification path
    // as files opened through the toolbar.
    if (app.arguments().size() > 1) {
        controller.openPath(app.arguments().at(1));
    }

    return app.exec();
}
