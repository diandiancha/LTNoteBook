#include "MarkdownHighlighter.h"
#include <QColor>
#include <QFont>

MarkdownHighlighter::MarkdownHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent),
    headingRe("^(#{1,6})\\s+(.*)"),  // 标题必须有空格
    boldRe("\\*\\*([^*\\n]+?)\\*\\*|__([^_\\n]+?)__"),  // 支持**和__，不跨行
    italicRe("(?<!\\*)\\*([^*\\n]+?)\\*(?!\\*)|(?<!_)_([^_\\n]+?)_(?!_)"),  // 改进斜体，避免与粗体冲突
    codeRe("`([^`\\n]+?)`"),  // 行内代码不跨行
    linkRe("\\[([^\\]\\n]*?)\\]\\(([^)\\n]+?)\\)"),  // 改进链接匹配
    quoteRe("^\\s*>\\s?(.*)"),  // 支持缩进的引用
    strikethroughRe("~~([^~\\n]+?)~~"),  // 删除线不跨行
    unorderedListRe("^(\\s*)[-*+]\\s+"),  // 精确匹配列表标记
    orderedListRe("^(\\s*)\\d+\\.\\s+"),  // 有序列表
    hrRe("^\\s*(?:(?:-\\s*){3,}|(?:_\\s*){3,}|(?:\\*\\s*){3,})\\s*$"),  // 水平线
    fenceRe("^\\s*```\\s*$"),  // 代码围栏
    tableRe("\\|"),  // 表格分隔符（简化匹配）
    htmlTagRe("<\\/?[a-zA-Z][a-zA-Z0-9]*(?:\\s[^>]*)?>"),  // 改进HTML标签匹配
    htmlBoldRe("<(b|strong)\\b[^>]*>(.*?)<\\/(b|strong)>"),  // HTML粗体
    codeBlockRe("^\\s*```([a-zA-Z0-9_+-]*)\\s*$")  // 改进代码块语言检测
{
    // 标题格式（使用明亮柔和的颜色）
    headingFormats.resize(6);
    QColor headingColors[6] = {
        QColor("#87CEEB"), // H1: 天空蓝
        QColor("#98FB98"), // H2: 淡绿色
        QColor("#DDA0DD"), // H3: 梅红色
        QColor("#F0E68C"), // H4: 卡其色
        QColor("#FFB6C1"), // H5: 浅粉红
        QColor("#FFD700")  // H6: 金色
    };
    QVector<int> headingSizes = {24, 22, 20, 18, 16, 14};
    for (int i = 0; i < 6; ++i) {
        headingFormats[i].setForeground(headingColors[i]);
        headingFormats[i].setFontWeight(QFont::Bold);
        headingFormats[i].setFontPointSize(headingSizes[i]);
    }

    // Markdown 基本格式（使用明亮柔和的颜色）
    boldFormat.setForeground(QColor("#FF6B6B"));        // 亮珊瑚红
    boldFormat.setFontWeight(QFont::Bold);

    italicFormat.setForeground(QColor("#FFB347"));      // 亮橙色
    italicFormat.setFontItalic(true);

    codeFormat.setForeground(QColor("#FF8C69"));        // 亮鲑鱼色
    codeFormat.setFontFamily("Consolas");

    linkFormat.setForeground(QColor("#87CEFA"));        // 亮天空蓝
    linkFormat.setFontUnderline(true);

    quoteFormat.setForeground(QColor("#98D8C8"));       // 薄荷绿
    quoteFormat.setFontItalic(true);

    strikethroughFormat.setForeground(QColor("#C0C0C0"));  // 银灰色
    strikethroughFormat.setFontStrikeOut(true);

    unorderedListFormat.setForeground(QColor("#DDA0DD"));  // 梅红色
    unorderedListFormat.setFontWeight(QFont::Bold);

    orderedListFormat.setForeground(QColor("#87CEEB"));    // 天空蓝
    orderedListFormat.setFontWeight(QFont::Bold);

    hrFormat.setForeground(QColor("#F0E68C"));             // 卡其色
    hrFormat.setFontPointSize(12);

    fenceFormat.setForeground(QColor("#B0B0B0"));          // 亮灰色
    fenceFormat.setFontFamily("Consolas");
    fenceFormat.setFontWeight(QFont::Bold);

    tableFormat.setForeground(QColor("#FFD700"));          // 金色

    htmlTagFormat.setForeground(QColor("#7FFFD4"));        // 碧绿色
    htmlTagFormat.setFontItalic(true);

    // 代码块格式
    codeBlockFormat.setForeground(QColor("#FFFFFF"));      // 白色文字
    codeBlockFormat.setFontFamily("Consolas");

    // 编程语言关键字（使用明亮柔和的配色方案）
    keywordFormat.setForeground(QColor("#DA70D6"));        // 亮洋红色：基本关键字
    keywordFormat.setFontWeight(QFont::Bold);
    keywordFormat.setFontFamily("Consolas");

    controlKeywordFormat.setForeground(QColor("#FF69B4")); // 亮粉红色：控制流关键字
    controlKeywordFormat.setFontWeight(QFont::Bold);
    controlKeywordFormat.setFontFamily("Consolas");

    typeKeywordFormat.setForeground(QColor("#40E0D0"));    // 亮青绿色：类型关键字
    typeKeywordFormat.setFontWeight(QFont::Bold);
    typeKeywordFormat.setFontFamily("Consolas");

    constantFormat.setForeground(QColor("#FF7F50"));       // 珊瑚色：常量
    constantFormat.setFontWeight(QFont::Bold);
    constantFormat.setFontFamily("Consolas");

    stringFormat.setForeground(QColor("#F4A460"));         // 沙棕色：字符串
    stringFormat.setFontFamily("Consolas");

    commentFormat.setForeground(QColor("#90EE90"));        // 亮绿色：注释
    commentFormat.setFontItalic(true);
    commentFormat.setFontFamily("Consolas");

    numberFormat.setForeground(QColor("#FFA500"));         // 亮橙色：数字
    numberFormat.setFontFamily("Consolas");

    preprocessorFormat.setForeground(QColor("#D2B48C"));   // 亮棕褐色：预处理器
    preprocessorFormat.setFontFamily("Consolas");

    functionFormat.setForeground(QColor("#DDA0DD"));       // 亮紫色：函数名
    functionFormat.setFontWeight(QFont::Bold);
    functionFormat.setFontFamily("Consolas");

    operatorFormat.setForeground(QColor("#A9A9A9"));       // 亮灰色：操作符
    operatorFormat.setFontFamily("Consolas");

    classFormat.setForeground(QColor("#87CEEB"));          // 天蓝色：类名
    classFormat.setFontWeight(QFont::Bold);
    classFormat.setFontFamily("Consolas");
}

