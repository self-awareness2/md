#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>

class QTimer;
class QFileSystemWatcher;

class ApplicationController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(QString currentFileName READ currentFileName NOTIFY currentFileChanged)
    Q_PROPERTY(QString documentText READ documentText NOTIFY documentTextChanged)
    Q_PROPERTY(QString documentHtml READ documentHtml NOTIFY documentTextChanged)
    Q_PROPERTY(QString documentPreviewHtml READ documentPreviewHtml NOTIFY documentTextChanged)
    Q_PROPERTY(bool hasDocument READ hasDocument NOTIFY documentTextChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(bool recoveryAvailable READ recoveryAvailable NOTIFY recoveryAvailableChanged)
    Q_PROPERTY(bool externalChangeDetected READ externalChangeDetected NOTIFY externalChangeDetectedChanged)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)

public:
    explicit ApplicationController(QObject *parent = nullptr);

    [[nodiscard]] QString version() const;
    [[nodiscard]] QString currentFile() const;
    [[nodiscard]] QString currentFileName() const;
    [[nodiscard]] QString documentText() const;
    [[nodiscard]] QString documentHtml() const;
    [[nodiscard]] QString documentPreviewHtml() const;
    [[nodiscard]] bool hasDocument() const;
    [[nodiscard]] bool modified() const;
    [[nodiscard]] bool recoveryAvailable() const;
    [[nodiscard]] bool externalChangeDetected() const;
    [[nodiscard]] QStringList recentFiles() const;

    Q_INVOKABLE void openFile();
    Q_INVOKABLE bool openPath(const QString &path);
    Q_INVOKABLE void newDocument();
    Q_INVOKABLE void closeDocument();
    Q_INVOKABLE bool save();
    Q_INVOKABLE void saveAs();
    Q_INVOKABLE bool savePath(const QString &path);
    Q_INVOKABLE void setDocumentText(const QString &text);
    Q_INVOKABLE bool exportHtml(const QString &path);
    Q_INVOKABLE void exportHtmlAs();
    Q_INVOKABLE bool exportPdf(const QString &path);
    Q_INVOKABLE void exportPdfAs();
    Q_INVOKABLE QString prepareImage(const QString &path);
    Q_INVOKABLE void insertImage();
    Q_INVOKABLE bool recoverDocument();
    Q_INVOKABLE void discardRecovery();
    Q_INVOKABLE bool reloadFromDisk();
    Q_INVOKABLE bool openLink(const QString &link);
    Q_INVOKABLE void clearRecentFiles();

signals:
    void currentFileChanged();
    void documentTextChanged();
    void modifiedChanged();
    void notificationRequested(const QString &message);
    void imagePrepared(const QString &relativePath);
    void recoveryAvailableChanged();
    void externalChangeDetectedChanged();
    void recentFilesChanged();

private:
    QString m_currentFile;
    QString m_documentText;
    QString m_documentHtml;
    QString m_documentPreviewHtml;
    bool m_hasDocument = false;
    bool m_modified = false;
    bool m_recoveryAvailable = false;
    bool m_externalChangeDetected = false;
    QFileSystemWatcher *m_fileWatcher = nullptr;
    QDateTime m_lastDiskModified;
    qint64 m_lastDiskSize = -1;
    QStringList m_recentFiles;
    QTimer *m_recoveryTimer = nullptr;

    [[nodiscard]] QString recoveryPath() const;
    void writeRecovery();
    void clearRecovery();
    void watchCurrentFile();
    void handleFileChanged(const QString &path);
    void addRecentFile(const QString &path);
    void updatePreviewHtml();
};
