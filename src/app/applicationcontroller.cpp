#include "applicationcontroller.h"
#include "markdown/markdownrenderer.h"

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
#include <QtGui/QPdfWriter>
#include <QtGui/QTextDocument>
#include <QtGui/QPageSize>

namespace {

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
    m_documentPreviewHtml = m_documentHtml;
    if (m_currentFile.isEmpty() || m_documentPreviewHtml.isEmpty()) {
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
    m_documentText = std::move(text);
    m_documentHtml = marknote::markdown::MarkdownRenderer::toHtml(m_documentText);
    updatePreviewHtml();
    m_hasDocument = true;
    watchCurrentFile();
    if (m_externalChangeDetected) {
        m_externalChangeDetected = false;
        emit externalChangeDetectedChanged();
    }
    const bool wasModified = m_modified;
    m_modified = false;
    emit currentFileChanged();
    emit documentTextChanged();
    if (wasModified) {
        emit modifiedChanged();
    }
    emit notificationRequested(tr("Opened %1").arg(currentFileName()));
    return true;
}

void ApplicationController::newDocument()
{
    clearRecovery();
    const bool wasModified = m_modified;
    m_currentFile.clear();
    m_documentText.clear();
    m_documentHtml.clear();
    m_documentPreviewHtml.clear();
    m_hasDocument = true;
    watchCurrentFile();
    m_modified = false;
    emit currentFileChanged();
    emit documentTextChanged();
    if (wasModified) {
        emit modifiedChanged();
    }
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
    m_hasDocument = false;
    m_modified = false;
    watchCurrentFile();
    emit currentFileChanged();
    emit documentTextChanged();
    if (wasModified) {
        emit modifiedChanged();
    }
}

void ApplicationController::setDocumentText(const QString &text)
{
    if (m_documentText == text) {
        return;
    }

    m_documentText = text;
    m_documentHtml = marknote::markdown::MarkdownRenderer::toHtml(m_documentText);
    updatePreviewHtml();
    const bool wasModified = m_modified;
    m_modified = true;
    if (m_recoveryTimer) {
        m_recoveryTimer->start();
    }
    emit documentTextChanged();
    if (!wasModified) {
        emit modifiedChanged();
    }
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
    m_documentText = text;
    m_documentHtml = marknote::markdown::MarkdownRenderer::toHtml(m_documentText);
    updatePreviewHtml();
    m_hasDocument = true;
    m_modified = true;
    clearRecovery();
    emit currentFileChanged();
    emit documentTextChanged();
    emit modifiedChanged();
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
