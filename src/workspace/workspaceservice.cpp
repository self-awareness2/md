#include "workspace/workspaceservice.h"

#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>

namespace {

bool isMarkdownFile(const QString &fileName)
{
    return fileName.endsWith(QLatin1String(".md"), Qt::CaseInsensitive)
           || fileName.endsWith(QLatin1String(".markdown"), Qt::CaseInsensitive);
}

} // namespace

WorkspaceService::WorkspaceService(QObject *parent)
    : QObject(parent)
{
    QSettings settings;
    const QString stored = settings.value(QStringLiteral("workspaceRoot")).toString();
    if (!stored.isEmpty() && QFileInfo::exists(stored)) {
        openFolderPath(stored);
    }
}

QString WorkspaceService::rootPath() const
{
    return m_rootPath;
}

QString WorkspaceService::rootName() const
{
    return m_rootPath.isEmpty() ? QString() : QFileInfo(m_rootPath).fileName();
}

QStringList WorkspaceService::markdownFiles() const
{
    return m_markdownFiles;
}

bool WorkspaceService::hasWorkspace() const
{
    return !m_rootPath.isEmpty();
}

void WorkspaceService::openFolder()
{
    const QString path = QFileDialog::getExistingDirectory(
        nullptr,
        tr("Open workspace folder"),
        m_rootPath);
    if (!path.isEmpty()) {
        openFolderPath(path);
    }
}

bool WorkspaceService::openFolderPath(const QString &path)
{
    const QString absolute = QFileInfo(path).absoluteFilePath();
    QFileInfo info(absolute);
    if (!info.exists() || !info.isDir()) {
        emit notificationRequested(tr("Workspace folder does not exist"));
        return false;
    }

    const bool rootChanged = m_rootPath != absolute;
    m_rootPath = absolute;
    QSettings settings;
    settings.setValue(QStringLiteral("workspaceRoot"), m_rootPath);
    scan();
    if (rootChanged) {
        emit rootPathChanged();
    }
    emit notificationRequested(tr("Opened workspace %1").arg(rootName()));
    return true;
}

void WorkspaceService::closeWorkspace()
{
    if (m_rootPath.isEmpty() && m_markdownFiles.isEmpty()) {
        return;
    }
    m_rootPath.clear();
    m_markdownFiles.clear();
    QSettings settings;
    settings.remove(QStringLiteral("workspaceRoot"));
    emit rootPathChanged();
    emit markdownFilesChanged();
}

void WorkspaceService::refresh()
{
    if (m_rootPath.isEmpty()) {
        return;
    }
    scan();
}

QStringList WorkspaceService::searchFiles(const QString &query) const
{
    if (query.trimmed().isEmpty()) {
        return m_markdownFiles;
    }
    const QString needle = query.trimmed();
    QStringList matches;
    for (const QString &path : m_markdownFiles) {
        if (path.contains(needle, Qt::CaseInsensitive)
            || QFileInfo(path).fileName().contains(needle, Qt::CaseInsensitive)) {
            matches.push_back(path);
        }
    }
    return matches;
}

void WorkspaceService::scan()
{
    QStringList files;
    if (!m_rootPath.isEmpty()) {
        QDirIterator it(
            m_rootPath,
            QDir::Files | QDir::Readable | QDir::NoSymLinks,
            QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            const QString relative = QDir(m_rootPath).relativeFilePath(path);
            if (relative.startsWith(QLatin1String(".git/"))
                || relative.contains(QLatin1String("/.git/"))) {
                continue;
            }
            if (isMarkdownFile(QFileInfo(path).fileName())) {
                files.push_back(QDir::toNativeSeparators(path));
            }
            if (files.size() >= 2000) {
                break;
            }
        }
        files.sort(Qt::CaseInsensitive);
    }
    if (files != m_markdownFiles) {
        m_markdownFiles = std::move(files);
        emit markdownFilesChanged();
    }
}