void MarkdownHighlighter::highlightBlock(const QString& text)
{
    QRegularExpressionMatch m;

    // 检查代码块标记（更严格的匹配）
    m = codeBlockRe.match(text);
    if (m.hasMatch()) {
        QString language = m.captured(1);
        setFormat(0, text.length(), fenceFormat);

        if (previousBlockState() != 1) {
            setCurrentBlockState(1);
            currentLanguage = language.toLower(); // 保存并转换为小写
        } else {
            setCurrentBlockState(0);
            currentLanguage.clear();
        }
        return;
    }

    // 代码块内部处理
    if (previousBlockState() == 1) {
        setCurrentBlockState(1);
        setFormat(0, text.length(), codeBlockFormat);
        highlightCodeBlock(text, currentLanguage);
        return;
    }

    // 标题处理（要求标题后有空格）
    m = headingRe.match(text);
    if (m.hasMatch()) {
        int lvl = m.captured(1).length();
        if (lvl <= 6) {
            setFormat(0, text.length(), headingFormats[lvl - 1]);
            return;
        }
    }

    // 引用块处理（支持嵌套和缩进）
    m = quoteRe.match(text);
    if (m.hasMatch()) {
        setFormat(0, text.length(), quoteFormat);
        return;
    }

    // 水平分割线（更严格的匹配）
    m = hrRe.match(text);
    if (m.hasMatch()) {
        setFormat(0, text.length(), hrFormat);
        return;
    }

    // 列表项处理（精确匹配缩进）
    m = unorderedListRe.match(text);
    if (m.hasMatch()) {
        setFormat(m.capturedStart(), m.capturedLength(), unorderedListFormat);
    }

    m = orderedListRe.match(text);
    if (m.hasMatch()) {
        setFormat(m.capturedStart(), m.capturedLength(), orderedListFormat);
    }

    // 行内代码处理（不跨行，优先级最高）
    auto it = codeRe.globalMatch(text);
    QVector<QPair<int, int>> codeRanges;
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), codeFormat);
        codeRanges.append({match.capturedStart(), match.capturedEnd()});
    }

    // 其他行内格式（避开代码区域）
    highlightInlineFormats(text, codeRanges);
}

