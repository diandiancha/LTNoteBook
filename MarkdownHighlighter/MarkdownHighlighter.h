// MarkdownHighlighter.h
#ifndef MARKDOWNHIGHLIGHTER_H
#define MARKDOWNHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QRegularExpression>
#include <QTextCharFormat>

class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit MarkdownHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    // Markdown 语法正则表达式
    QRegularExpression headingRe;
    QRegularExpression boldRe;
    QRegularExpression italicRe;
    QRegularExpression codeRe;
    QRegularExpression linkRe;
    QRegularExpression quoteRe;
    QRegularExpression strikethroughRe;
    QRegularExpression unorderedListRe;
    QRegularExpression orderedListRe;
    QRegularExpression hrRe;
    QRegularExpression fenceRe;
    QRegularExpression tableRe;
    QRegularExpression htmlTagRe;
    QRegularExpression htmlBoldRe;
    QRegularExpression codeBlockRe;

    // Markdown 格式
    QVector<QTextCharFormat> headingFormats;
    QTextCharFormat boldFormat;
    QTextCharFormat italicFormat;
    QTextCharFormat codeFormat;
    QTextCharFormat linkFormat;
    QTextCharFormat quoteFormat;
    QTextCharFormat strikethroughFormat;
    QTextCharFormat unorderedListFormat;
    QTextCharFormat orderedListFormat;
    QTextCharFormat hrFormat;
    QTextCharFormat fenceFormat;
    QTextCharFormat tableFormat;
    QTextCharFormat htmlTagFormat;

    // 代码块高亮格式（参考VSCode配色）
    QTextCharFormat codeBlockFormat;
    QTextCharFormat keywordFormat;           // 基本关键字（蓝色）
    QTextCharFormat controlKeywordFormat;    // 控制流关键字（紫色）
    QTextCharFormat typeKeywordFormat;       // 类型关键字（青绿色）
    QTextCharFormat constantFormat;          // 常量（浅蓝色）
    QTextCharFormat stringFormat;            // 字符串（橙色）
    QTextCharFormat commentFormat;           // 注释（绿色）
    QTextCharFormat numberFormat;            // 数字（浅绿色）
    QTextCharFormat preprocessorFormat;      // 预处理器（灰色）
    QTextCharFormat functionFormat;          // 函数名（黄色）
    QTextCharFormat operatorFormat;          // 操作符（白色）
    QTextCharFormat classFormat;             // 类名（青绿色）

    // 辅助变量
    QString currentLanguage;  // 当前代码块语言

    // 辅助方法
    void highlightInlineFormats(const QString& text, const QVector<QPair<int, int>>& excludeRanges);
    bool isInRange(int pos, const QVector<QPair<int, int>>& ranges);
    void highlightCodeBlock(const QString& text, const QString& language);
    void highlightKeywords(const QString& text, const QStringList& keywords, const QTextCharFormat& format);
};

#endif // MARKDOWNHIGHLIGHTER_H
