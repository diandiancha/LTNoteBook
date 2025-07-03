// MarkdownHighlighter.cpp
#include "MarkdownHighlighter.h"
#include <QColor>
#include <QFont>

MarkdownHighlighter::MarkdownHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent),
    headingRe("^(#{1,6})\\s*(.*)"),
    boldRe("\\*\\*(.+?)\\*\\*"),
    italicRe("(?<!\\*)\\*(?!\\*)(.+?)(?<!\\*)\\*(?!\\*)"),
    codeRe("`([^`]+)`"),
    linkRe("\\[(.*?)\\]\\((.*?)\\)"),
    quoteRe("^>\\s?.*"),
    strikethroughRe("~~(.+?)~~"),
    unorderedListRe("^\\s*[-\\*\\+]\\s+"),
    orderedListRe("^\\s*\\d+\\.\\s+"),
    hrRe("^(?:(?:-\\s?){3,}|(?:_\\s?){3,}|(?:\\*\\s?){3,})\\s*$"),
    fenceRe("^```.*"),
    tableRe("\\|"),
    htmlTagRe("<[^>]+>"),
    htmlBoldRe("<(?:b|strong)>(.*?)</(?:b|strong)>")
{
    // БъЬт
    headingFormats.resize(6);
    QColor headingColors[6] = {
        QColor("#61AFEF"), QColor("#4FC1FF"),
        QColor("#3BB0FF"), QColor("#3BB0FF"),
        QColor("#3BB0FF"), QColor("#3BB0FF")
    };
    QVector<int> headingSizes = { 22,20,18,16,14,12 };
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



    strikethroughFormat.setForeground(QColor("#0CAFC1"));
    strikethroughFormat.setFontStrikeOut(true);

    unorderedListFormat.setForeground(QColor("#9C74D9"));
    unorderedListFormat.setFontWeight(QFont::Bold);

    orderedListFormat.setForeground(QColor("#A2CFFF"));
    orderedListFormat.setFontWeight(QFont::Bold);

    hrFormat.setForeground(QColor("#E16906"));
    hrFormat.setFontPointSize(12);

    fenceFormat.setForeground(QColor("#B9DF21"));
    fenceFormat.setFontFamily("Consolas");
    fenceFormat.setFontWeight(QFont::Bold);

    tableFormat.setForeground(QColor("#F7E57F"));

    htmlTagFormat.setForeground(QColor("#EA98DE"));
    htmlTagFormat.setFontItalic(true);
}

void MarkdownHighlighter::highlightBlock(const QString& text)
{
    QRegularExpressionMatch m;

    m = headingRe.match(text);
    if (m.hasMatch()) {
        int lvl = m.captured(1).length();
        setFormat(0, text.length(), headingFormats[lvl - 1]);
        return;
    }
    m = quoteRe.match(text);
    if (m.hasMatch()) {
        setFormat(0, text.length(), quoteFormat);
        return;
    }
    m = fenceRe.match(text);
    if (m.hasMatch()) {
        setFormat(0, text.length(), fenceFormat);
        return;
    }
    m = hrRe.match(text);
    if (m.hasMatch()) {
        setFormat(0, text.length(), hrFormat);
        return;
    }
    m = unorderedListRe.match(text);
    if (m.hasMatch()) {
        setFormat(m.capturedStart(), m.capturedLength(), unorderedListFormat);
    }
    m = orderedListRe.match(text);
    if (m.hasMatch()) {
        setFormat(m.capturedStart(), m.capturedLength(), orderedListFormat);
    }
    auto it = codeRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), codeFormat);
    }
    it = strikethroughRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), strikethroughFormat);
    }
    it = linkRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), linkFormat);
    }
    it = boldRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), boldFormat);
    }
    it = italicRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), italicFormat);
    }
    it = htmlTagRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), htmlTagFormat);
    }
    it = htmlBoldRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(1),
            match.capturedLength(1),
            boldFormat);
    }
    it = tableRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), 1, tableFormat);
    }
}