void MarkdownHighlighter::highlightInlineFormats(const QString& text, const QVector<QPair<int, int>>& excludeRanges)
{
    // 删除线
    auto it = strikethroughRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        if (!isInRange(match.capturedStart(), excludeRanges)) {
            setFormat(match.capturedStart(), match.capturedLength(), strikethroughFormat);
        }
    }

    // 链接
    it = linkRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        if (!isInRange(match.capturedStart(), excludeRanges)) {
            setFormat(match.capturedStart(), match.capturedLength(), linkFormat);
        }
    }

    // 粗体
    it = boldRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        if (!isInRange(match.capturedStart(), excludeRanges)) {
            setFormat(match.capturedStart(), match.capturedLength(), boldFormat);
        }
    }

    // 斜体
    it = italicRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        if (!isInRange(match.capturedStart(), excludeRanges)) {
            setFormat(match.capturedStart(), match.capturedLength(), italicFormat);
        }
    }

    // HTML 标签（简化匹配，更容易识别）
    it = htmlTagRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        if (!isInRange(match.capturedStart(), excludeRanges)) {
            setFormat(match.capturedStart(), match.capturedLength(), htmlTagFormat);
        }
    }

    // HTML 粗体（改进匹配）
    it = htmlBoldRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        if (!isInRange(match.capturedStart(), excludeRanges)) {
            // 高亮整个标签
            setFormat(match.capturedStart(), match.capturedLength(), htmlTagFormat);
            // 高亮内容为粗体
            if (match.capturedLength(2) > 0) {
                setFormat(match.capturedStart(2), match.capturedLength(2), boldFormat);
            }
        }
    }

    // 表格分隔符（简化处理）
    it = tableRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        if (!isInRange(match.capturedStart(), excludeRanges)) {
            setFormat(match.capturedStart(), match.capturedLength(), tableFormat);
        }
    }
}

bool MarkdownHighlighter::isInRange(int pos, const QVector<QPair<int, int>>& ranges)
{
    for (const auto& range : ranges) {
        if (pos >= range.first && pos < range.second) {
            return true;
        }
    }
    return false;
}

