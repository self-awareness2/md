#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class WorkspaceService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath NOTIFY rootPathChanged)
    Q_PROPERTY(QString rootName READ rootName NOTIFY rootPathChanged)
    Q_PROPERTY(QStringList markdownFiles READ markdownFiles NOTIFY markdownFilesChanged)
    Q_PROPERTY(bool hasWorkspace READ hasWorkspace NOTIFY rootPathChanged)

public:
    explicit WorkspaceService(QObject *parent = nullptr);

    [[nodiscard]] QString rootPath() const;
    [[nodiscard]] QString rootName() const;
    [[nodiscard]] QStringList markdownFiles() const;
    [[nodiscard]] bool hasWorkspace() const;

    Q_INVOKABLE void openFolder();
    Q_INVOKABLE bool openFolderPath(const QString &path);
    Q_INVOKABLE void closeWorkspace();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QStringList searchFiles(const QString &query) const;
    [[nodiscard]] Q_INVOKABLE QString relativePath(const QString &absolutePath) const;

signals:
    void rootPathChanged();
    void markdownFilesChanged();
    void notificationRequested(const QString &message);

private:
    QString m_rootPath;
    QStringList m_markdownFiles;

    void scan();
};
