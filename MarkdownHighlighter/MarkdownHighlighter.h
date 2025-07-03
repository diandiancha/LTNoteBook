#ifndef MARKDOWNHIGHLIGHTER_H
#define MARKDOWNHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit MarkdownHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
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
};

#endif // MARKDOWNHIGHLIGHTER_H