void MarkdownHighlighter::highlightCodeBlock(const QString& text, const QString& language)
{
    if (text.trimmed().isEmpty()) {
        return;
    }

    // 定义不同类型的关键字
    QStringList basicKeywords;
    QStringList controlKeywords;
    QStringList typeKeywords;
    QStringList constants;

    // 根据语言设置关键字
    QString lang = language.toLower();
    if (lang == "cpp" || lang == "c++" || lang == "c" || lang.isEmpty()) {
        basicKeywords = {"auto", "const", "static", "extern", "inline", "virtual", "override",
                         "explicit", "friend", "mutable", "volatile", "register", "thread_local"};
        controlKeywords = {"if", "else", "for", "while", "do", "switch", "case", "default",
                           "return", "break", "continue", "goto", "try", "catch", "throw"};
        typeKeywords = {"int", "char", "float", "double", "void", "bool", "long", "short",
                        "unsigned", "signed", "class", "struct", "union", "enum", "typedef",
                        "namespace", "template", "typename"};
        constants = {"true", "false", "nullptr", "NULL"};
    } else if (lang == "python" || lang == "py") {
        basicKeywords = {"def", "class", "import", "from", "as", "global", "nonlocal",
                         "lambda", "with", "pass", "del", "assert", "yield", "async", "await"};
        controlKeywords = {"if", "elif", "else", "for", "while", "try", "except", "finally",
                           "return", "break", "continue", "raise"};
        typeKeywords = {"int", "float", "str", "list", "dict", "tuple", "set", "bool"};
        constants = {"True", "False", "None", "self", "cls"};
    } else if (lang == "javascript" || lang == "js") {
        basicKeywords = {"var", "let", "const", "function", "class", "extends", "super",
                         "import", "export", "from", "as", "async", "await", "yield"};
        controlKeywords = {"if", "else", "for", "while", "do", "switch", "case", "default",
                           "return", "break", "continue", "try", "catch", "finally", "throw"};
        typeKeywords = {"typeof", "instanceof", "new", "this", "prototype"};
        constants = {"true", "false", "null", "undefined"};
    }

    // 高亮不同类型的关键字
    highlightKeywords(text, basicKeywords, keywordFormat);
    highlightKeywords(text, controlKeywords, controlKeywordFormat);
    highlightKeywords(text, typeKeywords, typeKeywordFormat);
    highlightKeywords(text, constants, constantFormat);

    // 高亮函数调用
    QRegularExpression functionRe("\\b([a-zA-Z_][\\w]*)\\s*(?=\\()");
    auto it = functionRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(1), match.capturedLength(1), functionFormat);
    }

    // 高亮类名
    QRegularExpression classRe("\\b([A-Z][a-zA-Z0-9_]*)\\b");
    it = classRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        QString word = match.captured(1);
        // 避免与其他关键字冲突
        if (!basicKeywords.contains(word) && !controlKeywords.contains(word) &&
            !typeKeywords.contains(word) && !constants.contains(word)) {
            setFormat(match.capturedStart(1), match.capturedLength(1), classFormat);
        }
    }

    // 高亮字符串
    QRegularExpression stringRe(R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'|`(?:[^`\\]|\\.)*`)");
    it = stringRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), stringFormat);
    }

    // 高亮注释
    QRegularExpression commentRe("//.*$|/\\*.*?\\*/|#.*$|<!--.*?-->|\"\"\".*?\"\"\"|'''.*?'''");
    it = commentRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), commentFormat);
    }

    // 高亮数字
    QRegularExpression numberRe("\\b(?:0x[0-9a-fA-F]+|0b[01]+|\\d+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?[fFdDlL]?)\\b");
    it = numberRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), numberFormat);
    }

    // 高亮预处理器指令
    QRegularExpression preprocessorRe("^\\s*#\\w+.*$");
    it = preprocessorRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), preprocessorFormat);
    }

    // 高亮操作符
    QRegularExpression operatorRe("[+\\-*/=<>!&|^~%]+|\\b(and|or|not|in|is)\\b");
    it = operatorRe.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), operatorFormat);
    }
}

void MarkdownHighlighter::highlightKeywords(const QString& text, const QStringList& keywords, const QTextCharFormat& format)
{
    for (const QString& keyword : keywords) {
        QRegularExpression keywordRe("\\b" + QRegularExpression::escape(keyword) + "\\b");
        auto it = keywordRe.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), format);
        }
    }
}
