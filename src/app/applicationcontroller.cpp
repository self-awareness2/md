#include "applicationcontroller.h"
#include "markdown/markdownrenderer.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QSaveFile>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QRegularExpression>
#include <QTranslator>
#include <QQmlEngine>
#include <QVariantMap>
#include <QtGui/QPdfWriter>
#include <QtGui/QTextDocument>
#include <QtGui/QPageSize>

#include <algorithm>

namespace {

QString stripFrontMatterForPreview(const QString &markdown)
{
    if (!markdown.startsWith(QLatin1String("---\n"))
        && !markdown.startsWith(QLatin1String("---\r\n"))) {
        return markdown;
    }
    const qsizetype firstBreak = markdown.indexOf(QLatin1Char('\n'));
    if (firstBreak < 0) {
        return markdown;
    }
    const qsizetype close = markdown.indexOf(QStringLiteral("\n---"), firstBreak + 1);
    if (close < 0) {
        return markdown;
    }
    qsizetype bodyStart = close + 4;
    if (bodyStart < markdown.size() && markdown.at(bodyStart) == QLatin1Char('\r')) {
        ++bodyStart;
    }
    if (bodyStart < markdown.size() && markdown.at(bodyStart) == QLatin1Char('\n')) {
        ++bodyStart;
    }
    return markdown.mid(bodyStart);
}

QString decodeMarkdown(const QByteArray &bytes)
{
    if (bytes.startsWith("\xFF\xFE")) {
        const QByteArray payload = bytes.mid(2);
        QString result;
        result.reserve(payload.size() / 2);
        for (qsizetype index = 0; index + 1 < payload.size(); index += 2) {
            const auto low = static_cast<uchar>(payload.at(index));
            const auto high = static_cast<uchar>(payload.at(index + 1));
            result.append(QChar(static_cast<ushort>(low | (high << 8))));
        }
        return result;
    }
    if (bytes.startsWith("\xFE\xFF")) {
        const QByteArray payload = bytes.mid(2);
        QString result;
        result.reserve(payload.size() / 2);
        for (qsizetype index = 0; index + 1 < payload.size(); index += 2) {
            const auto high = static_cast<uchar>(payload.at(index));
            const auto low = static_cast<uchar>(payload.at(index + 1));
            result.append(QChar(static_cast<ushort>(low | (high << 8))));
        }
        return result;
    }

    QString result = QString::fromUtf8(bytes);
    if (!result.isEmpty() && result.front() == QChar::ByteOrderMark) {
        result.remove(0, 1);
    }
    return result;
}

} // namespace

#ifndef MARKNOTE_VERSION
#define MARKNOTE_VERSION "0.0.0-dev"
#endif

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent)
{
    m_workspace = new WorkspaceService(this);
    m_translator = new QTranslator(this);
    connect(m_workspace, &WorkspaceService::notificationRequested,
            this, &ApplicationController::notificationRequested);
    connect(m_workspace, &WorkspaceService::markdownFilesChanged,
            this, &ApplicationController::workspaceNavigationChanged);
    connect(m_workspace, &WorkspaceService::rootPathChanged,
            this, &ApplicationController::workspaceNavigationChanged);
    connect(this, &ApplicationController::currentFileChanged,
            this, &ApplicationController::workspaceNavigationChanged);
    m_recoveryTimer = new QTimer(this);
    m_recoveryTimer->setSingleShot(true);
    m_recoveryTimer->setInterval(1500);
    connect(m_recoveryTimer, &QTimer::timeout, this, &ApplicationController::writeRecovery);
    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &ApplicationController::handleFileChanged);
    m_recoveryAvailable = QFileInfo::exists(recoveryPath());
    QSettings settings;
    m_recentFiles = settings.value(QStringLiteral("recentFiles")).toStringList();
    m_recentFiles.removeAll(QString());
    for (auto it = m_recentFiles.begin(); it != m_recentFiles.end();) {
        if (!QFileInfo::exists(*it)) {
            it = m_recentFiles.erase(it);
        } else {
            ++it;
        }
    }
    loadUiSettings();
}

void ApplicationController::setQmlEngine(QQmlEngine *engine)
{
    m_qmlEngine = engine;
    applyLanguage();
}

