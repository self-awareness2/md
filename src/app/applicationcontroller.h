#pragma once

#include "markdown/documentast.h"
#include "workspace/workspaceservice.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QDateTime>

#include <vector>

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
    Q_PROPERTY(QVariantList documentBlocks READ documentBlocks NOTIFY documentBlocksChanged)
    Q_PROPERTY(QVariantList documentOutline READ documentOutline NOTIFY documentBlocksChanged)
    Q_PROPERTY(bool hasDocument READ hasDocument NOTIFY documentTextChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(bool recoveryAvailable READ recoveryAvailable NOTIFY recoveryAvailableChanged)
    Q_PROPERTY(bool externalChangeDetected READ externalChangeDetected NOTIFY externalChangeDetectedChanged)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(WorkspaceService *workspace READ workspace CONSTANT)
    Q_PROPERTY(bool canOpenPreviousWorkspaceFile READ canOpenPreviousWorkspaceFile NOTIFY workspaceNavigationChanged)
    Q_PROPERTY(bool canOpenNextWorkspaceFile READ canOpenNextWorkspaceFile NOTIFY workspaceNavigationChanged)
    Q_PROPERTY(int workspaceFileIndex READ workspaceFileIndex NOTIFY workspaceNavigationChanged)
    Q_PROPERTY(int workspaceFileCount READ workspaceFileCount NOTIFY workspaceNavigationChanged)

public:
    explicit ApplicationController(QObject *parent = nullptr);

    [[nodiscard]] QString version() const;
    [[nodiscard]] QString currentFile() const;
    [[nodiscard]] QString currentFileName() const;
    [[nodiscard]] QString documentText() const;
    [[nodiscard]] QString documentHtml() const;
    [[nodiscard]] QString documentPreviewHtml() const;
    [[nodiscard]] QVariantList documentBlocks() const;
    [[nodiscard]] QVariantList documentOutline() const;
    [[nodiscard]] bool hasDocument() const;
    [[nodiscard]] bool modified() const;
    [[nodiscard]] bool recoveryAvailable() const;
    [[nodiscard]] bool externalChangeDetected() const;
    [[nodiscard]] QStringList recentFiles() const;
    [[nodiscard]] WorkspaceService *workspace() const;
    [[nodiscard]] bool canOpenPreviousWorkspaceFile() const;
    [[nodiscard]] bool canOpenNextWorkspaceFile() const;
    [[nodiscard]] int workspaceFileIndex() const;
    [[nodiscard]] int workspaceFileCount() const;

    Q_INVOKABLE void openFile();
    Q_INVOKABLE bool openPath(const QString &path);
    Q_INVOKABLE bool openPreviousWorkspaceFile();
    Q_INVOKABLE bool openNextWorkspaceFile();
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

    Q_INVOKABLE bool updateBlockDisplay(qulonglong blockId, const QString &displayText);
    Q_INVOKABLE bool setBlockKind(qulonglong blockId, const QString &kindName, int level = 0);
    Q_INVOKABLE bool toggleTaskChecked(qulonglong blockId);
    Q_INVOKABLE bool insertBlockAfter(qulonglong blockId, const QString &kindName = QStringLiteral("paragraph"));
    Q_INVOKABLE bool deleteBlock(qulonglong blockId);
    Q_INVOKABLE QString serializeBlocksToDocument() const;

signals:
    void currentFileChanged();
    void documentTextChanged();
    void documentBlocksChanged();
    void modifiedChanged();
    void notificationRequested(const QString &message);
    void imagePrepared(const QString &relativePath);
    void recoveryAvailableChanged();
    void externalChangeDetectedChanged();
    void recentFilesChanged();
    void workspaceNavigationChanged();

private:
    QString m_currentFile;
    QString m_documentText;
    QString m_documentHtml;
    QString m_documentPreviewHtml;
    std::vector<marknote::markdown::AstBlock> m_blocks;
    bool m_hasDocument = false;
    bool m_modified = false;
    bool m_recoveryAvailable = false;
    bool m_externalChangeDetected = false;
    bool m_suppressBlockRebuild = false;
    QFileSystemWatcher *m_fileWatcher = nullptr;
    QDateTime m_lastDiskModified;
    qint64 m_lastDiskSize = -1;
    QStringList m_recentFiles;
    QTimer *m_recoveryTimer = nullptr;
    WorkspaceService *m_workspace = nullptr;

    [[nodiscard]] QString recoveryPath() const;
    void writeRecovery();
    void clearRecovery();
    void watchCurrentFile();
    void handleFileChanged(const QString &path);
    void addRecentFile(const QString &path);
    void updatePreviewHtml();
    void rebuildAst();
    void applyDocumentText(const QString &text, bool markModified);
    [[nodiscard]] marknote::markdown::AstBlock *findBlock(qulonglong blockId);
    void replaceDocumentFromBlocks();
    [[nodiscard]] int workspaceIndexOfCurrent() const;
    bool openWorkspaceFileByDelta(int delta);
};
