#include "MarkdownHighlighter.h"
#include <QColor>

MarkdownHighlighter::MarkdownHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent),
    headingRe("^(#{1,6})\\s*(.*)"),

    boldRe("\\*\\*(.+?)\\*\\*"),
    italicRe("(?<!\\*)\\*(?!\\*)(.+?)(?<!\\*)\\*(?!\\*)"),
    codeRe("`([^`]+)`"),
    linkRe("\\[(.*?)\\]\\((.*?)\\)"),
    quoteRe("^>\\s?.*")
{
    headingFormats.resize(6);
    QColor headingColors[6] = {
        QColor("#61AFEF"),
        QColor("#4FC1FF"),
        QColor("#3BB0FF"),
        QColor("#3BB0FF"),
        QColor("#3BB0FF"),
        QColor("#3BB0FF")
    };
    QVector<int> headingSizes = {20, 18, 16, 14, 14, 14};

    for (int i = 0; i < 6; ++i) {
        headingFormats[i].setForeground(headingColors[i]);
        headingFormats[i].setFontWeight(QFont::Bold);
        headingFormats[i].setFontPointSize(headingSizes[i]);
    }

    boldFormat.setForeground(QColor("#CE5353"));
    boldFormat.setFontWeight(QFont::Bold);

    italicFormat.setForeground(QColor("#5EE895"));
    italicFormat.setFontItalic(true);

    codeFormat.setForeground(QColor("#D19A66"));
    codeFormat.setFontFamily("Consolas");

    linkFormat.setForeground(QColor("#ACA8A7"));
    linkFormat.setFontUnderline(true);

    quoteFormat.setForeground(QColor("#E360B5"));
    quoteFormat.setFontItalic(true);
}

void MarkdownHighlighter::highlightBlock(const QString& text)
{
    auto m = headingRe.match(text);
    if (m.hasMatch()) {
        int level = m.captured(1).length();
        setFormat(0, text.length(), headingFormats[level - 1]);
        return;
    }

    m = quoteRe.match(text);
    if (m.hasMatch()) {
        setFormat(0, text.length(), quoteFormat);
        return;
    }

    auto i = boldRe.globalMatch(text);
    while (i.hasNext()) {
        auto match = i.next();
        setFormat(match.capturedStart(), match.capturedLength(), boldFormat);
    }

    i = italicRe.globalMatch(text);
    while (i.hasNext()) {
        auto match = i.next();
        setFormat(match.capturedStart(), match.capturedLength(), italicFormat);
    }

    i = codeRe.globalMatch(text);
    while (i.hasNext()) {
        auto match = i.next();
        setFormat(match.capturedStart(), match.capturedLength(), codeFormat);
    }

    i = linkRe.globalMatch(text);
    while (i.hasNext()) {
        auto match = i.next();
        setFormat(match.capturedStart(), match.capturedLength(), linkFormat);
    }
}