void ApplicationController::loadUiSettings()
{
    QSettings settings;
    m_language = settings.value(QStringLiteral("ui/language"), QStringLiteral("en")).toString();
    m_paperTheme = settings.value(QStringLiteral("ui/paperTheme"), QStringLiteral("default")).toString();
    m_darkMode = settings.value(QStringLiteral("ui/darkMode"), false).toBool();
    if (m_language != QStringLiteral("en") && m_language != QStringLiteral("zh_CN")) {
        m_language = QStringLiteral("en");
    }
    const QStringList papers{
        QStringLiteral("default"),
        QStringLiteral("cream"),
        QStringLiteral("sepia"),
        QStringLiteral("mint"),
        QStringLiteral("night"),
    };
    if (!papers.contains(m_paperTheme)) {
        m_paperTheme = QStringLiteral("default");
    }
}

void ApplicationController::saveUiSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("ui/language"), m_language);
    settings.setValue(QStringLiteral("ui/paperTheme"), m_paperTheme);
    settings.setValue(QStringLiteral("ui/darkMode"), m_darkMode);
}

void ApplicationController::applyLanguage()
{
    QCoreApplication::removeTranslator(m_translator);
    bool loaded = false;
    if (m_language == QStringLiteral("zh_CN")) {
        loaded = m_translator->load(QStringLiteral(":/i18n/marknote_zh_CN.qm"));
        if (!loaded) {
            loaded = m_translator->load(QStringLiteral("marknote_zh_CN"),
                                        QStringLiteral(":/i18n"));
        }
        if (loaded) {
            QCoreApplication::installTranslator(m_translator);
        }
    }
    if (m_qmlEngine != nullptr) {
        m_qmlEngine->retranslate();
    }
}

QString ApplicationController::language() const
{
    return m_language;
}

QString ApplicationController::paperTheme() const
{
    return m_paperTheme;
}

bool ApplicationController::darkMode() const
{
    return m_darkMode;
}

void ApplicationController::setLanguage(const QString &language)
{
    QString normalized = language;
    if (normalized == QStringLiteral("zh") || normalized == QStringLiteral("zh-CN")) {
        normalized = QStringLiteral("zh_CN");
    }
    if (normalized != QStringLiteral("en") && normalized != QStringLiteral("zh_CN")) {
        normalized = QStringLiteral("en");
    }
    if (m_language == normalized) {
        return;
    }
    m_language = normalized;
    saveUiSettings();
    applyLanguage();
    emit languageChanged();
    emit notificationRequested(
        m_language == QStringLiteral("zh_CN")
            ? QStringLiteral("界面语言已切换为中文")
            : QStringLiteral("Interface language switched to English"));
}

void ApplicationController::setPaperTheme(const QString &paperTheme)
{
    const QStringList papers{
        QStringLiteral("default"),
        QStringLiteral("cream"),
        QStringLiteral("sepia"),
        QStringLiteral("mint"),
        QStringLiteral("night"),
    };
    const QString normalized = papers.contains(paperTheme) ? paperTheme : QStringLiteral("default");
    if (m_paperTheme == normalized) {
        return;
    }
    m_paperTheme = normalized;
    saveUiSettings();
    emit paperThemeChanged();
}

void ApplicationController::setDarkMode(bool darkMode)
{
    if (m_darkMode == darkMode) {
        return;
    }
    m_darkMode = darkMode;
    saveUiSettings();
    emit darkModeChanged();
}

QVariantList ApplicationController::availableLanguages() const
{
    QVariantList list;
    list.push_back(QVariantMap{
        {QStringLiteral("id"), QStringLiteral("en")},
        {QStringLiteral("title"), QStringLiteral("English")},
    });
    list.push_back(QVariantMap{
        {QStringLiteral("id"), QStringLiteral("zh_CN")},
        {QStringLiteral("title"), QStringLiteral("简体中文")},
    });
    return list;
}

