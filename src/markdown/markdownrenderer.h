#pragma once

#include <QString>

namespace marknote::markdown {

class MarkdownRenderer final
{
public:
    // Returns safe HTML. Raw HTML in the source is omitted by cmark's safe mode.
    [[nodiscard]] static QString toHtml(const QString &markdown);
};

} // namespace marknote::markdown

