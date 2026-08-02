#include "markdown/markdownsyntaxhighlighter.h"

#include <QtGui/QTextCharFormat>
#include <QtGui/QTextDocument>
#include <QtQuick/QQuickTextDocument>

#include <QColor>
#include <QRegularExpression>

namespace marknote::markdown {

MarkdownSyntaxHighlighter::MarkdownSyntaxHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent)
{
}

QObject *MarkdownSyntaxHighlighter::document() const
{
    return m_documentObject;
}

void MarkdownSyntaxHighlighter::setDocument(QObject *documentObject)
{
    if (m_documentObject == documentObject) {
        return;
    }

    m_documentObject = documentObject;
    auto *quickDocument = qobject_cast<QQuickTextDocument *>(documentObject);
    QSyntaxHighlighter::setDocument(quickDocument ? quickDocument->textDocument() : nullptr);
    emit documentChanged();
}

bool MarkdownSyntaxHighlighter::dark() const
{
    return m_dark;
}

void MarkdownSyntaxHighlighter::setDark(bool dark)
{
    if (m_dark == dark) {
        return;
    }
    m_dark = dark;
    rehighlight();
    emit darkChanged();
}

void MarkdownSyntaxHighlighter::highlightBlock(const QString &text)
{
    const QColor headingColor = m_dark ? QColor(QStringLiteral("#66b3ff"))
                                       : QColor(QStringLiteral("#0066cc"));
    const QColor syntaxColor = m_dark ? QColor(QStringLiteral("#ff9f0a"))
                                      : QColor(QStringLiteral("#b45309"));
    const QColor linkColor = m_dark ? QColor(QStringLiteral("#64d2ff"))
                                    : QColor(QStringLiteral("#007aff"));
    const QColor codeColor = m_dark ? QColor(QStringLiteral("#d2a8ff"))
                                    : QColor(QStringLiteral("#7c3aed"));

    if (text.startsWith(QStringLiteral("#"))) {
        const QRegularExpression headingPattern(QStringLiteral("^#{1,6}(?:\\s+.*)?$"));
        if (headingPattern.match(text).hasMatch()) {
            QTextCharFormat format;
            format.setForeground(headingColor);
            format.setFontWeight(QFont::Bold);
            setFormat(0, text.size(), format);
        }
    }

    if (text.startsWith(QStringLiteral(">")) || text.startsWith(QStringLiteral("- "))
        || text.startsWith(QStringLiteral("* ")) || text.startsWith(QStringLiteral("+ "))) {
        QTextCharFormat format;
        format.setForeground(syntaxColor);
        setFormat(0, qMin(3, text.size()), format);
    }

    const QRegularExpression codePattern(QStringLiteral("`[^`]+`"));
    auto codeMatch = codePattern.globalMatch(text);
    while (codeMatch.hasNext()) {
        const auto match = codeMatch.next();
        QTextCharFormat format;
        format.setForeground(codeColor);
        format.setFontFamily(QStringLiteral("Consolas"));
        setFormat(match.capturedStart(), match.capturedLength(), format);
    }

    const QRegularExpression strongPattern(QStringLiteral("(\\*\\*|__)(.+?)(\\1)"));
    auto strongMatch = strongPattern.globalMatch(text);
    while (strongMatch.hasNext()) {
        const auto match = strongMatch.next();
        QTextCharFormat format;
        format.setFontWeight(QFont::Bold);
        setFormat(match.capturedStart(), match.capturedLength(), format);
    }

    const QRegularExpression linkPattern(QStringLiteral("\\[[^\\]]+\\]\\([^\\)]+\\)"));
    auto linkMatch = linkPattern.globalMatch(text);
    while (linkMatch.hasNext()) {
        const auto match = linkMatch.next();
        QTextCharFormat format;
        format.setForeground(linkColor);
        format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        setFormat(match.capturedStart(), match.capturedLength(), format);
    }
}

} // namespace marknote::markdown