QVariantList ApplicationController::availablePaperThemes() const
{
    QVariantList list;
    list.push_back(QVariantMap{
        {QStringLiteral("id"), QStringLiteral("default")},
        {QStringLiteral("title"), tr("Default")},
        {QStringLiteral("color"), QStringLiteral("#ffffff")},
    });
    list.push_back(QVariantMap{
        {QStringLiteral("id"), QStringLiteral("cream")},
        {QStringLiteral("title"), tr("Cream")},
        {QStringLiteral("color"), QStringLiteral("#f4efe4")},
    });
    list.push_back(QVariantMap{
        {QStringLiteral("id"), QStringLiteral("sepia")},
        {QStringLiteral("title"), tr("Sepia")},
        {QStringLiteral("color"), QStringLiteral("#efe2c8")},
    });
    list.push_back(QVariantMap{
        {QStringLiteral("id"), QStringLiteral("mint")},
        {QStringLiteral("title"), tr("Mint")},
        {QStringLiteral("color"), QStringLiteral("#e7efea")},
    });
    list.push_back(QVariantMap{
        {QStringLiteral("id"), QStringLiteral("night")},
        {QStringLiteral("title"), tr("Night")},
        {QStringLiteral("color"), QStringLiteral("#1b1e24")},
    });
    return list;
}

QString ApplicationController::version() const
{
    return QStringLiteral(MARKNOTE_VERSION);
}

QString ApplicationController::currentFile() const
{
    return m_currentFile;
}

QString ApplicationController::currentFileName() const
{
    return m_currentFile.isEmpty() ? QStringLiteral("Untitled") : QFileInfo(m_currentFile).fileName();
}

QString ApplicationController::documentText() const
{
    return m_documentText;
}

QString ApplicationController::documentHtml() const
{
    return m_documentHtml;
}

QString ApplicationController::documentPreviewHtml() const
{
    return m_documentPreviewHtml;
}

void ApplicationController::updatePreviewHtml()
{
    static const QString style = QStringLiteral(
        "<style>"
        "body{color:inherit;line-height:1.65;}"
        "h1,h2,h3,h4,h5,h6{line-height:1.3;margin:1.1em 0 0.45em;font-weight:600;}"
        "p,ul,ol,blockquote,pre,table{margin:0.75em 0;}"
        "a{color:#0a84ff;text-decoration:none;}"
        "code{font-family:Consolas,'Courier New',monospace;background:rgba(127,127,127,0.12);padding:0.1em 0.35em;border-radius:4px;}"
        "pre{background:rgba(127,127,127,0.12);padding:12px 14px;border-radius:8px;overflow:auto;}"
        "pre code{background:transparent;padding:0;}"
        "blockquote{border-left:3px solid #0a84ff;padding:0.2em 0 0.2em 1em;color:#6e6e73;}"
        "table{border-collapse:collapse;width:100%;}"
        "th,td{border:1px solid rgba(127,127,127,0.35);padding:6px 10px;}"
        "th{background:rgba(127,127,127,0.08);}"
        "hr{border:none;border-top:1px solid rgba(127,127,127,0.35);margin:1.4em 0;}"
        "img{max-width:100%;}"
        "ul.contains-task-list{list-style:none;padding-left:1.2em;}"
        "</style>");

    m_documentPreviewHtml = style + m_documentHtml;
    if (m_currentFile.isEmpty() || m_documentHtml.isEmpty()) {
        return;
    }

    const QUrl baseUrl = QUrl::fromLocalFile(QFileInfo(m_currentFile).absolutePath() + QStringLiteral("/"));
    const QRegularExpression imageSource(
        QStringLiteral(R"((<img\b[^>]*\bsrc=")([^"]+)("))"),
        QRegularExpression::CaseInsensitiveOption);
    int searchPosition = 0;
    while (true) {
        const QRegularExpressionMatch match = imageSource.match(m_documentPreviewHtml, searchPosition);
        if (!match.hasMatch()) {
            break;
        }
        const QString source = match.captured(2);
        const QUrl sourceUrl(source);
        if (sourceUrl.scheme().isEmpty() && !source.startsWith(QLatin1Char('#'))) {
            const QString resolved = baseUrl.resolved(sourceUrl).toString();
            m_documentPreviewHtml.replace(match.capturedStart(2), source.size(), resolved);
            searchPosition = match.capturedStart(2) + resolved.size();
        } else {
            searchPosition = match.capturedEnd(2);
        }
    }
}

bool ApplicationController::hasDocument() const
{
    return m_hasDocument;
}

bool ApplicationController::modified() const
{
    return m_modified;
}

bool ApplicationController::recoveryAvailable() const
{
    return m_recoveryAvailable;
}

bool ApplicationController::externalChangeDetected() const
{
    return m_externalChangeDetected;
}

QStringList ApplicationController::recentFiles() const
{
    return m_recentFiles;
}

QVariantList ApplicationController::documentBlocks() const
{
    return marknote::markdown::DocumentAst::toVariantList(m_blocks);
}

