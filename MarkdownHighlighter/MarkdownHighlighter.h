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

    QRegularExpression headingRe;
    QRegularExpression boldRe;
    QRegularExpression italicRe;
    QRegularExpression codeRe;
    QRegularExpression linkRe;
    QRegularExpression quoteRe;
};

#endif // MARKDOWNHIGHLIGHTER_H
