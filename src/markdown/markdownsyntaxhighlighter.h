#pragma once

#include <QSyntaxHighlighter>

class QQuickTextDocument;

namespace marknote::markdown {

class MarkdownSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    Q_PROPERTY(QObject *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged)

public:
    explicit MarkdownSyntaxHighlighter(QObject *parent = nullptr);

    [[nodiscard]] QObject *document() const;
    void setDocument(QObject *document);
    [[nodiscard]] bool dark() const;
    void setDark(bool dark);

signals:
    void documentChanged();
    void darkChanged();

protected:
    void highlightBlock(const QString &text) override;

private:
    QObject *m_documentObject = nullptr;
    bool m_dark = false;
};

} // namespace marknote::markdown