QVariantList ApplicationController::documentOutline() const
{
    return marknote::markdown::DocumentAst::outline(m_blocks);
}

WorkspaceService *ApplicationController::workspace() const
{
    return m_workspace;
}

int ApplicationController::workspaceIndexOfCurrent() const
{
    if (m_workspace == nullptr || !m_workspace->hasWorkspace() || m_currentFile.isEmpty()) {
        return -1;
    }

    const QString current = QFileInfo(m_currentFile).absoluteFilePath();
    const QStringList files = m_workspace->markdownFiles();
    for (int i = 0; i < files.size(); ++i) {
        if (QFileInfo(files.at(i)).absoluteFilePath().compare(current, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }
    return -1;
}

int ApplicationController::workspaceFileCount() const
{
    if (m_workspace == nullptr || !m_workspace->hasWorkspace()) {
        return 0;
    }
    return m_workspace->markdownFiles().size();
}

int ApplicationController::workspaceFileIndex() const
{
    const int index = workspaceIndexOfCurrent();
    return index < 0 ? 0 : index + 1;
}

bool ApplicationController::canOpenPreviousWorkspaceFile() const
{
    return workspaceIndexOfCurrent() > 0;
}

bool ApplicationController::canOpenNextWorkspaceFile() const
{
    const int index = workspaceIndexOfCurrent();
    return index >= 0 && index + 1 < workspaceFileCount();
}

bool ApplicationController::openWorkspaceFileByDelta(int delta)
{
    const int index = workspaceIndexOfCurrent();
    if (index < 0 || m_workspace == nullptr) {
        return false;
    }

    const QStringList files = m_workspace->markdownFiles();
    const int target = index + delta;
    if (target < 0 || target >= files.size()) {
        return false;
    }

    if (m_modified && !m_currentFile.isEmpty() && !save()) {
        emit notificationRequested(tr("Save the current document before switching files"));
        return false;
    }

    return openPath(files.at(target));
}

bool ApplicationController::openPreviousWorkspaceFile()
{
    return openWorkspaceFileByDelta(-1);
}

bool ApplicationController::openNextWorkspaceFile()
{
    return openWorkspaceFileByDelta(1);
}

void ApplicationController::rebuildAst()
{
    const auto previous = m_blocks;
    m_blocks = marknote::markdown::DocumentAst::reconcile(previous, m_documentText);
    if (m_blocks.empty() && m_hasDocument) {
        marknote::markdown::AstBlock empty;
        empty.id = previous.empty() ? 1 : previous.front().id;
        empty.kind = marknote::markdown::BlockKind::Paragraph;
        m_blocks.push_back(std::move(empty));
    }
    emit documentBlocksChanged();
}

void ApplicationController::applyDocumentText(const QString &text, bool markModified)
{
    m_documentText = text;
    m_documentHtml = marknote::markdown::MarkdownRenderer::toHtml(
        stripFrontMatterForPreview(m_documentText));
    updatePreviewHtml();
    if (!m_suppressBlockRebuild) {
        rebuildAst();
    }
    if (markModified) {
        const bool wasModified = m_modified;
        m_modified = true;
        if (m_recoveryTimer) {
            m_recoveryTimer->start();
        }
        emit documentTextChanged();
        if (!wasModified) {
            emit modifiedChanged();
        }
        return;
    }
    emit documentTextChanged();
}

marknote::markdown::AstBlock *ApplicationController::findBlock(qulonglong blockId)
{
    for (auto &block : m_blocks) {
        if (block.id == static_cast<std::uint64_t>(blockId)) {
            return &block;
        }
    }
    return nullptr;
}

void ApplicationController::replaceDocumentFromBlocks()
{
    m_documentText = serializeBlocksToDocument();
    m_documentHtml = marknote::markdown::MarkdownRenderer::toHtml(
        stripFrontMatterForPreview(m_documentText));
    updatePreviewHtml();
    const bool wasModified = m_modified;
    m_modified = true;
    if (m_recoveryTimer) {
        m_recoveryTimer->start();
    }
    m_suppressBlockRebuild = true;
    emit documentTextChanged();
    m_suppressBlockRebuild = false;
    rebuildAst();
    if (!wasModified) {
        emit modifiedChanged();
    }
}

QString ApplicationController::serializeBlocksToDocument() const
{
    QStringList pieces;
    pieces.reserve(static_cast<int>(m_blocks.size()));
    for (const auto &block : m_blocks) {
        pieces.push_back(marknote::markdown::DocumentAst::serializeBlock(block));
    }
    return pieces.join(QStringLiteral("\n\n"));
}

void ApplicationController::addRecentFile(const QString &path)
{
    if (path.isEmpty()) {
        return;
    }
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > 12) {
        m_recentFiles.removeLast();
    }
    QSettings settings;
    settings.setValue(QStringLiteral("recentFiles"), m_recentFiles);
    emit recentFilesChanged();
}

void ApplicationController::clearRecentFiles()
{
    if (m_recentFiles.isEmpty()) {
        return;
    }
    m_recentFiles.clear();
    QSettings settings;
    settings.remove(QStringLiteral("recentFiles"));
    emit recentFilesChanged();
}

void ApplicationController::watchCurrentFile()
{
    if (!m_fileWatcher) {
        return;
    }
    m_fileWatcher->removePaths(m_fileWatcher->files());
    m_lastDiskModified = {};
    m_lastDiskSize = -1;
    if (m_currentFile.isEmpty()) {
        return;
    }
    m_fileWatcher->addPath(m_currentFile);
    const QFileInfo info(m_currentFile);
    m_lastDiskModified = info.lastModified();
    m_lastDiskSize = info.size();
}

void ApplicationController::handleFileChanged(const QString &path)
{
    if (path != m_currentFile || !QFileInfo::exists(path)) {
        return;
    }

    const QFileInfo info(path);
    if (info.lastModified() == m_lastDiskModified && info.size() == m_lastDiskSize) {
        watchCurrentFile();
        return;
    }
    watchCurrentFile();
    if (!m_externalChangeDetected) {
        m_externalChangeDetected = true;
        emit externalChangeDetectedChanged();
        emit notificationRequested(tr("The file changed outside Marknote"));
    }
}

QString ApplicationController::recoveryPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/recovery.md");
}

void ApplicationController::writeRecovery()
{
    if (!m_modified || !m_hasDocument || m_documentText.isEmpty()) {
        return;
    }

    const QString path = recoveryPath();
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        return;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    if (file.write(m_documentText.toUtf8()) >= 0 && file.commit()) {
        if (!m_recoveryAvailable) {
            m_recoveryAvailable = true;
            emit recoveryAvailableChanged();
        }
    }
}

void ApplicationController::clearRecovery()
{
    if (m_recoveryTimer) {
        m_recoveryTimer->stop();
    }
    const bool wasAvailable = m_recoveryAvailable;
    QFile::remove(recoveryPath());
    m_recoveryAvailable = false;
    if (wasAvailable) {
        emit recoveryAvailableChanged();
    }
}

void ApplicationController::openFile()
{
    const QString path = QFileDialog::getOpenFileName(
        nullptr,
        tr("Open Markdown file"),
        QString(),
        tr("Markdown files (*.md *.markdown);;All files (*)"));

    if (path.isEmpty()) {
        return;
    }

    openPath(path);
}

bool ApplicationController::openPath(const QString &path)
{
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit notificationRequested(
            tr("Cannot open %1: %2").arg(QFileInfo(absolutePath).fileName(), file.errorString()));
        return false;
    }

    const QString text = decodeMarkdown(file.readAll());

    clearRecovery();

    m_currentFile = absolutePath;
    addRecentFile(absolutePath);
    m_hasDocument = true;
    watchCurrentFile();
    if (m_externalChangeDetected) {
        m_externalChangeDetected = false;
        emit externalChangeDetectedChanged();
    }
    const bool wasModified = m_modified;
    m_modified = false;
    m_documentText.clear();
    applyDocumentText(text, false);
    if (wasModified) {
        emit modifiedChanged();
    }
    emit currentFileChanged();
    emit notificationRequested(tr("Opened %1").arg(currentFileName()));
    return true;
}

void ApplicationController::newDocument()
{
    clearRecovery();
    const bool wasModified = m_modified;
    m_currentFile.clear();
    m_hasDocument = true;
    watchCurrentFile();
    m_modified = false;
    applyDocumentText(QString(), false);
    if (wasModified) {
        emit modifiedChanged();
    }
    emit currentFileChanged();
}

void ApplicationController::closeDocument()
{
    if (!m_hasDocument && m_currentFile.isEmpty() && m_documentText.isEmpty()) {
        return;
    }
    const bool wasModified = m_modified;
    clearRecovery();
    m_currentFile.clear();
    m_documentText.clear();
    m_documentHtml.clear();
    m_documentPreviewHtml.clear();
    m_blocks.clear();
    m_hasDocument = false;
    m_modified = false;
    watchCurrentFile();
    emit currentFileChanged();
    emit documentTextChanged();
    emit documentBlocksChanged();
    if (wasModified) {
        emit modifiedChanged();
    }
}

void ApplicationController::setDocumentText(const QString &text)
{
    if (m_documentText == text) {
        return;
    }
    applyDocumentText(text, true);
}

bool ApplicationController::save()
{
    if (m_currentFile.isEmpty()) {
        saveAs();
        return !m_currentFile.isEmpty() && !m_modified;
    }
    return savePath(m_currentFile);
}

void ApplicationController::saveAs()
{
    const QString path = QFileDialog::getSaveFileName(
        nullptr,
        tr("Save Markdown file"),
        m_currentFile.isEmpty() ? QStringLiteral("Untitled.md") : m_currentFile,
        tr("Markdown files (*.md *.markdown);;All files (*)"));
    if (!path.isEmpty()) {
        savePath(path);
    }
}

bool ApplicationController::savePath(const QString &path)
{
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    QSaveFile file(absolutePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit notificationRequested(
            tr("Cannot save %1: %2").arg(QFileInfo(absolutePath).fileName(), file.errorString()));
        return false;
    }

    if (file.write(m_documentText.toUtf8()) < 0 || !file.commit()) {
        emit notificationRequested(
            tr("Cannot save %1: %2").arg(QFileInfo(absolutePath).fileName(), file.errorString()));
        return false;
    }

    const bool pathChanged = m_currentFile != absolutePath;
    const bool wasModified = m_modified;
    const bool wasDocument = m_hasDocument;
    m_currentFile = absolutePath;
    addRecentFile(absolutePath);
    m_hasDocument = true;
    m_modified = false;
    updatePreviewHtml();
    watchCurrentFile();
    if (m_externalChangeDetected) {
        m_externalChangeDetected = false;
        emit externalChangeDetectedChanged();
    }
    clearRecovery();
    if (pathChanged || !wasDocument) {
        emit currentFileChanged();
    }
    if (wasModified) {
        emit modifiedChanged();
    }
    emit notificationRequested(tr("Saved %1").arg(currentFileName()));
    return true;
}

bool ApplicationController::exportHtml(const QString &path)
{
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    QSaveFile file(absolutePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit notificationRequested(
            tr("Cannot export %1: %2").arg(QFileInfo(absolutePath).fileName(), file.errorString()));
        return false;
    }

    const QByteArray html = m_documentHtml.toUtf8();
    if (file.write(html) < 0 || !file.commit()) {
        emit notificationRequested(
            tr("Cannot export %1: %2").arg(QFileInfo(absolutePath).fileName(), file.errorString()));
        return false;
    }
    emit notificationRequested(tr("Exported %1").arg(QFileInfo(absolutePath).fileName()));
    return true;
}

void ApplicationController::exportHtmlAs()
{
    const QString path = QFileDialog::getSaveFileName(
        nullptr,
        tr("Export HTML"),
        m_currentFile.isEmpty() ? QStringLiteral("Untitled.html")
                                : QFileInfo(m_currentFile).completeBaseName() + QStringLiteral(".html"),
        tr("HTML files (*.html);;All files (*)"));
    if (!path.isEmpty()) {
        exportHtml(path);
    }
}

bool ApplicationController::exportPdf(const QString &path)
{
    const QString absolutePath = QFileInfo(path).absoluteFilePath();
    QPdfWriter writer(absolutePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(18, 18, 18, 18));

    QTextDocument document;
    document.setDocumentMargin(0);
    document.setHtml(QStringLiteral("<style>body{font-family:'Segoe UI';font-size:11pt;}"
                                    "pre{background:#f3f3f3;padding:8px;}"
                                    "blockquote{border-left:3px solid #999;padding-left:10px;}"
                                    "table{border-collapse:collapse;}"
                                    "td,th{border:1px solid #aaa;padding:4px;}</style>")
                     + m_documentHtml);
    document.setPageSize(QSizeF(writer.width(), writer.height()));
    document.print(&writer);
    emit notificationRequested(tr("Exported %1").arg(QFileInfo(absolutePath).fileName()));
    return true;
}

void ApplicationController::exportPdfAs()
{
    const QString path = QFileDialog::getSaveFileName(
        nullptr,
        tr("Export PDF"),
        m_currentFile.isEmpty() ? QStringLiteral("Untitled.pdf")
                                : QFileInfo(m_currentFile).completeBaseName() + QStringLiteral(".pdf"),
        tr("PDF files (*.pdf);;All files (*)"));
    if (!path.isEmpty()) {
        exportPdf(path);
    }
}

QString ApplicationController::prepareImage(const QString &path)
{
    const QString sourcePath = QFileInfo(path).absoluteFilePath();
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        emit notificationRequested(tr("Image file does not exist"));
        return {};
    }

    if (m_currentFile.isEmpty()) {
        emit notificationRequested(tr("Save the document before inserting an image"));
        return {};
    }

    const QFileInfo documentInfo(m_currentFile);
    QDir assetsDir(documentInfo.absolutePath() + QStringLiteral("/assets"));
    if (!assetsDir.exists() && !QDir().mkpath(assetsDir.absolutePath())) {
        emit notificationRequested(tr("Cannot create the assets directory"));
        return {};
    }

    QString fileName = sourceInfo.fileName();
    QString destination = assetsDir.absoluteFilePath(fileName);
    if (QFileInfo::exists(destination)) {
        const QString stem = sourceInfo.completeBaseName();
        const QString suffix = sourceInfo.completeSuffix().isEmpty()
                                   ? QString()
                                   : QStringLiteral(".") + sourceInfo.completeSuffix();
        int index = 2;
        do {
            fileName = QStringLiteral("%1-%2%3").arg(stem).arg(index++).arg(suffix);
            destination = assetsDir.absoluteFilePath(fileName);
        } while (QFileInfo::exists(destination));
    }

    if (QFileInfo(sourcePath).absoluteFilePath() != QFileInfo(destination).absoluteFilePath()
        && !QFile::copy(sourcePath, destination)) {
        emit notificationRequested(tr("Cannot copy image into the assets directory"));
        return {};
    }

    return QStringLiteral("assets/") + fileName;
}

void ApplicationController::insertImage()
{
    const QString path = QFileDialog::getOpenFileName(
        nullptr,
        tr("Insert image"),
        QString(),
        tr("Images (*.png *.jpg *.jpeg *.gif *.webp *.svg);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }

    const QString relativePath = prepareImage(path);
    if (!relativePath.isEmpty()) {
        emit imagePrepared(relativePath);
    }
}

bool ApplicationController::recoverDocument()
{
    QFile file(recoveryPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QString text = decodeMarkdown(file.readAll());
    if (text.isEmpty()) {
        discardRecovery();
        return false;
    }

    m_currentFile.clear();
    m_hasDocument = true;
    clearRecovery();
    applyDocumentText(text, true);
    emit currentFileChanged();
    emit notificationRequested(tr("Recovered the last unsaved document"));
    return true;
}

void ApplicationController::discardRecovery()
{
    clearRecovery();
}

bool ApplicationController::reloadFromDisk()
{
    if (m_currentFile.isEmpty()) {
        return false;
    }
    return openPath(m_currentFile);
}

bool ApplicationController::openLink(const QString &link)
{
    const QUrl rawUrl(link);
    QUrl resolved;
    if (rawUrl.scheme().isEmpty() && !m_currentFile.isEmpty()) {
        const QString localPath = QFileInfo(m_currentFile).absoluteDir().absoluteFilePath(link);
        resolved = QUrl::fromLocalFile(QFileInfo(localPath).absoluteFilePath());
    } else {
        resolved = QUrl::fromUserInput(link);
    }
    if (!resolved.isValid()) {
        return false;
    }

    const QString scheme = resolved.scheme().toLower();
    if (scheme == QStringLiteral("javascript") || scheme == QStringLiteral("data")) {
        return false;
    }
    return QDesktopServices::openUrl(resolved);
}

bool ApplicationController::updateBlockDisplay(qulonglong blockId, const QString &displayText)
{
    auto *block = findBlock(blockId);
    if (!block || block->displayText == displayText) {
        return false;
    }
    block->displayText = displayText;
    block->source = marknote::markdown::DocumentAst::serializeBlock(*block);
    replaceDocumentFromBlocks();
    return true;
}

bool ApplicationController::setBlockKind(qulonglong blockId, const QString &kindName, int level)
{
    auto *block = findBlock(blockId);
    if (!block) {
        return false;
    }
    const auto kind = marknote::markdown::DocumentAst::kindFromName(kindName);
    block->kind = kind;
    if (kind == marknote::markdown::BlockKind::Heading) {
        block->level = qBound(1, level <= 0 ? 1 : level, 6);
    }
    if (kind == marknote::markdown::BlockKind::ListItem && kindName == QLatin1String("task")) {
        block->task = true;
    }
    if (kindName == QLatin1String("ordered")) {
        block->kind = marknote::markdown::BlockKind::ListItem;
        block->ordered = true;
        block->task = false;
    }
    if (kindName == QLatin1String("task")) {
        block->kind = marknote::markdown::BlockKind::ListItem;
        block->task = true;
        block->ordered = false;
    }
    if (kindName == QLatin1String("list")) {
        block->ordered = false;
        block->task = false;
    }
    block->source = marknote::markdown::DocumentAst::serializeBlock(*block);
    replaceDocumentFromBlocks();
    return true;
}

bool ApplicationController::toggleTaskChecked(qulonglong blockId)
{
    auto *block = findBlock(blockId);
    if (!block || !block->task) {
        return false;
    }
    block->taskChecked = !block->taskChecked;
    block->source = marknote::markdown::DocumentAst::serializeBlock(*block);
    replaceDocumentFromBlocks();
    return true;
}

bool ApplicationController::insertBlockAfter(qulonglong blockId, const QString &kindName)
{
    marknote::markdown::AstBlock created;
    created.id = 0;
    for (const auto &block : m_blocks) {
        created.id = std::max(created.id, block.id);
    }
    ++created.id;
    if (kindName == QLatin1String("task") || kindName == QLatin1String("ordered")) {
        created.kind = marknote::markdown::BlockKind::ListItem;
        created.task = kindName == QLatin1String("task");
        created.ordered = kindName == QLatin1String("ordered");
        created.displayText = QStringLiteral("List item");
    } else {
        created.kind = marknote::markdown::DocumentAst::kindFromName(kindName);
        if (created.kind == marknote::markdown::BlockKind::Heading) {
            created.level = 2;
            created.displayText = QStringLiteral("Heading");
        } else if (created.kind == marknote::markdown::BlockKind::CodeBlock) {
            created.displayText = QString();
        } else if (created.kind == marknote::markdown::BlockKind::ThematicBreak) {
            created.displayText = QStringLiteral("---");
        } else if (created.kind == marknote::markdown::BlockKind::ListItem) {
            created.displayText = QStringLiteral("List item");
        } else if (created.kind == marknote::markdown::BlockKind::Table) {
            created.displayText = QStringLiteral("| Column 1 | Column 2 |\n| --- | --- |\n|  |  |");
            created.source = created.displayText;
        } else {
            created.displayText = QString();
        }
    }
    if (created.source.isEmpty()) {
        created.source = marknote::markdown::DocumentAst::serializeBlock(created);
    }

    if (m_blocks.empty()) {
        m_blocks.push_back(created);
    } else {
        bool inserted = false;
        for (size_t i = 0; i < m_blocks.size(); ++i) {
            if (m_blocks[i].id == static_cast<std::uint64_t>(blockId)) {
                m_blocks.insert(m_blocks.begin() + static_cast<std::ptrdiff_t>(i + 1), created);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            m_blocks.push_back(created);
        }
    }
    replaceDocumentFromBlocks();
    return true;
}

bool ApplicationController::deleteBlock(qulonglong blockId)
{
    const auto it = std::find_if(m_blocks.begin(), m_blocks.end(), [blockId](const auto &block) {
        return block.id == static_cast<std::uint64_t>(blockId);
    });
    if (it == m_blocks.end()) {
        return false;
    }
    m_blocks.erase(it);
    if (m_blocks.empty()) {
        marknote::markdown::AstBlock empty;
        empty.id = 1;
        empty.kind = marknote::markdown::BlockKind::Paragraph;
        m_blocks.push_back(empty);
    }
    replaceDocumentFromBlocks();
    return true;
}
