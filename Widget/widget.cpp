#include "widget.h"
#include "ui_widget.h"
#include "config.h"
#include "TranslationManager.h"
#include "shortsetting.h"
#include <QSettings>
#include <QApplication>
#include <QLibraryInfo>
#include <QToolTip>
#include <QLabel>
#include <QMenu>
#include <QTimer>
#include <QHelpEvent>
#include <QOverload>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

const QStringConverter::Encoding DEFAULT_ENCODING = QStringConverter::Utf8;

static const QStringList supportedTextExtensions = {
    "txt", "md", "cpp", "h", "c", "hpp", "cc", "cxx",
    "py", "js", "html", "css", "json", "xml",
    "log", "ini", "cfg", "conf"
};


QString Widget::encodingToString(QStringConverter::Encoding e)
{
    switch (e) {
    case QStringConverter::Utf8:    return "UTF-8";
    case QStringConverter::Utf16LE: return "UTF-16 LE";
    case QStringConverter::Utf16BE: return "UTF-16 BE";
    case QStringConverter::System:  return "ANSI";
    default:
        return "UTF-8";

    }
    return "UTF-8";
}

QStringConverter::Encoding Widget::encodingFromString(const QString &s)
{
    if (s.compare("UTF-8", Qt::CaseInsensitive) == 0)    return QStringConverter::Utf8;
    if (s.compare("UTF-16 LE", Qt::CaseInsensitive) == 0) return QStringConverter::Utf16LE;
    if (s.compare("UTF-16 BE", Qt::CaseInsensitive) == 0) return QStringConverter::Utf16BE;
    if (s.compare("ANSI", Qt::CaseInsensitive) == 0)     return QStringConverter::System;
    return DEFAULT_ENCODING;
}

#ifdef Q_OS_WIN
void Widget::registerFileAssociations()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QStringList extensions = supportedTextExtensions;

    // 检查是否已有其他程序注册
    QSettings registry("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    QStringList conflictExts;

    for (const QString &ext : extensions) {
        QString currentApp = registry.value("." + ext + "/.", "").toString();
        if (!currentApp.isEmpty() && !currentApp.startsWith("LTNoteBook")) {
            conflictExts.append(ext);
        }
    }

    if (!conflictExts.isEmpty()) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, tr("文件关联"),
                                                                  tr("以下文件类型已被其他程序关联：\n%1\n\n是否仍要关联到 LTNoteBook？")
                                                                      .arg(conflictExts.join(", ")),
                                                                  QMessageBox::Yes | QMessageBox::No,
                                                                  QMessageBox::No);

        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    try {
        for (const QString &ext : extensions) {
            registerSingleFileType(ext, appPath);
        }

        // 添加右键菜单
        addContextMenuEntry();

        QMessageBox::information(this, tr("文件关联"),
                                 tr("文件关联设置成功！\n\n支持的格式：.txt, .md, .cpp, .h, .c, .hpp, .py, .js, .html, .css, .json, .xml, .log\n\n已添加右键菜单支持。"));
    } catch (...) {
        QMessageBox::warning(this, tr("文件关联"),
                             tr("文件关联设置失败。可能需要管理员权限。"));
    }
}

void Widget::registerSingleFileType(const QString &extension, const QString &appPath)
{
    QSettings registry("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);

    QString progId = QString("LTNoteBook.%1").arg(extension);
    QString description = tr("LTNoteBook 文档");

    // 注册文件扩展名
    registry.setValue("." + extension + "/.", progId);

    // 注册程序ID
    registry.setValue(progId + "/.", description);
    registry.setValue(progId + "/DefaultIcon/.", QString("%1,0").arg(appPath));
    registry.setValue(progId + "/shell/open/command/.",
                      QString("\"%1\" \"%2\"").arg(appPath, "%1"));

    // 添加到"打开方式"菜单
    registry.setValue(progId + "/shell/open/.", tr("用 LTNoteBook 打开"));
}

void Widget::unregisterFileAssociations()
{
    QSettings registry("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    QStringList extensions = supportedTextExtensions;
    for (const QString &ext : extensions) {
        QString progId = QString("LTNoteBook.%1").arg(ext);
        // 移除注册项
        registry.remove("." + ext);
        registry.remove(progId);
    }

    // 添加移除右键菜单
    removeContextMenuEntry();

    QMessageBox::information(this, tr("文件关联"),
                             tr("文件关联已移除。"));
}

void Widget::addContextMenuEntry()
{
    QString appPath = QCoreApplication::applicationFilePath();
    QSettings registry("HKEY_CURRENT_USER\\Software\\Classes\\*\\shell", QSettings::NativeFormat);

    // 添加右键菜单项
    registry.setValue("LTNoteBook/.", tr("用 LTNoteBook 编辑"));
    registry.setValue("LTNoteBook/Icon", QString("%1,0").arg(appPath));
    registry.setValue("LTNoteBook/command/.", QString("\"%1\" \"%2\"").arg(appPath, "%1"));
}

void Widget::removeContextMenuEntry()
{
    QSettings registry("HKEY_CURRENT_USER\\Software\\Classes\\*\\shell", QSettings::NativeFormat);
    registry.remove("LTNoteBook");
}
#endif

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , mdHighlighter(nullptr)
    , m_hasInitialBackup(false)
    , m_smartBackupEnabled(true)
{
    QSettings s(ORG,APP);

    ui->setupUi(this);
    ui->retranslateUi(this);
    qApp->installEventFilter(this);

    bool markdownOn = s.value(KEY_MD, false).toBool();
    if (markdownOn){
        mdHighlighter = new MarkdownHighlighter(ui->textEdit->document());
    }

    ui->textEdit->installEventFilter(this);

    // === 正确的connect语法 ===
    connect(ui->fileCombobox,
            QOverload<int>::of(&QComboBox::activated),
            this,
            &Widget::on_fileCombobox_activated);

    connect(ui->fileCombobox, &QComboBox::currentTextChanged,
            this, &Widget::on_fileCombobox_currentTextChanged);

    connect(ui->textEdit, &QTextEdit::textChanged, this, &Widget::updateCharCount);

    connect(ui->textEdit, &QTextEdit::cursorPositionChanged,
            this, &Widget::updateCursorPosition);

    connect(ui->fileArrangement,
            QOverload<int>::of(&QComboBox::activated),
            this,
            &Widget::on_fileArrangement_activated);

    connect(ui->fontBox,
            QOverload<const QFont&>::of(&QFontComboBox::currentFontChanged),
            this,
            &Widget::on_fontBox_currentFontChanged);

    connect(ui->textEdit->document(), &QTextDocument::undoAvailable,
            ui->Reset, &QPushButton::setEnabled);
    connect(ui->textEdit->document(), &QTextDocument::redoAvailable,
            ui->Withdraw, &QPushButton::setEnabled);
    ui->Reset->setEnabled(false);    // 初始状态禁用撤销按钮
    ui->Withdraw->setEnabled(false); // 初始状态禁用重做按钮

    loadShortcuts();

    updateCharCount();
    updateCursorPosition();
    updateFileArrangement();

    // 美化 textEdit 滚动条 + 保留原有样式
    QString systemStyleScrollbar = R"(
    QTextEdit {
        background-color: rgb(43, 44, 47);
        color: rgb(255, 255, 255);
    }

    /* 垂直滚动条 */
    QTextEdit QScrollBar:vertical {
        background: #2a2a2a;
        width: 16px;
        border: none;
        margin: 0px;
    }

    /* 滚动条滑块 */
    QTextEdit QScrollBar::handle:vertical {
        background: #555555;
        border: 1px solid #666666;
        border-radius: 2px;
        min-height: 20px;
        margin: 1px;
    }

    /* 滚动条滑块悬停 */
    QTextEdit QScrollBar::handle:vertical:hover {
        background: #666666;
        border: 1px solid #777777;
    }

    /* 滚动条滑块按下 */
    QTextEdit QScrollBar::handle:vertical:pressed {
        background: #777777;
        border: 1px solid #888888;
    }

    /* 隐藏箭头按钮 */
    QTextEdit QScrollBar::add-line:vertical,
    QTextEdit QScrollBar::sub-line:vertical {
        height: 0px;
    }

    /* 滚动条背景区域 */
    QTextEdit QScrollBar::add-page:vertical,
    QTextEdit QScrollBar::sub-page:vertical {
        background: transparent;
    }

    /* 水平滚动条 */
    QTextEdit QScrollBar:horizontal {
        background: #2a2a2a;
        height: 16px;
        border: none;
        margin: 0px;
    }

    /* 水平滚动条滑块 */
    QTextEdit QScrollBar::handle:horizontal {
        background: #555555;
        border: 1px solid #666666;
        border-radius: 2px;
        min-width: 10px;
        margin: 1px;
    }

    /* 水平滚动条滑块悬停 */
    QTextEdit QScrollBar::handle:horizontal:hover {
        background: #666666;
        border: 1px solid #777777;
    }

    /* 水平滚动条滑块按下 */
    QTextEdit QScrollBar::handle:horizontal:pressed {
        background: #777777;
        border: 1px solid #888888;
    }

    /* 隐藏箭头按钮 */
    QTextEdit QScrollBar::add-line:horizontal,
    QTextEdit QScrollBar::sub-line:horizontal {
        width: 0px;
    }

    /* 水平滚动条背景区域 */
    QTextEdit QScrollBar::add-page:horizontal,
    QTextEdit QScrollBar::sub-page:horizontal {
        background: transparent;
    }
)";
    // 应用合并后的样式到 textEdit
    ui->textEdit->setStyleSheet(systemStyleScrollbar);

    // UI样式设置
    ui->Open->setFlat(true);
    ui->Open->setIconSize(QSize(22, 22));
    ui->Open->setFixedSize(32, 32);

    QString buttonStyle = R"(
    QPushButton {
        background-color: transparent;
        border: none;
        padding: 6px;
        border-radius: 6px;
    }
    QPushButton:hover {
        background-color: rgba(100, 100, 100, 0.2);
    }
    QPushButton:pressed {
        background-color: rgba(100, 100, 100, 0.3);
        padding-left: 7px;
        padding-top: 7px;
        padding-right: 5px;
        padding-bottom: 5px;
    }
)";
    ui->Open->setStyleSheet(buttonStyle);
    ui->Save->setStyleSheet(buttonStyle);
    ui->Exit->setStyleSheet(buttonStyle);
    ui->Reset->setStyleSheet(buttonStyle);
    ui->Withdraw->setStyleSheet(buttonStyle);
    ui->btnOpenSet->setStyleSheet(buttonStyle);
    ui->Delete->setStyleSheet(buttonStyle);

    // 样式设置
    ui->fileArrangement->setStyleSheet(
        "QComboBox {"
        "    border: 2px solid #18e2cf;"
        "    border-radius: 16px;"
        "    background: #222831;"
        "    color: white;"
        "    padding: 6px 8px;"
        "    font: 10pt 'Microsoft YaHei UI';"
        "    min-height: 10px;"
        "    margin-bottom: 6px;"
        "}"
        "QComboBox::drop-down {"
        "    width: 0px;"
        "    border: none;"
        "}"
        "QComboBox::down-arrow {"
        "    image: none;"
        "    width: 0px; height: 0px;"
        "}"
        "QComboBox QAbstractItemView {"
        "    border: 2px solid #18e2cf;"
        "    border-radius: 8px;"
        "    background: #232931;"
        "    color: white;"
        "    font: 10pt 'Microsoft YaHei UI';"
        "    padding: 6px;"
        "    min-height: 36px;"
        "    selection-background-color: #3de2cf;"
        "    selection-color: #222831;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    padding: 8px 12px;"         // 稍微降低垂直内边距
        "    min-height: 26px;"          // 降低项目最小高度
        "    border: none;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "    background-color: rgba(24, 226, 207, 0.2);"  // 更柔和的选中背景色
        "    color: white;"              // 保持白色文字
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "    background-color: rgba(24, 226, 207, 0.1);"  // 更淡的悬停背景色
        "}"
        );

    ui->fileCombobox->setStyleSheet(
        "QComboBox {"
        "    border: 1px solid #18e2cf;"
        "    border-radius: 10px;"
        "    background: #222831;"
        "    color: white;"
        "    padding: 3px 6px;"
        "    font: 10pt 'Microsoft YaHei UI';"
        "    min-height: 8px;"
        "    min-width: 90px;"
        "} "
        "QComboBox::drop-down {"
        "    width: 0px;"
        "    border: none;"
        "} "
        "QComboBox::down-arrow {"
        "    image: none;"
        "    width: 0px; height: 0px;"
        "} "
        "QComboBox QAbstractItemView {"
        "    border: 1px solid #18e2cf;"
        "    border-radius: 6px;"
        "    background: #232931;"
        "    color: white;"
        "    font: 8pt 'Microsoft YaHei UI';"
        "    padding: 4px;"
        "    min-height: 50x;"
        "    selection-background-color: #3de2cf;"
        "    selection-color: #222831;"
        "}"
        );

    // === 自动保存配置初始化 ===
    asEnabled  = s.value(KEY_AE, false).toBool();
    asInterval = s.value(KEY_AT, 30).toInt();
    asPath     = s.value(KEY_AP).toString();

    // 如果用户没有设置路径，根据启用状态设置默认路径
    if (asPath.isEmpty()) {
        if (asEnabled) {
            // 启用自动保存时默认使用文档目录
            asPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            if (asPath.isEmpty()) {
                asPath = QDir::homePath();
            }
        } else {
            // 未启用时使用程序目录（用于备份功能）
            asPath = QCoreApplication::applicationDirPath();
            QDir testDir(asPath);
            if (!testDir.mkdir("test_write_permission")) {
                asPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            } else {
                testDir.rmdir("test_write_permission");
            }
        }
    }

    // 确保备份目录在程序启动时就创建
    QString backupDir = getBackupDirectory();
    if (!backupDir.isEmpty()) {
        QDir dir;
        if (!dir.exists(backupDir)) {
            if (dir.mkpath(backupDir)) {
                // 只在程序目录创建简单的说明文件
                if (backupDir.startsWith(QCoreApplication::applicationDirPath())) {
                    createProgramDirReadme(backupDir);
                }
            }
        } else {
            // 备份目录已存在，检查说明文件是否存在
            if (backupDir.startsWith(QCoreApplication::applicationDirPath())) {
                QString readmePath = QDir(backupDir).filePath(getReadmeFileName());
                if (!QFile::exists(readmePath)) {
                    // 说明文件不存在，重新创建
                    createProgramDirReadme(backupDir);
                }
            }
        }
    }

    // === 防抖定时器设置 ===
    debounceTimer.setSingleShot(true);
    debounceTimer.setInterval(1000); // 1秒防抖
    connect(&debounceTimer, &QTimer::timeout, this, &Widget::onAutoSaveTimeout);

    // === 文本变化检测 ===
    connect(ui->textEdit, &QTextEdit::textChanged, this, [this](){
        autoSaveDirty = true;

        // 标记文档已修改
        if (!ui->textEdit->document()->isModified()) {
            ui->textEdit->document()->setModified(true);
        }

        // 启动防抖定时器（无论是否启用自动保存）
        debounceTimer.start();

        // 立即更新窗口标题显示修改状态
        updateWindowTitle();
    });

    QStringList args = QCoreApplication::arguments();
    if (args.size() > 1) {
        // 有命令行参数，尝试打开文件
        QString filePath = args[1];
        QFileInfo fileInfo(filePath);

        // 检查文件是否存在且是支持的格式
        if (fileInfo.exists() && fileInfo.isFile()) {
            QString suffix = fileInfo.suffix().toLower();
            if (suffix.isEmpty() || supportedTextExtensions.contains(suffix)) {
                QTimer::singleShot(100, this, [this, filePath]() {
                    openFileFromPath(filePath);
                });
            }
        }
    }

    cleanupTimer.setSingleShot(false);
    cleanupTimer.setInterval(120000);
    connect(&cleanupTimer, &QTimer::timeout, this, &Widget::cleanupTemporaryFiles);
    cleanupTimer.start();

    QString defaultEncodingStr = "UTF-8";
    int idx = ui->fileCombobox->findText(defaultEncodingStr);
    if (idx >= 0) {
        ui->fileCombobox->setCurrentIndex(idx);
    }
    fileEncodings[""] = encodingFromString(defaultEncodingStr);

    QTimer::singleShot(5000, this, [this]() {
        updateAutosaveTimer();
        cleanupExcessiveBackups();
    });
    setupToolTips();
}

Widget::~Widget() {
    delete ui;
    if (mdHighlighter) delete mdHighlighter;
    if (scOpen) delete scOpen;
    if (scSave) delete scSave;
    if (scUndo) delete scUndo;
    if (scRedo) delete scRedo;
    if (scClear) delete scClear;
    if (scDeleteFile) delete scDeleteFile;
}

void Widget::cleanupMemoryIfNeeded()
{
    // 只有当内存使用过多时才清理
    if (openedFiles.size() <= 20) return;

    // 找出不在最近历史中的文件
    QStringList filesToRemove;
    for (auto it = openedFiles.begin(); it != openedFiles.end(); ++it) {
        QString path = it.key();

        // 保留当前文件
        if (path == currentFilePath) continue;

        // 保留最近15个文件
        if (fileHistory.contains(path) && fileHistory.indexOf(path) < 15) continue;

        // 保留fileOrder中的文件（用户明确打开过的）
        if (fileOrder.contains(path)) continue;

        filesToRemove.append(path);
    }

    // 执行清理
    for (const QString& path : filesToRemove) {
        openedFiles.remove(path);
        fileEncodings.remove(path);
        fileRawData.remove(path);
        fileHistory.removeAll(path);
    }

    // 调整fileHistoryPos
    if (fileHistoryPos >= fileHistory.size()) {
        fileHistoryPos = fileHistory.size() - 1;
    }
    if (!filesToRemove.isEmpty()) {
        updateFileArrangement();
    }
}

void Widget::openFileFromPath(const QString &path)
{
    if (path.isEmpty()) return;

    m_readmeEncodingWarningShown = false;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("错误"),
                             tr("无法打开文件：%1\n错误：%2")
                                 .arg(QFileInfo(path).fileName(), file.errorString()));
        return;
    }

    QByteArray raw = file.readAll();
    file.close();

    // 保存原始数据
    fileRawData[path] = raw;

    QStringConverter::Encoding encoding;

    // 优先使用持久化保存的编码记录
    QSettings s(ORG, APP);
    QString savedEncodingStr = s.value("FileEncoding/" + path).toString();

    if (!savedEncodingStr.isEmpty()) {
        encoding = encodingFromString(savedEncodingStr);
    } else if (fileEncodings.contains(path)) {
        // 内存中有编码记录
        encoding = fileEncodings[path];
    } else {
        // 都没有才进行自动检测
        encoding = detectEncodingImproved(raw);
    }

    int bom = 0;
    if (raw.startsWith("\xEF\xBB\xBF")) bom = 3;
    else if (raw.startsWith("\xFF\xFE") || raw.startsWith("\xFE\xFF")) bom = 2;

    QStringDecoder decoder(encoding);
    QString text = decoder(raw.mid(bom));

    // 在切换到新文件前，保存当前文件的备份
    if (!currentFilePath.isEmpty() && m_smartBackupEnabled && autoSaveDirty) {
        performSmartBackup(ui->textEdit->toPlainText());
    }

    // 切换文件前的备份（来自 on_Open_clicked 的逻辑）
    if (!currentFilePath.isEmpty() && m_smartBackupEnabled) {
        updateBackupIfChanged(currentFilePath, ui->textEdit->toPlainText());
    }
    if (!currentFilePath.isEmpty()) {
        openedFiles[currentFilePath] = ui->textEdit->toPlainText();
    }

    // 设置新文件
    currentFilePath = path;
    openedFiles[path] = text;
    fileEncodings[path] = encoding;  // 确保内存中保存正确的编码
    if (!fileOrder.contains(path)) fileOrder.append(path);

    // 为新打开的文件设置初始状态
    m_initialFileContent = text;
    m_hasInitialBackup = false;

    // 创建初始备份
    if (m_smartBackupEnabled) createInitialBackup(path, text);

    addToHistory(path);

    ui->textEdit->setPlainText(text);

    // 检查是否为说明文件，如果是则设置为只读
    QFileInfo fileInfo(path);
    if (isReadmeFile(fileInfo.fileName())) {
        ui->textEdit->setReadOnly(true);

        // 检查文件是否在备份目录中
        QString backupDir = getBackupDirectory();
        if (!backupDir.isEmpty() && fileInfo.absolutePath() == QDir(backupDir).absolutePath()) {
            // 是备份目录中的说明文件，显示特殊提示
            QMessageBox::information(this, tr("系统文件"),
                                     tr("已打开备份目录说明文件，仅供查看。\n\n"
                                        "文件名：%1\n\n"
                                        "此文件由程序自动生成和维护，不允许编辑或删除。\n"
                                        "如果此文件被意外删除，程序会自动重新创建。")
                                         .arg(fileInfo.fileName()));
        } else {
            QMessageBox::information(this, tr("只读文件"),
                                     tr("已打开系统说明文件，仅供查看。\n\n"
                                        "文件名：%1\n\n"
                                        "此文件不允许编辑、保存或更改编码格式。")
                                         .arg(fileInfo.fileName()));
        }
    } else {
        ui->textEdit->setReadOnly(false);
    }

    ui->textEdit->document()->setModified(false);

    // 使用忽略标志更新UI中的编码选择 - 防止触发activated事件
    m_ignoreEncodingChange = true;
    QString encStr = encodingToString(encoding);
    int idx = ui->fileCombobox->findText(encStr);
    if (idx >= 0) {
        ui->fileCombobox->setCurrentIndex(idx);
    } else {
        ui->fileCombobox->setCurrentText(encStr);
    }
    m_ignoreEncodingChange = false;

    updateFileArrangement();
    updateWindowTitle();
    updateUndoRedoButtons();
}

void Widget::updateEncodingSelection(const QString &path)
{
    auto enc = fileEncodings.value(path, DEFAULT_ENCODING);
    QString name = encodingToString(enc);
    int idx = ui->fileCombobox->findText(name);
    if (idx >= 0)
        ui->fileCombobox->setCurrentIndex(idx);
}

void Widget::addToHistory(const QString &path)
{
    if (path.isEmpty()) return;

    // 移除已存在的记录
    fileHistory.removeAll(path);
    // 添加到开头
    fileHistory.prepend(path);

    const int MAX_HISTORY = 15;
    while (fileHistory.size() > MAX_HISTORY) {
        QString removedPath = fileHistory.takeLast();
        // 只有当文件不在fileOrder中且不是当前文件时才从内存中移除
        if (!fileOrder.contains(removedPath) && removedPath != currentFilePath) {
            openedFiles.remove(removedPath);
            fileEncodings.remove(removedPath);
            fileRawData.remove(removedPath);
        }
    }

    // 重新设置位置为0
    fileHistoryPos = 0;

    cleanupMemoryIfNeeded();
}

bool Widget::isBinaryContent(const QByteArray &data) const
{
    if (data.isEmpty()) return false;

    int nullCount = 0;
    int controlCount = 0;
    int printableCount = 0;
    int totalBytes = qMin(4096, data.size()); // 检查前4KB

    for (int i = 0; i < totalBytes; i++) {
        unsigned char c = data[i];

        if (c == 0) {
            nullCount++;
        } else if (c < 32 && c != '\t' && c != '\n' && c != '\r') {
            controlCount++;
        } else if (c >= 32 && c <= 126) {
            printableCount++;
        }
    }

    // 如果包含太多空字节或控制字符，认为是二进制
    double nullRatio = (double)nullCount / totalBytes;
    double controlRatio = (double)controlCount / totalBytes;

    return (nullRatio > 0.05 || controlRatio > 0.3);
}

void Widget::on_Open_clicked()
{
    if (!checkUnsavedChanges()) {
        return;
    }

    static QString lastDir = QDir::homePath();

    static const QStringList supportedTextExtensions = {
        "txt", "md", "cpp", "h", "c", "hpp", "cc", "cxx",
        "py", "js", "html", "css", "json", "xml",
        "log", "ini", "cfg", "conf"
    };

    QString filter = tr("所有支持的文件 (*.txt *.md *.cpp *.h *.c *.hpp *.cc *.cxx *.py *.js *.html *.css *.json *.xml *.log *.ini *.cfg *.conf);;") +
                     tr("文本文件 (*.txt);;") +
                     tr("Markdown (*.md);;") +
                     tr("C/C++ 源码 (*.cpp *.h *.c *.hpp *.cc *.cxx);;") +
                     tr("Python (*.py);;") +
                     tr("Web 文件 (*.js *.html *.css);;") +
                     tr("配置文件 (*.json *.xml *.ini *.cfg *.conf);;") +
                     tr("日志文件 (*.log);;") +
                     tr("所有文件 (*.*)");

    QString path = QFileDialog::getOpenFileName(
        this,
        tr("打开文件"),
        lastDir,
        filter
        );

    if (path.isEmpty()) return;
    lastDir = QFileInfo(path).absolutePath();

    QFileInfo info(path);
    QString suffix = info.suffix().toLower();

    QStringList binaryExtensions = {
        "exe", "dll", "so", "dylib", "bin", "obj", "o",
        "jpg", "jpeg", "png", "gif", "bmp", "ico", "tiff",
        "mp3", "wav", "mp4", "avi", "mov", "pdf", "zip",
        "rar", "7z", "tar", "gz"
    };

    if (binaryExtensions.contains(suffix)) {
        QMessageBox::warning(this, tr("警告"),
                             tr("不能打开二进制文件：%1\n文件类型：%2")
                                 .arg(info.fileName(), suffix.toUpper()));
        return;
    }

    // 文件大小检查
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("错误"),
                             tr("无法打开文件：%1\n错误：%2")
                                 .arg(info.fileName(), file.errorString()));
        return;
    }

    qint64 fileSize = file.size();
    if (fileSize > 50 * 1024 * 1024) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("大文件警告"),
            tr("文件 %1 较大（%2 MB），打开可能需要一些时间。\n是否继续？")
                .arg(info.fileName())
                .arg(QString::number(fileSize / (1024.0 * 1024.0), 'f', 1)),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );
        if (reply != QMessageBox::Yes) {
            file.close();
            return;
        }
    }
    file.close();

    // 调用统一的文件打开函数
    openFileFromPath(path);
}

void Widget::on_Save_clicked()
{
    if (!currentFilePath.isEmpty()) {
        QFileInfo fileInfo(currentFilePath);
        if (isReadmeFile(fileInfo.fileName())) {
            QMessageBox::warning(this, tr("无法保存文件"),
                                 tr("此文件是系统生成的备份说明文件，不允许修改。\n\n"
                                    "文件名：%1\n\n"
                                    "如需修改说明文件语言，请在设置选择语言后重新打开文件。")
                                     .arg(fileInfo.fileName()));
            return;
        }
    }
    // 如果没有启用自动保存，先检查是否真的需要保存
    if (!asEnabled) {
        if (!ui->textEdit->document()->isModified()) {
            QString fileName = currentFilePath.isEmpty() ?
                                   tr("新文档") : QFileInfo(currentFilePath).fileName();
            QMessageBox::information(this, tr("提示"),
                                     tr("文件 \"%1\" 没有修改，无需保存。").arg(fileName));
            return;
        }

        // 对于已有文件，检查内容是否真的变化了
        if (!currentFilePath.isEmpty()) {
            QString currentContent = ui->textEdit->toPlainText();
            QString originalContent = openedFiles.value(currentFilePath, "");

            if (currentContent.trimmed() == originalContent.trimmed()) {
                QString fileName = QFileInfo(currentFilePath).fileName();
                QMessageBox::information(this, tr("提示"),
                                         tr("文件 \"%1\" 内容没有实际变化，无需保存。").arg(fileName));

                // 重置修改状态
                ui->textEdit->document()->setModified(false);
                updateWindowTitle();
                return;
            }
        }

        // 对于新文档，检查是否为空
        if (currentFilePath.isEmpty()) {
            QString content = ui->textEdit->toPlainText().trimmed();
            if (content.isEmpty()) {
                QMessageBox::information(this, tr("提示"),
                                         tr("新文档内容为空，无需保存。"));
                return;
            }
        }
    }

    QString path = currentFilePath;

    // 获取用户选择的编码
    QStringConverter::Encoding encoding = encodingFromString(ui->fileCombobox->currentText());

    if (path.isEmpty()) {
        // 根据Markdown状态选择默认文件名和过滤器
        QString defaultDir;
        QString defaultFileName;
        QString fileFilter;

        // 检查是否启用了Markdown
        QSettings s(ORG, APP);
        bool markdownOn = s.value(KEY_MD, false).toBool();

        if (markdownOn) {
            defaultFileName = tr("新建文件.md");
            fileFilter = tr("Markdown (*.md);;文本文件 (*.txt);;所有文件 (*.*);;C++ 源码 (*.cpp *.h)");
        } else {
            defaultFileName = tr("新建文件.txt");
            fileFilter = tr("文本文件 (*.txt);;所有文件 (*.*);;Markdown (*.md);;C++ 源码 (*.cpp *.h)");
        }

        if (asEnabled && !asPath.isEmpty()) {
            defaultDir = asPath;
        } else {
            defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            if (defaultDir.isEmpty()) {
                defaultDir = QDir::homePath();
            }
        }

        QString defaultPath = QDir(defaultDir).filePath(defaultFileName);

        path = QFileDialog::getSaveFileName(this,
                                            tr("保存文件"),
                                            defaultPath,
                                            fileFilter);
        if (path.isEmpty()) return;

        // 设置新文件路径并添加到历史
        currentFilePath = path;

        // 立即为新文件设置编码，使用之前获取的编码值
        fileEncodings[path] = encoding;

        if (!fileOrder.contains(path))
            fileOrder.append(path);
        addToHistory(path);
        updateFileArrangement();
    }

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("错误"), tr("无法写入文件！"));
        return;
    }
    fileEncodings[path] = encoding;

    // 根据编码写入BOM
    if (encoding == QStringConverter::Utf8) {
        f.write("\xEF\xBB\xBF");
    } else if (encoding == QStringConverter::Utf16LE) {
        f.write("\xFF\xFE");
    } else if (encoding == QStringConverter::Utf16BE) {
        f.write("\xFE\xFF");
    }

    QTextStream out(&f);
    out.setEncoding(encoding);
    QString text = ui->textEdit->toPlainText();
    out << text;
    f.close();

    // 更新内存中的内容
    openedFiles[path] = text;

    // 更新原始数据以保持同步
    QByteArray newRawData;
    if (encoding == QStringConverter::Utf8) {
        newRawData.append("\xEF\xBB\xBF");
    } else if (encoding == QStringConverter::Utf16LE) {
        newRawData.append("\xFF\xFE");
    } else if (encoding == QStringConverter::Utf16BE) {
        newRawData.append("\xFE\xFF");
    }
    QStringEncoder encoder(encoding);
    newRawData.append(encoder(text));
    fileRawData[path] = newRawData;

    ui->textEdit->document()->setModified(false);
    updateFileArrangement();
    updateWindowTitle();

    // 添加编码持久化
    QSettings s(ORG, APP);
    s.setValue("FileEncoding/" + path, ui->fileCombobox->currentText());

    // 删除临时保存或自动保存的备份文件
    QString backupPath = path + ".autosave";
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }

    // 为新文档创建初始备份
    if (m_smartBackupEnabled) {
        m_initialFileContent = text;
        m_hasInitialBackup = true;
        createInitialBackup(path, text);
    }

    QMessageBox::information(this, tr("提示"), tr("保存成功！"));
}

void Widget::on_Delete_clicked()
{
    if (currentFilePath.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("没有打开的文件可以删除。"));
        return;
    }

    QFileInfo fileInfo(currentFilePath);
    QString fileName = fileInfo.fileName();

    // 检查是否为说明文件
    if (isReadmeFile(fileName)) {
        QMessageBox::warning(this, tr("无法删除"),
                             tr("此文件是系统生成的备份说明文件，不允许删除。\n\n"
                                "文件名：%1\n\n"
                                "此文件用于说明备份文件夹的用途，请保留。")
                                 .arg(fileName));
        return;
    }

    QString fullPath = QDir::toNativeSeparators(fileInfo.absoluteFilePath());

    // 显示完整路径和文件大小信息
    QString fileSize = QString::number(fileInfo.size() / 1024.0, 'f', 1) + " KB";
    if (fileInfo.size() > 1024 * 1024) {
        fileSize = QString::number(fileInfo.size() / (1024.0 * 1024.0), 'f', 1) + " MB";
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("确认删除文件"),
        tr("确定要删除以下文件吗？\n\n"
           "文件名：%1\n"
           "位置：%2\n"
           "大小：%3\n"
           "修改时间：%4\n\n"
           "⚠️ 原文件将被移至回收站，此操作可以恢复。")
            .arg(fileName)
            .arg(fullPath)
            .arg(fileSize)
            .arg(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss")),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (reply != QMessageBox::Yes) {
        return;
    }

    // 尝试删除文件
    if (!QFile::moveToTrash(currentFilePath)) {
        QMessageBox::critical(this, tr("删除失败"),
                              tr("无法将文件移至回收站。\n\n"
                                 "可能的原因：\n"
                                 "• 文件正在被其他程序使用\n"
                                 "• 没有足够的权限\n"
                                 "• 磁盘空间不足\n\n"
                                 "文件路径：%1").arg(fullPath));
        return;
    }

    // 保存被删除的文件路径和文件名，用于后续提示
    QString deletedPath = currentFilePath;
    QString deletedFileName = fileName;

    // 清理所有相关数据
    openedFiles.remove(deletedPath);
    fileEncodings.remove(deletedPath);
    fileOrder.removeAll(deletedPath);
    fileRawData.remove(deletedPath);
    fileHistory.removeAll(deletedPath);

    // 清理备份文件
    QString backupPath = deletedPath + ".autosave";
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }

    // 重置历史位置
    if (fileHistory.isEmpty()) {
        fileHistoryPos = -1;
    } else {
        fileHistoryPos = 0;  // 重置到第一个历史文件
    }

    // 查找要切换到的文件
    QString nextPath;
    QString nextFileName;

    // 尝试从历史记录中找一个存在的文件
    for (const QString& path : fileHistory) {
        if (QFile::exists(path)) {
            nextPath = path;
            nextFileName = QFileInfo(path).fileName();
            break;
        }
    }

    // 如果历史记录中没有可用文件，从fileOrder中找
    if (nextPath.isEmpty()) {
        for (const QString& path : fileOrder) {
            if (QFile::exists(path) && path != deletedPath) {
                nextPath = path;
                nextFileName = QFileInfo(path).fileName();
                // 将这个文件添加到历史记录
                fileHistory.prepend(path);
                fileHistoryPos = 0;
                break;
            }
        }
    }

    // 如果找到了下一个文件
    if (!nextPath.isEmpty()) {
        // 如果文件在内存中，直接切换
        if (openedFiles.contains(nextPath)) {
            currentFilePath = nextPath;
            ui->textEdit->setPlainText(openedFiles[nextPath]);

            // 检查新文件的只读状态
            QFileInfo fileInfo(nextPath);
            if (isReadmeFile(fileInfo.fileName())) {
                ui->textEdit->setReadOnly(true);
            } else {
                ui->textEdit->setReadOnly(false);
            }

            // 更新编码选择
            updateEncodingSelection(nextPath);
            ui->textEdit->document()->setModified(false);
        } else {
            // 文件不在内存中，重新打开
            openFileFromPath(nextPath);
        }

        // 更新界面
        updateFileArrangement();
        updateWindowTitle();

        QMessageBox::information(this, tr("删除成功"),
                                 tr("文件 \"%1\" 已删除。\n\n已切换到：%2")
                                     .arg(deletedFileName)
                                     .arg(nextFileName));
    } else {
        // 没有其他文件可切换，完全清空界面
        ui->textEdit->clear();
        ui->textEdit->setReadOnly(false);
        currentFilePath.clear();
        fileHistory.clear();
        fileHistoryPos = -1;
        fileOrder.clear();
        openedFiles.clear();
        fileEncodings.clear();
        fileRawData.clear();

        // 重置编码为默认值
        ui->fileCombobox->setCurrentText("UTF-8");

        // 更新界面
        updateFileArrangement();
        updateWindowTitle();
        updateCharCount();
        updateCursorPosition();

        QMessageBox::information(this, tr("删除成功"),
                                 tr("文件 \"%1\" 已删除。\n\n"
                                    "如需恢复，可以：\n"
                                    "1. 从回收站恢复\n"
                                    "2. 从备份文件夹查找备份").arg(deletedFileName));
    }
}

void Widget::on_Exit_clicked()
{
    if (currentFilePath.isEmpty() && ui->textEdit->toPlainText().isEmpty()) {
        return;
    }

    if (!checkUnsavedChanges()) {
        return;
    }

    // 智能备份检查
    if (!currentFilePath.isEmpty() && m_smartBackupEnabled) {
        updateBackupIfChanged(currentFilePath, ui->textEdit->toPlainText());
    }

    // 关闭当前文件
    if (!currentFilePath.isEmpty()) {
        // 保存当前文件路径和文件名用于显示
        QString closedPath = currentFilePath;
        QString closedFileName = QFileInfo(closedPath).fileName();

        // 先保存当前位置
        int currentPos = fileHistory.indexOf(currentFilePath);

        // 从所有数据结构中移除当前文件
        openedFiles.remove(closedPath);
        fileEncodings.remove(closedPath);
        fileOrder.removeAll(closedPath);
        fileRawData.remove(closedPath);
        fileHistory.removeAll(closedPath);

        // 调整历史位置
        if (fileHistory.isEmpty()) {
            fileHistoryPos = -1;
        } else {
            // 优先选择原位置的下一个文件
            if (currentPos >= fileHistory.size()) {
                fileHistoryPos = fileHistory.size() - 1;
            } else if (currentPos > 0 && fileHistory.size() > 0) {
                fileHistoryPos = currentPos - 1;
            } else {
                fileHistoryPos = 0;
            }
        }

        // 尝试切换到历史中的其他文件
        if (!fileHistory.isEmpty() && fileHistoryPos >= 0 && fileHistoryPos < fileHistory.size()) {
            QString nextPath = fileHistory[fileHistoryPos];

            // 确保文件存在
            if (QFile::exists(nextPath)) {
                // 如果文件不在内存中，重新加载
                if (!openedFiles.contains(nextPath)) {
                    openFileFromPath(nextPath);
                    return;
                }

                // 文件在内存中，直接切换
                currentFilePath = nextPath;
                ui->textEdit->setPlainText(openedFiles[nextPath]);

                // 检查新文件的只读状态
                QFileInfo fileInfo(nextPath);
                if (isReadmeFile(fileInfo.fileName())) {
                    ui->textEdit->setReadOnly(true);
                } else {
                    ui->textEdit->setReadOnly(false);
                }

                updateEncodingSelection(nextPath);
                ui->textEdit->document()->setModified(false);
                updateFileArrangement();
                updateWindowTitle();

                // 已删除提示框：文件已经切换完成
                return;
            } else {
                // 文件不存在，从历史中移除
                fileHistory.removeAt(fileHistoryPos);
                if (fileHistoryPos >= fileHistory.size() && fileHistoryPos > 0) {
                    fileHistoryPos--;
                }
                // 继续尝试下一个文件
                if (!fileHistory.isEmpty() && fileHistoryPos >= 0) {
                    QString nextPath = fileHistory[fileHistoryPos];
                    if (QFile::exists(nextPath)) {
                        if (!openedFiles.contains(nextPath)) {
                            openFileFromPath(nextPath);
                            return;
                        }
                        currentFilePath = nextPath;
                        ui->textEdit->setPlainText(openedFiles[nextPath]);
                        QFileInfo fileInfo(nextPath);
                        if (isReadmeFile(fileInfo.fileName())) {
                            ui->textEdit->setReadOnly(true);
                        } else {
                            ui->textEdit->setReadOnly(false);
                        }
                        updateEncodingSelection(nextPath);
                        ui->textEdit->document()->setModified(false);
                        updateFileArrangement();
                        updateWindowTitle();
                        updateUndoRedoButtons();

                        return;
                    }
                }
            }
        }

        // ===== 单文件情况：先清空界面 =====
        // 没有其他文件可切换，先清空界面
        m_readmeEncodingWarningShown = false;
        ui->textEdit->clear();
        ui->textEdit->setReadOnly(false);
        currentFilePath.clear();
        fileHistory.clear();
        fileHistoryPos = -1;
        fileOrder.clear();
        openedFiles.clear();
        fileEncodings.clear();
        fileRawData.clear();

        // 重置编码为默认值
        ui->fileCombobox->setCurrentText("UTF-8");
        fileEncodings[""] = QStringConverter::Utf8;
        ui->textEdit->document()->setModified(false);

        // 更新界面
        updateFileArrangement();
        updateWindowTitle();
        updateCharCount();
        updateCursorPosition();

    } else {
        // 新文档的情况，直接清空
        m_readmeEncodingWarningShown = false;
        ui->textEdit->clear();
        ui->textEdit->setReadOnly(false);
        currentFilePath.clear();
        fileHistory.clear();
        fileHistoryPos = -1;
        ui->fileCombobox->setCurrentText("UTF-8");
        fileEncodings[""] = QStringConverter::Utf8;
        ui->textEdit->document()->setModified(false);

        updateFileArrangement();
        updateWindowTitle();
        updateCharCount();
        updateCursorPosition();
        updateUndoRedoButtons();
    }
}

void Widget::on_fileCombobox_currentTextChanged(const QString &text)
{
    // 如果是说明文件，不记录编码变化
    if (!currentFilePath.isEmpty()) {
        QFileInfo fileInfo(currentFilePath);
        if (isReadmeFile(fileInfo.fileName())) {
            return; // 说明文件不处理编码变化
        }
    }

    // 对于新文档（currentFilePath为空），也要记录编码选择
    if (currentFilePath.isEmpty()) {
        fileEncodings[""] = encodingFromString(text);
    }
}

void Widget::updateWindowTitle()
{
    QString title = "LTNoteBook";

    if (!currentFilePath.isEmpty()) {
        QString fileName = QFileInfo(currentFilePath).fileName();
        QString filePath = QDir::toNativeSeparators(QFileInfo(currentFilePath).absolutePath());
        bool isModified = ui->textEdit->document()->isModified();

        // 检查是否为说明文件
        if (isReadmeFile(fileName)) {
            title = QString("%1 [只读] - %2")
                        .arg(fileName)
                        .arg("LTNoteBook");
            setToolTip(tr("系统生成的说明文件（只读）\n文件路径：%1").arg(filePath));
            setWindowTitle(title);
            return;
        }

        // 显示更详细的状态信息
        title = QString("%1%2 - %3")
                    .arg(fileName)
                    .arg(isModified ? " •" : "")
                    .arg("LTNoteBook");

        QString encoding = ui->fileCombobox->currentText();

        // 显示文件大小和行数信息
        QFileInfo fileInfo(currentFilePath);
        if (fileInfo.exists()) {
            QString fileSize;
            qint64 size = fileInfo.size();
            if (size > 1024 * 1024) {
                fileSize = QString::number(size / (1024.0 * 1024.0), 'f', 1) + "MB";
            } else if (size > 1024) {
                fileSize = QString::number(size / 1024.0, 'f', 1) + "KB";
            } else {
                fileSize = QString::number(size) + "B";
            }

            int lineCount = ui->textEdit->document()->blockCount();
            title += QString(" [%1, %2, %3L]").arg(encoding).arg(fileSize).arg(lineCount);
        } else {
            title += QString(" [%1]").arg(encoding);
        }

        // 显示功能状态
        QStringList features;
        if (m_smartBackupEnabled) {
            features.append(tr("智能备份"));
        }
        if (asEnabled) {
            features.append(tr("自动保存"));
        }

        if (!features.isEmpty()) {
            title += " [" + features.join(", ") + "]";
        }

        // 在工具提示中显示完整路径
        setToolTip(tr("文件路径：%1").arg(filePath));

    } else if (!ui->textEdit->toPlainText().isEmpty()) {
        // 【修改】新文档也显示完整功能状态
        QStringList features;
        if (m_smartBackupEnabled) {
            features.append(tr("智能备份"));
        }
        if (asEnabled) {
            features.append(tr("自动保存"));
        }

        if (!features.isEmpty()) {
            title = tr("新文档 [%1] - ").arg(features.join(", ")) + title;
        } else {
            title = tr("新文档 * - ") + title;
        }
        setToolTip(tr("新建文档"));
    } else {
        setToolTip("");
    }

    setWindowTitle(title);
}

QStringConverter::Encoding Widget::detectEncodingImproved(const QByteArray &data) {
    // 检查BOM - 这些是最可靠的
    if (data.startsWith("\xEF\xBB\xBF")) return QStringConverter::Utf8;
    if (data.startsWith("\xFF\xFE")) return QStringConverter::Utf16LE;
    if (data.startsWith("\xFE\xFF")) return QStringConverter::Utf16BE;

    // 空文件或太小的文件直接返回UTF-8
    if (data.size() < 4) return QStringConverter::Utf8;

    // 取样本进行检测
    QByteArray sample = data.left(qMin(4096, data.size())); // 增加样本大小

    // ASCII检查
    bool isAscii = true;
    for (unsigned char c : sample) {
        if (c > 127) {
            isAscii = false;
            break;
        }
    }
    if (isAscii) return QStringConverter::Utf8;

    // UTF-8有效性检查 - 更严格的检测
    QStringDecoder utf8Decoder(QStringConverter::Utf8);
    QString utf8Text = utf8Decoder(sample);
    bool validUtf8 = !utf8Text.contains(QChar::ReplacementCharacter);

    // 检查UTF-8序列的有效性
    bool utf8SequenceValid = true;
    for (int i = 0; i < sample.size(); i++) {
        unsigned char c = sample[i];
        if (c >= 0x80) {  // 非ASCII字符
            int seqLen = 0;
            if ((c & 0xE0) == 0xC0) seqLen = 2;      // 110xxxxx
            else if ((c & 0xF0) == 0xE0) seqLen = 3; // 1110xxxx
            else if ((c & 0xF8) == 0xF0) seqLen = 4; // 11110xxx
            else { utf8SequenceValid = false; break; }

            // 检查后续字节
            for (int j = 1; j < seqLen && i + j < sample.size(); j++) {
                if ((sample[i + j] & 0xC0) != 0x80) { // 10xxxxxx
                    utf8SequenceValid = false;
                    break;
                }
            }
            if (!utf8SequenceValid) break;
            i += seqLen - 1;
        }
    }

    if (validUtf8 && utf8SequenceValid) {
        // 进一步检查是否是真正的文本
        int printableCount = 0;
        int controlCount = 0;
        for (const QChar &c : utf8Text) {
            if (c.isPrint() || c.isSpace()) printableCount++;
            else if (c.category() == QChar::Other_Control) controlCount++;
        }

        // 如果可打印字符比例高且控制字符少，认为是UTF-8
        if (printableCount > utf8Text.length() * 0.85 &&
            controlCount < utf8Text.length() * 0.05) {
            return QStringConverter::Utf8;
        }
    }

    // 检查可能的GBK/GB2312编码
    int possibleGbkPairs = 0;
    int totalBytes = 0;
    for (int i = 0; i < sample.size() - 1; i++) {
        unsigned char c1 = sample[i];
        unsigned char c2 = sample[i + 1];

        // GBK第一字节范围：0x81-0xFE
        // GBK第二字节范围：0x40-0x7E, 0x80-0xFE
        if (c1 >= 0x81 && c1 <= 0xFE) {
            if ((c2 >= 0x40 && c2 <= 0x7E) || (c2 >= 0x80 && c2 <= 0xFE)) {
                possibleGbkPairs++;
                i++; // 跳过第二个字节
                totalBytes += 2;
            } else {
                totalBytes += 1;
            }
        } else if (c1 >= 0x80) {
            totalBytes += 1;
        }
    }

    // 如果可能的GBK字符对比例较高，且通过系统编码测试
    if (possibleGbkPairs > 0 && totalBytes > 0) {
        double gbkRatio = (double)(possibleGbkPairs * 2) / totalBytes;
        if (gbkRatio > 0.5) {
            // 尝试用系统编码解码测试
            QStringDecoder systemDecoder(QStringConverter::System);
            QString systemText = systemDecoder(sample);
            if (!systemText.contains(QChar::ReplacementCharacter)) {
                return QStringConverter::System;
            }
        }
    }

    // 默认返回UTF-8
    return QStringConverter::Utf8;
}

void Widget::on_fileArrangement_activated(int index)
{
    if (m_ignoreFileArrangement) return;

    // 获取真实的历史索引
    QVariant data = ui->fileArrangement->itemData(index);
    if (!data.isValid()) return;

    int historyIndex = data.toInt();
    if (historyIndex < 0) {
        // 这是"新文档"或"无历史记录"项，不做处理
        return;
    }

    if (historyIndex >= fileHistory.size()) return;

    m_readmeEncodingWarningShown = false;

    QString path = fileHistory[historyIndex];

    // 如果选择的是当前文件，不需要切换
    if (path == currentFilePath) {
        return;
    }

    // 检查文件是否存在
    if (!QFile::exists(path)) {
        QMessageBox::warning(this, tr("文件不存在"),
                             tr("文件已被删除或移动。\n%1").arg(path));

        // 从历史中移除不存在的文件
        fileHistory.removeAt(historyIndex);
        openedFiles.remove(path);
        fileEncodings.remove(path);
        fileRawData.remove(path);
        fileOrder.removeAll(path);

        // 重新调整历史位置
        if (fileHistoryPos > historyIndex) {
            fileHistoryPos--;
        }
        if (fileHistoryPos >= fileHistory.size()) {
            fileHistoryPos = fileHistory.size() - 1;
        }

        updateFileArrangement();
        return;
    }

    // 检查当前是否有未保存的修改
    if (!currentFilePath.isEmpty() && ui->textEdit->document()->isModified()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("未保存的修改"),
            tr("当前文件有未保存的修改，是否保存？"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save
            );

        if (reply == QMessageBox::Cancel) {
            // 恢复下拉框选择
            updateFileArrangement();
            return;
        } else if (reply == QMessageBox::Save) {
            on_Save_clicked();
            if (ui->textEdit->document()->isModified()) {
                // 保存失败，恢复选择
                updateFileArrangement();
                return;
            }
        }
    }

    // 在切换前保存当前文件内容（如果有）
    if (!currentFilePath.isEmpty()) {
        openedFiles[currentFilePath] = ui->textEdit->toPlainText();

        // 智能备份
        if (m_smartBackupEnabled) {
            updateBackupIfChanged(currentFilePath, ui->textEdit->toPlainText());
        }
    }

    // 检查文件是否在内存中
    if (!openedFiles.contains(path)) {
        // 文件不在内存中，重新读取
        openFileFromPath(path);
        return;
    }

    // 切换到选中的文件
    fileHistoryPos = historyIndex;
    currentFilePath = path;

    // 为切换到的文件创建初始备份
    if (m_smartBackupEnabled) {
        createInitialBackup(path, openedFiles[path]);
    }

    // 更新界面
    ui->textEdit->setPlainText(openedFiles[path]);

    // 检查是否为说明文件，设置只读状态
    QFileInfo fileInfo(path);
    if (isReadmeFile(fileInfo.fileName())) {
        ui->textEdit->setReadOnly(true);
    } else {
        ui->textEdit->setReadOnly(false);
    }

    // 更新编码选择
    QStringConverter::Encoding fileEncoding = fileEncodings.value(path, DEFAULT_ENCODING);
    QString encodingStr = encodingToString(fileEncoding);

    // 检查持久化记录
    QSettings s(ORG, APP);
    QString savedEncodingStr = s.value("FileEncoding/" + path).toString();
    if (!savedEncodingStr.isEmpty()) {
        encodingStr = savedEncodingStr;
        fileEncoding = encodingFromString(savedEncodingStr);
        fileEncodings[path] = fileEncoding;
    }

    // 更新UI编码选择
    m_ignoreEncodingChange = true;
    int idx = ui->fileCombobox->findText(encodingStr);
    if (idx >= 0) {
        ui->fileCombobox->setCurrentIndex(idx);
    } else {
        ui->fileCombobox->setCurrentText(encodingStr);
    }
    m_ignoreEncodingChange = false;

    ui->textEdit->document()->setModified(false);

    updateFileArrangement();
    updateWindowTitle();
    updateUndoRedoButtons();
}

void Widget::on_fileCombobox_activated(int idx)
{
    if (currentFilePath.isEmpty()) return;
    if (m_ignoreEncodingChange) return;

    // 检查当前文件是否为说明文件
    QFileInfo fileInfo(currentFilePath);
    if (isReadmeFile(fileInfo.fileName())) {
        // 设置忽略标志，防止递归调用
        m_ignoreEncodingChange = true;

        // 恢复到UTF-8编码显示
        int utf8Idx = ui->fileCombobox->findText("UTF-8");
        if (utf8Idx >= 0) {
            ui->fileCombobox->setCurrentIndex(utf8Idx);
        }

        // 重置忽略标志
        m_ignoreEncodingChange = false;

        // 只在没有显示过警告时才显示
        if (!m_readmeEncodingWarningShown) {
            m_readmeEncodingWarningShown = true;

            QMessageBox::warning(this, tr("无法更改编码"),
                                 tr("此文件是系统生成的备份说明文件，不允许更改编码格式。\n\n"
                                    "文件名：%1\n\n"
                                    "系统文件将始终使用 UTF-8 编码。")
                                     .arg(fileInfo.fileName()));
        }

        return;
    }

    auto currentEncoding = fileEncodings.value(currentFilePath, DEFAULT_ENCODING);
    auto newEncoding = encodingFromString(ui->fileCombobox->itemText(idx));

    if (newEncoding == currentEncoding) return;

    // 如果有未保存修改，给用户选择
    if (ui->textEdit->document()->isModified()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("切换编码"),
            tr("切换编码需要重新加载文件，是否保存当前修改？"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save
            );

        if (reply == QMessageBox::Cancel) {
            // 恢复原编码选择
            m_ignoreEncodingChange = true;
            QString oldEncStr = encodingToString(currentEncoding);
            int oldIdx = ui->fileCombobox->findText(oldEncStr);
            if (oldIdx >= 0) ui->fileCombobox->setCurrentIndex(oldIdx);
            m_ignoreEncodingChange = false;
            return;
        }
        else if (reply == QMessageBox::Save) {
            on_Save_clicked();
            if (ui->textEdit->document()->isModified()) {
                // 保存失败，恢复编码
                m_ignoreEncodingChange = true;
                QString oldEncStr = encodingToString(currentEncoding);
                int oldIdx = ui->fileCombobox->findText(oldEncStr);
                if (oldIdx >= 0) ui->fileCombobox->setCurrentIndex(oldIdx);
                m_ignoreEncodingChange = false;
                return;
            }
        }
    }

    // 使用保存的原始数据重新解码，而不是重新读取文件
    QByteArray raw = fileRawData.value(currentFilePath);
    if (raw.isEmpty()) {
        // 如果没有原始数据，重新读取文件
        QFile f(currentFilePath);
        if (!f.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, tr("错误"), tr("无法重新读取文件！"));
            return;
        }
        raw = f.readAll();
        f.close();
        fileRawData[currentFilePath] = raw; // 保存原始数据
    }

    int bom = 0;
    if (newEncoding == QStringConverter::Utf8 && raw.startsWith("\xEF\xBB\xBF")) bom = 3;
    else if (newEncoding == QStringConverter::Utf16LE && raw.startsWith("\xFF\xFE")) bom = 2;
    else if (newEncoding == QStringConverter::Utf16BE && raw.startsWith("\xFE\xFF")) bom = 2;

    QStringDecoder decoder(newEncoding);
    QString txt = decoder(raw.mid(bom));

    // 更新数据
    fileEncodings[currentFilePath] = newEncoding;
    openedFiles[currentFilePath] = txt;
    ui->textEdit->setPlainText(txt);
    ui->textEdit->document()->setModified(false);

    if (!currentFilePath.isEmpty()) {
        QSettings s(ORG, APP);
        QString newEncodingStr = encodingToString(newEncoding);
        s.setValue("FileEncoding/" + currentFilePath, newEncodingStr);
    }
    updateWindowTitle();
    updateUndoRedoButtons();
}

void Widget::updateFileArrangement()
{
    m_ignoreFileArrangement = true;
    ui->fileArrangement->clear();

    // 添加历史记录中的文件
    for (int i = 0; i < fileHistory.size(); ++i) {
        QString path = fileHistory[i];
        QString name = QFileInfo(path).fileName();

        // 检查文件是否存在
        if (!QFile::exists(path)) {
            name += tr(" (已删除)");
        }

        // 标记当前打开的文件
        if (path == currentFilePath) {
            // 如果是当前文件且有修改，添加星号
            if (ui->textEdit->document()->isModified()) {
                name += " *";
            }
            name = "▶ " + name;  // 添加一个指示符表示当前文件
        }

        ui->fileArrangement->addItem(tr("历史：%1").arg(name), QVariant(i));
    }

    // 如果当前没有打开文件但有历史记录，显示提示
    if (currentFilePath.isEmpty() && !fileHistory.isEmpty()) {
        ui->fileArrangement->insertItem(0, tr("-------    新文档 (无文件)    -------"), QVariant(-1));
        ui->fileArrangement->setCurrentIndex(0);
    } else if (fileHistoryPos >= 0 && fileHistoryPos < fileHistory.size()) {
        // 如果有当前文件，选中对应的历史记录
        ui->fileArrangement->setCurrentIndex(fileHistoryPos);
    } else if (currentFilePath.isEmpty()) {
        // 完全没有文件的情况
        ui->fileArrangement->addItem(tr("--------    无历史记录    --------"), QVariant(-1));
        ui->fileArrangement->setCurrentIndex(0);
    }

    m_ignoreFileArrangement = false;
}

void Widget::on_fontBox_currentFontChanged(const QFont &f)
{
    QFont cur = ui->textEdit->font();
    cur.setFamily(f.family());
    ui->textEdit->setFont(cur);
}

void Widget::updateCharCount()
{
    QString text = ui->textEdit->toPlainText();

    // 移除换行符和回车符，只计算可见字符
    QString visibleText = text;
    visibleText.remove('\n');
    visibleText.remove('\r');

    int charCount = visibleText.length();
    ui->Word->setText(tr("共 %1 个字符").arg(charCount));
    updateWindowTitle();
}

void Widget::updateCursorPosition()
{
    QTextCursor c = ui->textEdit->textCursor();
    ui->Position->setText(tr("行 %1, 列 %2").arg(c.blockNumber() + 1).arg(c.positionInBlock() + 1));
}

void Widget::updateUndoRedoButtons()
{
    ui->Reset->setEnabled(ui->textEdit->document()->isUndoAvailable());
    ui->Withdraw->setEnabled(ui->textEdit->document()->isRedoAvailable());
}

void Widget::on_Reset_clicked()
{
    if (ui->textEdit->document()->isUndoAvailable())
        ui->textEdit->undo();
}

void Widget::on_Withdraw_clicked(){
    if (ui->textEdit->document()->isRedoAvailable())
        ui->textEdit->redo();
}

void Widget::clearCurrentFile()
{
    // 如果当前有文件，直接调用 Exit 逻辑
    if (!currentFilePath.isEmpty()) {
        on_Exit_clicked();
        return;
    }

    // 新文档的情况：仅清空文本内容
    if (ui->textEdit->toPlainText().trimmed().isEmpty()) {
        return; // 如果文本已经为空，无需操作
    }

    // 检查未保存的修改
    if (!checkUnsavedChanges()) {
        return;
    }

    // 清空文本内容
    ui->textEdit->clear();
    ui->textEdit->document()->setModified(false);

    // 重置编码为默认值
    ui->fileCombobox->setCurrentText("UTF-8");
    fileEncodings[""] = QStringConverter::Utf8;

    // 更新界面
    updateWindowTitle();
    updateCharCount();
    updateCursorPosition();
    updateUndoRedoButtons();
}

void Widget::deleteCurrentFileByShortcut()
{
    // 检查当前是否真的有文件打开
    if (currentFilePath.isEmpty()) {
        // 双重检查：如果文本框不为空但没有文件路径，可能是新文档
        if (!ui->textEdit->toPlainText().isEmpty()) {
            QMessageBox::information(this, tr("提示"),
                                     tr("当前是新文档，请先保存后再删除。"));
        } else {
            QMessageBox::information(this, tr("提示"),
                                     tr("没有打开的文件可以删除。"));
        }
        return;
    }

    // 确保文件路径对应的文件确实存在
    if (!QFile::exists(currentFilePath)) {
        // 文件已经不存在了，清理相关状态
        QMessageBox::warning(this, tr("文件不存在"),
                             tr("文件已被删除或移动：\n%1").arg(currentFilePath));

        // 清理状态
        QString deletedPath = currentFilePath;
        openedFiles.remove(deletedPath);
        fileEncodings.remove(deletedPath);
        fileOrder.removeAll(deletedPath);
        fileRawData.remove(deletedPath);
        fileHistory.removeAll(deletedPath);

        // 重置界面
        currentFilePath.clear();
        ui->textEdit->clear();
        ui->textEdit->setReadOnly(false);

        if (fileHistory.isEmpty()) {
            fileHistoryPos = -1;
        } else {
            fileHistoryPos = 0;
        }

        updateFileArrangement();
        updateWindowTitle();
        updateCharCount();
        updateCursorPosition();

        return;
    }

    // 检查是否为说明文件
    QFileInfo fileInfo(currentFilePath);
    QString fileName = fileInfo.fileName();

    if (isReadmeFile(fileName)) {
        QMessageBox::warning(this, tr("无法删除"),
                             tr("此文件是系统生成的备份说明文件，不允许删除。\n\n"
                                "文件名：%1\n\n"
                                "此文件用于说明备份文件夹的用途，请保留。")
                                 .arg(fileName));
        return;
    }

    // 直接调用现有的删除功能
    on_Delete_clicked();
}

void Widget::loadShortcuts()
{
    QSettings settings(ORG, APP);

    // 清理现有快捷键
    if (scOpen) {
        disconnect(scOpen, nullptr, nullptr, nullptr);
        delete scOpen;
    }
    if (scSave) {
        disconnect(scSave, nullptr, nullptr, nullptr);
        delete scSave;
    }
    if (scUndo) {
        disconnect(scUndo, nullptr, nullptr, nullptr);
        delete scUndo;
    }
    if (scRedo) {
        disconnect(scRedo, nullptr, nullptr, nullptr);
        delete scRedo;
    }
    // 新增：清理新的快捷键
    if (scClear) {
        disconnect(scClear, nullptr, nullptr, nullptr);
        delete scClear;
    }
    if (scDeleteFile) {
        disconnect(scDeleteFile, nullptr, nullptr, nullptr);
        delete scDeleteFile;
    }

    // 重新初始化为nullptr
    scOpen = nullptr;
    scSave = nullptr;
    scUndo = nullptr;
    scRedo = nullptr;
    scClear = nullptr;        // 新增
    scDeleteFile = nullptr;   // 新增

    QString undoShortcut = settings.value("Shortcut/Undo", "Ctrl+Z").toString();
    QString redoShortcut = settings.value("Shortcut/Redo", "Ctrl+Y").toString();

    // 现有快捷键
    scOpen = new QShortcut(settings.value("Shortcut/Open", "Ctrl+O").value<QKeySequence>(), this);
    connect(scOpen, &QShortcut::activated, this, &Widget::on_Open_clicked);

    scSave = new QShortcut(settings.value("Shortcut/Save", "Ctrl+S").value<QKeySequence>(), this);
    connect(scSave, &QShortcut::activated, this, &Widget::on_Save_clicked);

    scUndo = new QShortcut(QKeySequence(undoShortcut), this);
    connect(scUndo, &QShortcut::activated, ui->textEdit, &QTextEdit::undo);

    scRedo = new QShortcut(QKeySequence(redoShortcut), this);
    connect(scRedo, &QShortcut::activated, ui->textEdit, &QTextEdit::redo);

    scClear = new QShortcut(settings.value("Shortcut/Clear", "Ctrl+N").value<QKeySequence>(), this);
    connect(scClear, &QShortcut::activated, this, &Widget::clearCurrentFile);

    scDeleteFile = new QShortcut(settings.value("Shortcut/DeleteFile", "Ctrl+D").value<QKeySequence>(), this);
    connect(scDeleteFile, &QShortcut::activated, this, &Widget::deleteCurrentFileByShortcut);

    ui->textEdit->setShortcutEnabled(QKeySequence::Undo, false);
    ui->textEdit->setShortcutEnabled(QKeySequence::Redo, false);
}

void Widget::resetToDefaultShortcuts()
{
    QSettings settings(ORG, APP);

    if (settings.value("Shortcut/Undo") != "Ctrl+Z") {
        settings.setValue("Shortcut/Undo", "Ctrl+Z");
    }
    if (settings.value("Shortcut/Redo") != "Ctrl+Y") {
        settings.setValue("Shortcut/Redo", "Ctrl+Y");
    }
    if (settings.value("Shortcut/Clear") != "Ctrl+N") {
        settings.setValue("Shortcut/Clear", "Ctrl+N");
    }
    if (settings.value("Shortcut/DeleteFile") != "Ctrl+D") {
        settings.setValue("Shortcut/DeleteFile", "Ctrl+D");
    }

    loadShortcuts();

    ui->textEdit->setShortcutEnabled(QKeySequence::Undo, true);
    ui->textEdit->setShortcutEnabled(QKeySequence::Redo, true);

    removeEventFilter(this);
}

bool Widget::isUsingCustomShortcut(int key) {
    QSettings settings(ORG, APP);

    if (key == Qt::Key_Z && settings.value("Shortcut/Undo") != "Ctrl+Z") {
        return true;
    }
    if (key == Qt::Key_Y && settings.value("Shortcut/Redo") != "Ctrl+Y") {
        return true;
    }
    if (key == Qt::Key_N && settings.value("Shortcut/Clear") != "Ctrl+N") {
        return true;
    }
    if (key == Qt::Key_D && settings.value("Shortcut/DeleteFile") != "Ctrl+D") {
        return true;
    }

    return false;
}

bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->textEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        QSettings settings(ORG, APP);
        QString undoShortcut = settings.value("Shortcut/Undo", "Ctrl+Z").toString();
        QString redoShortcut = settings.value("Shortcut/Redo", "Ctrl+Y").toString();
        QString clearShortcut = settings.value("Shortcut/Clear", "Ctrl+N").toString();
        QString deleteShortcut = settings.value("Shortcut/DeleteFile", "Ctrl+D").toString();

        if (keyEvent->modifiers() == Qt::ControlModifier) {
            if (keyEvent->key() == Qt::Key_Z && undoShortcut != "Ctrl+Z") {
                event->accept();
                return true;
            }
            if (keyEvent->key() == Qt::Key_Y && redoShortcut != "Ctrl+Y") {
                event->accept();
                return true;
            }
            if (keyEvent->key() == Qt::Key_N && clearShortcut != "Ctrl+N") {
                event->accept();
                return true;
            }
            if (keyEvent->key() == Qt::Key_D && deleteShortcut != "Ctrl+D") {
                event->accept();
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void Widget::cleanupTemporaryFiles()
{
    if (currentFilePath.isEmpty()) return;

    QFileInfo fileInfo(currentFilePath);
    QDir dir = fileInfo.absoluteDir();
    QString baseName = fileInfo.fileName();

    // 清理可能的临时文件
    QStringList tempPatterns = {
        baseName + ".tmp",
        baseName + ".saving",
        baseName + ".bak",
        baseName + ".temp"
    };

    for (const QString &pattern : tempPatterns) {
        QString tempPath = dir.filePath(pattern);
        if (QFile::exists(tempPath)) {
            QFile::remove(tempPath);
        }
    }
}

void Widget::cleanupExcessiveBackups() const
{
    QString backupDir = getBackupDirectory();
    if (backupDir.isEmpty()) return;

    QDir dir(backupDir);
    if (!dir.exists()) return;

    // 获取所有备份文件，包括新文档备份和普通文件备份
    QStringList nameFilters;
    nameFilters << "*.backup.*"      // 普通文件备份：filename.backup.ext
                << "*_备份_*.*"      // 新文档备份：新文档_备份_timestamp.ext
                << "*_自动保存_*.*"; // 自动保存的新文档

    QFileInfoList allBackups = dir.entryInfoList(nameFilters, QDir::Files, QDir::Time | QDir::Reversed);

    // 过滤掉说明文件，只保留真正的备份文件
    QFileInfoList validBackups;
    for (const QFileInfo &fileInfo : allBackups) {
        if (!isReadmeFile(fileInfo.fileName())) {
            validBackups.append(fileInfo);
        }
    }

    // 设置最大备份文件数量为50
    const int MAX_BACKUP_COUNT = 50;

    if (validBackups.size() > MAX_BACKUP_COUNT) {
        int toDelete = validBackups.size() - MAX_BACKUP_COUNT;

        // 删除最旧的备份文件（列表已按时间逆序排列，最旧的在末尾）
        int deletedCount = 0;
        for (int i = validBackups.size() - toDelete; i < validBackups.size(); ++i) {
            const QFileInfo &backup = validBackups[i];
            QString backupPath = backup.absoluteFilePath();

            if (QFile::remove(backupPath)) {
                deletedCount++;
            }
        }
    }
}

void Widget::saveToOriginalFile(const QString &content)
{
    if (currentFilePath.isEmpty()) return;

    // 检查编码是否变化，如果变化了给用户提示
    QStringConverter::Encoding currentUIEncoding = encodingFromString(ui->fileCombobox->currentText());
    QStringConverter::Encoding storedEncoding = fileEncodings.value(currentFilePath, DEFAULT_ENCODING);

    bool encodingChanged = (currentUIEncoding != storedEncoding);

    // 使用更简洁的临时文件名，避免生成多个临时文件
    QString tempPath = currentFilePath + ".saving";

    // 确保删除可能存在的旧临时文件
    if (QFile::exists(tempPath)) {
        QFile::remove(tempPath);
    }

    QFile f(tempPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QString errorMsg = tr("自动保存失败：%1\n原因：%2")
                               .arg(QFileInfo(currentFilePath).fileName())
                               .arg(f.errorString());
        showAutoSaveStatus(false, QFileInfo(currentFilePath).fileName());
        qWarning() << errorMsg;
        return;
    }

    // 使用UI中当前选择的编码
    QStringConverter::Encoding encoding = currentUIEncoding;

    // 只在编码真的不同时才更新
    if (encoding != storedEncoding) {
        fileEncodings[currentFilePath] = encoding;
    }

    // 持久化保存编码选择
    QSettings s(ORG, APP);
    QString currentSavedEncoding = s.value("FileEncoding/" + currentFilePath).toString();
    QString newEncodingStr = encodingToString(encoding);

    if (currentSavedEncoding != newEncodingStr) {
        s.setValue("FileEncoding/" + currentFilePath, newEncodingStr);
    }

    // 写入 BOM（仅针对 Unicode 编码）
    if (encoding == QStringConverter::Utf8) {
        f.write("\xEF\xBB\xBF");
    } else if (encoding == QStringConverter::Utf16LE) {
        f.write("\xFF\xFE");
    } else if (encoding == QStringConverter::Utf16BE) {
        f.write("\xFE\xFF");
    }

    // 用 QTextStream 按指定编码写入
    QTextStream out(&f);
    out.setEncoding(encoding);
    out << content;
    f.close();

    // 成功写入临时文件后，安全地替换原文件，并立即清理临时文件
    if (QFile::exists(tempPath)) {
        // 直接替换，不创建多余的备份文件
        bool success = false;

#ifdef Q_OS_WIN
        // Windows 下需要先删除目标文件
        if (QFile::exists(currentFilePath)) {
            // 设置文件为可写
            SetFileAttributesW(reinterpret_cast<const wchar_t*>(currentFilePath.utf16()),
                               FILE_ATTRIBUTE_NORMAL);
            QFile::remove(currentFilePath);
        }
        success = QFile::rename(tempPath, currentFilePath);
#else
        // Unix 系统可以直接覆盖
        success = QFile::rename(tempPath, currentFilePath);
#endif

        if (success) {
            // 更新内存中的内容
            openedFiles[currentFilePath] = content;

            // 重置修改状态
            ui->textEdit->document()->setModified(false);

            // 显示保存状态
            QString fileName = QFileInfo(currentFilePath).fileName();
            if (encodingChanged) {
                showAutoSaveStatus(true, fileName + tr(" [编码: %1]").arg(newEncodingStr));
            } else {
                showAutoSaveStatus(true, fileName);
            }
        } else {
            showAutoSaveStatus(false, QFileInfo(currentFilePath).fileName());
        }

        // 【修复】确保清理临时文件
        if (QFile::exists(tempPath)) {
            QFile::remove(tempPath);
        }
    } else {
        showAutoSaveStatus(false, QFileInfo(currentFilePath).fileName());
    }

    // 清理所有可能的临时文件
    cleanupTemporaryFiles();
}

QString Widget::getBackupDirectory() const
{
    QString backupDir;

    // 1. 优先使用程序所在目录
    QString appDir = QCoreApplication::applicationDirPath();

    // 在程序目录下创建Backup文件夹
    QString programBackupDir = QDir(appDir).filePath("Backup");

    // 测试程序目录是否可写
    QDir testDir(appDir);
    if (testDir.mkdir("test_write_permission")) {
        // 有写权限，删除测试文件夹
        testDir.rmdir("test_write_permission");
        backupDir = programBackupDir;
    } else {
        // 2. 回退到用户设置路径
        if (!asPath.isEmpty() && asPath != appDir) {
            QDir dir(asPath);
            if (dir.exists() || dir.mkpath(asPath)) {
                backupDir = QDir(asPath).filePath("LTNoteBook_Backup");
            }
        }

        // 3. 回退到用户文档目录
        if (backupDir.isEmpty()) {
            QString docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            if (!docsPath.isEmpty()) {
                backupDir = QDir(docsPath).filePath("LTNoteBook_Backup");
            }
        }

        // 4. 最后回退到桌面
        if (backupDir.isEmpty()) {
            QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
            if (!desktopPath.isEmpty()) {
                backupDir = QDir(desktopPath).filePath("LTNoteBook_Backup");
            }
        }
    }

    // 创建备份目录
    QDir dir;
    if (!dir.exists(backupDir)) {
        if (dir.mkpath(backupDir)) {
            // 只在程序目录创建简单的说明文件
            if (backupDir.startsWith(QCoreApplication::applicationDirPath())) {
                createProgramDirReadme(backupDir);
            }
        } else {
            return QString();
        }
    }

    return backupDir;
}

QString Widget::getReadmeFileName() const
{
    QSettings settings(ORG, APP);
    QString langCode = settings.value(KEY_LANG, "zh_CN").toString();

    if (langCode == "en") {
        return "! README - Do Not Delete.txt";
    } else if (langCode == "ja") {
        return "! 説明書 - 削除しないでください.txt";
    } else {
        return "! 说明 - 请勿删除.txt";
    }
}

void Widget::cleanupOldReadmeFiles(const QString &backupDir) const
{
    QDir dir(backupDir);

    // 所有可能的说明文件名（仅三种语言）
    QStringList possibleReadmeNames = {
        "! 说明 - 请勿删除.txt",              // 中文
        "! README - Do Not Delete.txt",       // 英文
        "! 説明書 - 削除しないでください.txt", // 日文
        "说明.txt"                            // 旧版本中文文件名
    };

    QString currentReadmeName = getReadmeFileName();

    for (const QString &fileName : possibleReadmeNames) {
        if (fileName != currentReadmeName) {
            QString filePath = dir.filePath(fileName);
            if (QFile::exists(filePath)) {
#ifdef Q_OS_WIN
                // 移除只读属性
                SetFileAttributesW(reinterpret_cast<const wchar_t*>(filePath.utf16()),
                                   FILE_ATTRIBUTE_NORMAL);
#endif
                // 实际删除文件
                if (QFile::remove(filePath)) {
                    qDebug() << tr("已清理旧说明文件：") << fileName;
                }
            }
        }
    }
}

bool Widget::isReadmeFile(const QString &fileName) const
{
    QStringList readmeNames = {
        "! 说明 - 请勿删除.txt",              // 中文
        "! README - Do Not Delete.txt",       // 英文
        "! 説明書 - 削除しないでください.txt", // 日文
        "说明.txt"                            // 旧版本
    };

    return readmeNames.contains(fileName);
}

void Widget::createProgramDirReadme(const QString &backupDir) const
{
    // 使用多语言文件名
    QString readmePath = QDir(backupDir).filePath(getReadmeFileName());

    // 如果说明文件已存在，不重复创建
    if (QFile::exists(readmePath)) {
        return;
    }

    // 清理其他语言的旧说明文件
    cleanupOldReadmeFiles(backupDir);

    QFile readme(readmePath);
    if (readme.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&readme);
        out.setEncoding(QStringConverter::Utf8);
        out << "\xEF\xBB\xBF"; // UTF-8 BOM

        QString content = tr(
                              "===================================\n"
                              "   LTNoteBook 智能备份文件夹\n"
                              "===================================\n\n"
                              "📁 此文件夹用途：保存您文档的自动备份\n\n"
                              "🔄 备份机制：\n"
                              "• 每个文件只保留一个最新备份，避免重复\n"
                              "• 智能检测内容变化，仅在真正修改时才备份\n"
                              "• 支持多种编码格式的文件备份\n\n"
                              "📝 备份命名规则：\n"
                              "• 已保存文件：文件名.backup.扩展名\n"
                              "• 新建文档：新文档_备份_时间戳.txt/md\n\n"
                              "💡 实际示例：\n"
                              "• 我的笔记.txt → 我的笔记.backup.txt\n"
                              "• 项目代码.cpp → 项目代码.backup.cpp\n"
                              "• report.md → report.backup.md\n\n"
                              "🛠️ 使用方法：\n"
                              "• 恢复文件：复制.backup文件并重命名去掉.backup\n"
                              "• 清理空间：可安全删除不需要的备份文件\n"
                              "• 完全移除：可删除整个Backup文件夹（程序会重建）\n\n"
                              "⚙️ 备份触发条件：\n"
                              "• 文件内容发生实际变化时\n"
                              "• 切换到其他文件时\n"
                              "• 程序关闭前\n\n"
                              "ℹ️ 重要说明：\n"
                              "• 此说明文件如被删除会自动重建\n"
                              "• 备份文件使用与原文件相同的编码格式\n"
                              "• 程序卸载时可手动删除此文件夹\n\n"
                              "📍 备份位置：%1\n"
                              "⏰ 创建时间：%2\n\n"
                              "===================================\n"
                              "此文件由 LTNoteBook 自动生成和维护\n"
                              "===================================\n"
                              ).arg(backupDir, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

        out << content;
        readme.close();

        // 设置文件属性，提示用户不要删除
#ifdef Q_OS_WIN
        SetFileAttributesW(reinterpret_cast<const wchar_t*>(readmePath.utf16()),
                           FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN);
#else
        // 在 Linux/Mac 上，尝试设置为只读
        QFile::setPermissions(readmePath, QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther);
#endif
    }
}

void Widget::cleanupBackupFolder()
{
    QString backupDir = getBackupDirectory();
    if (backupDir.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("无法找到备份文件夹"));
        return;
    }

    QDir dir(backupDir);
    if (!dir.exists()) {
        QMessageBox::warning(this, tr("错误"), tr("备份文件夹不存在"));
        return;
    }

    // 获取所有备份文件
    QStringList nameFilters;
    nameFilters << "*.backup.*" << "*_备份_*.*" << "*_自动保存_*.*";

    QFileInfoList backupFiles = dir.entryInfoList(nameFilters, QDir::Files);

    if (backupFiles.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("备份文件夹中没有备份文件"));
        return;
    }

    // 显示确认对话框
    QString message = tr("找到 %1 个备份文件\n\n"
                         "是否要清理所有备份文件？\n"
                         "（说明文件将被保留）").arg(backupFiles.size());

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("清理备份文件夹"),
        message,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        int deletedCount = 0;
        QStringList failedFiles;

        for (const QFileInfo &fileInfo : backupFiles) {
            if (QFile::remove(fileInfo.absoluteFilePath())) {
                deletedCount++;
            } else {
                failedFiles.append(fileInfo.fileName());
            }
        }

        QString resultMessage;
        if (failedFiles.isEmpty()) {
            resultMessage = tr("成功删除 %1 个备份文件").arg(deletedCount);
        } else {
            resultMessage = tr("删除了 %1 个文件，%2 个文件删除失败：\n%3")
                                .arg(deletedCount)
                                .arg(failedFiles.size())
                                .arg(failedFiles.join("\n"));
        }

        QMessageBox::information(this, tr("清理完成"), resultMessage);
    }
}

void Widget::checkReadmeIntegrity(const QString &backupDir) const
{
    QString readmePath = QDir(backupDir).filePath(getReadmeFileName());
    if (!QFile::exists(readmePath)) {
        // 说明文件不存在，重新创建
        createProgramDirReadme(backupDir);

        // 显示提示信息
        static bool hasShownDeleteWarning = false;
        if (!hasShownDeleteWarning) {
            // 只在程序运行期间显示一次提示
            QTimer::singleShot(100, [this]() {
                if (this && !this->isHidden()) {
                    QMessageBox::information(
                        const_cast<Widget*>(this),
                        tr("提示"),
                        tr("检测到备份说明文件被删除，已自动重新创建。\n\n"
                           "此文件用于说明备份文件夹的用途，建议保留。")
                        );
                }
            });
            hasShownDeleteWarning = true;
        }
    }
}

void Widget::updateAutosaveTimer()
{
    autosaveTimer.stop();
    // 断开所有连接避免重复连接
    disconnect(&autosaveTimer, &QTimer::timeout, this, &Widget::onAutoSaveTimeout);

    if (asEnabled && asInterval > 0) {
        connect(&autosaveTimer, &QTimer::timeout, this, &Widget::onAutoSaveTimeout);
        autosaveTimer.start(asInterval * 1000);
    }
}

void Widget::cleanupOldNewDocumentBackups(const QString &backupDir) const
{
    QDir dir(backupDir);
    QStringList filters;
    filters << tr("新文档_备份_*.txt") << tr("新文档_自动保存_*.txt");

    QFileInfoList backups = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);

    // 删除5分钟前的文件
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-300); // 5分钟前

    for (const QFileInfo &backup : backups) {
        if (backup.lastModified() < cutoff) {
            if (QFile::remove(backup.absoluteFilePath())) {
                qDebug() << tr("已清理过期新文档备份：") << backup.fileName();
            }
        }
    }
}

void Widget::openAutoSaveFolder()
{
    QString autoSaveDir;

    // 优先检查程序目录
    QString appDir = QCoreApplication::applicationDirPath();
    QString programAutoSaveDir = QDir(appDir).filePath("AutoSave");

    if (QDir(programAutoSaveDir).exists()) {
        autoSaveDir = programAutoSaveDir;
    } else if (!asPath.isEmpty()) {
        autoSaveDir = QDir(asPath).filePath("AutoSave");
    }

    if (autoSaveDir.isEmpty() || !QDir(autoSaveDir).exists()) {
        QMessageBox::information(this, tr("提示"),
                                 tr("自动保存文件夹不存在，可能还没有创建自动保存文件。\n"
                                    "自动保存文件会在以下位置：\n%1")
                                     .arg(QDir::toNativeSeparators(programAutoSaveDir)));
        return;
    }

    bool success = QDesktopServices::openUrl(QUrl::fromLocalFile(autoSaveDir));

    if (!success) {
        QString nativePath = QDir::toNativeSeparators(autoSaveDir);
        QMessageBox::information(this, tr("自动保存文件夹"),
                                 tr("无法自动打开文件夹，请手动打开：\n%1").arg(nativePath));
    }
}

void Widget::openBackupFolder()
{
    QString backupDir = getBackupDirectory();
    if (backupDir.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("无法找到备份文件夹"));
        return;
    }

    // 确保文件夹存在
    QDir dir(backupDir);
    bool isNewlyCreated = !dir.exists();

    if (!dir.exists()) {
        // 备份文件夹不存在，创建它
        if (!dir.mkpath(backupDir)) {
            QMessageBox::warning(this, tr("错误"),
                                 tr("无法创建备份文件夹：\n%1").arg(backupDir));
            return;
        }

        // 新创建的文件夹，添加说明文件
        if (backupDir.startsWith(QCoreApplication::applicationDirPath())) {
            createProgramDirReadme(backupDir);

            QMessageBox::information(this, tr("提示"),
                                     tr("备份文件夹不存在，已重新创建：\n%1\n\n"
                                        "说明文件也已创建。").arg(QDir::toNativeSeparators(backupDir)));
        }
    } else {
        // 文件夹存在，检查说明文件
        if (backupDir.startsWith(QCoreApplication::applicationDirPath())) {
            checkReadmeIntegrity(backupDir);
        }
    }

    // 创建右键菜单，提供更多选项
    QMenu menu(this);
    QAction *openAction = menu.addAction(tr("打开备份文件夹"));
    QAction *cleanupAction = menu.addAction(tr("清理备份文件"));
    menu.addSeparator();
    QAction *refreshReadmeAction = menu.addAction(tr("重新生成说明文件"));

    QAction *selectedAction = menu.exec(QCursor::pos());

    if (selectedAction == openAction) {
        // 尝试打开文件夹
        bool success = QDesktopServices::openUrl(QUrl::fromLocalFile(backupDir));

        if (!success) {
            // 如果失败，显示路径给用户手动打开
            QString message = tr("备份文件夹位置：\n%1\n\n无法自动打开文件夹，请手动打开上述路径。")
                                  .arg(QDir::toNativeSeparators(backupDir));
            QMessageBox::information(this, tr("备份文件夹"), message);
        } else if (isNewlyCreated) {
            // 成功打开新创建的文件夹时的提示
            static bool firstTimeCreateBackup = true;
            if (firstTimeCreateBackup) {
                QMessageBox::information(this, tr("备份文件夹"),
                                         tr("已创建新的备份文件夹。\n\n"
                                            "您可以随时删除整个文件夹，程序会在需要时重新创建。"));
                firstTimeCreateBackup = false;
            }
        }
    } else if (selectedAction == cleanupAction) {
        cleanupBackupFolder();
    } else if (selectedAction == refreshReadmeAction) {
        // 删除旧的说明文件并重新创建
        cleanupOldReadmeFiles(backupDir);
        QString currentReadmePath = QDir(backupDir).filePath(getReadmeFileName());

#ifdef Q_OS_WIN
        if (QFile::exists(currentReadmePath)) {
            SetFileAttributesW(reinterpret_cast<const wchar_t*>(currentReadmePath.utf16()),
                               FILE_ATTRIBUTE_NORMAL);
        }
#endif

        QFile::remove(currentReadmePath);
        createProgramDirReadme(backupDir);
        QMessageBox::information(this, tr("提示"), tr("说明文件已重新生成"));
    }
}

void Widget::showAutoSaveStatus(bool success, const QString &fileName)
{
    QString message;
    if (success) {
        // 显示完整的路径信息
        QString fullPath;
        if (!currentFilePath.isEmpty()) {
            fullPath = currentFilePath;
        } else {
            // 新文档的情况，显示保存路径
            QString appDir = QCoreApplication::applicationDirPath();
            QString programAutoSaveDir = QDir(appDir).filePath("AutoSave");

            QDir testDir(appDir);
            if (testDir.mkdir("test_write_permission")) {
                testDir.rmdir("test_write_permission");
                fullPath = QDir(programAutoSaveDir).filePath(fileName);
            } else {
                fullPath = QDir(QDir(asPath).filePath("AutoSave")).filePath(fileName);
            }
        }

        message = tr("✓ 自动保存: %1").arg(QDir::toNativeSeparators(fullPath));
    } else {
        message = tr("✗ 自动保存失败: %1").arg(fileName);
    }

    // 在窗口标题临时显示
    QString originalTitle = windowTitle();
    setWindowTitle(message);

    QTimer::singleShot(3000, this, [this]() {
        updateWindowTitle(); // 恢复正常标题
    });
}

void Widget::createNewDocumentBackup(const QString &content)
{
    QString backupDir = getBackupDirectory();
    if (backupDir.isEmpty()) {
        qWarning() << tr("无法获取备份目录");
        return;
    }

    // 确保备份目录存在
    QDir dir;
    if (!dir.exists(backupDir)) {
        if (!dir.mkpath(backupDir)) {
            return;
        }
        // 如果是程序目录，创建说明文件
        if (backupDir.startsWith(QCoreApplication::applicationDirPath())) {
            createProgramDirReadme(backupDir);
        }
    }

    cleanupOldNewDocumentBackups(backupDir);

    QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

    // 获取用户选择的编码
    QStringConverter::Encoding encoding = encodingFromString(ui->fileCombobox->currentText());

    QSettings s(ORG, APP);
    bool markdownOn = s.value(KEY_MD, false).toBool();

    QString backupFileName;
    if (markdownOn) {
        backupFileName = QString(tr("新文档_备份_%1.md")).arg(timeStamp);
    } else {
        backupFileName = QString(tr("新文档_备份_%1.txt")).arg(timeStamp);
    }

    QString backupPath = QDir(backupDir).filePath(backupFileName);

    QFile backupFile(backupPath);
    if (backupFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // 根据用户选择的编码写入BOM
        if (encoding == QStringConverter::Utf8) {
            backupFile.write("\xEF\xBB\xBF");
        } else if (encoding == QStringConverter::Utf16LE) {
            backupFile.write("\xFF\xFE");
        } else if (encoding == QStringConverter::Utf16BE) {
            backupFile.write("\xFE\xFF");
        }

        QTextStream out(&backupFile);
        out.setEncoding(encoding);
        out << content;
        backupFile.close();

        showAutoBackupStatus(true, backupFileName);
    } else {
        showAutoBackupStatus(false, backupFileName);
    }
    cleanupExcessiveBackups();
}

void Widget::showAutoBackupStatus(bool success, const QString &fileName)
{
    QString message;
    if (success) {
        // 显示完整的备份路径
        QString backupDir = getBackupDirectory();
        QString fullPath = QDir(backupDir).filePath(fileName);

        message = tr("✓ 自动备份: %1").arg(QDir::toNativeSeparators(fullPath));
    } else {
        message = tr("✗ 自动备份失败: %1").arg(fileName);
    }

    // 在窗口标题临时显示
    QString originalTitle = windowTitle();
    setWindowTitle(message);

    QTimer::singleShot(3000, this, [this]() {
        updateWindowTitle(); // 恢复正常标题
    });
}

void Widget::onAutoSaveTimeout()
{
    if (!autoSaveDirty) {
        return;
    }

    QString content = ui->textEdit->toPlainText();
    if (content.isEmpty()) {
        autoSaveDirty = false;
        return;
    }

    if (!currentFilePath.isEmpty()) {
        // 有文件路径的情况
        if (asEnabled) {
            // 启用自动保存时，先进行智能备份，然后保存到原文件
            if (m_smartBackupEnabled) {
                performSmartBackup(content);
            }

            // 保存前清理旧的临时文件
            cleanupTemporaryFiles();
            saveToOriginalFile(content);
        } else {
            // 未启用自动保存：创建智能备份到Backup目录
            performSmartBackup(content);
        }
    } else {
        // 新文档的情况
        if (asEnabled) {
            // 启用自动保存时，先创建备份，然后保存到AutoSave目录
            if (m_smartBackupEnabled) {
                createNewDocumentBackup(content);
            }
            saveNewDocumentToTemp(content);
        } else {
            // 未启用自动保存：创建备份到Backup目录
            createNewDocumentBackup(content);
        }
    }

    autoSaveDirty = false;
}

QString Widget::getBackupFileName(const QString &originalPath) const
{
    QString backupDir = getBackupDirectory();
    if (backupDir.isEmpty()) return QString();

    QFileInfo fileInfo(originalPath);
    QString fileName = fileInfo.fileName();

    // 防止说明文件被当作普通文件处理
    if (isReadmeFile(fileName)) {
        return QString();
    }

    QString baseName = fileInfo.completeBaseName();
    QString extension = fileInfo.suffix();

    // 处理已经包含.backup的文件名
    if (baseName.endsWith(".backup")) {
        baseName = baseName.left(baseName.lastIndexOf(".backup"));
    }

    QString backupFileName;
    if (extension.isEmpty()) {
        backupFileName = QString("%1.backup").arg(baseName);
    } else {
        backupFileName = QString("%1.backup.%2").arg(baseName, extension);
    }

    return QDir(backupDir).filePath(backupFileName);
}

void Widget::cleanupOldBackups(const QString &originalPath) const
{
    QString backupDir = getBackupDirectory();
    if (backupDir.isEmpty()) return;

    // 如果原文件是说明文件，不进行清理
    QFileInfo fileInfo(originalPath);
    if (isReadmeFile(fileInfo.fileName())) {
        return;
    }

    QString baseName = fileInfo.completeBaseName();
    QString extension = fileInfo.suffix();

    QDir dir(backupDir);
    QStringList filters;

    // 查找所有可能的旧备份格式
    if (extension.isEmpty()) {
        filters << QString("%1.backup_*").arg(baseName)
        << QString("%1_backup_*").arg(baseName)
        << QString("%1*.backup").arg(baseName);
    } else {
        filters << QString("%1.backup_*.%2").arg(baseName, extension)
        << QString("%1_backup_*.%2").arg(baseName, extension)
        << QString("%1*.backup.%2").arg(baseName, extension);
    }

    QFileInfoList oldBackups = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo &backup : oldBackups) {
        QString oldBackupPath = backup.absoluteFilePath();
        QString currentBackupPath = getBackupFileName(originalPath);

        // 不删除当前的备份文件，也不删除说明文件
        if (oldBackupPath != currentBackupPath &&
            !isReadmeFile(backup.fileName())) {
            if (QFile::remove(oldBackupPath)) {
                qDebug() << tr("已清理旧备份：") << backup.fileName();
            }
        }
    }
}

void Widget::updateBackupIfChanged(const QString &filePath, const QString &currentContent)
{
    if (filePath.isEmpty() || !m_hasInitialBackup) return;

    // 如果是说明文件，直接跳过备份
    QFileInfo fileInfo(filePath);
    if (isReadmeFile(fileInfo.fileName())) {
        return;
    }

    // 比较当前内容与初始内容
    if (m_initialFileContent.trimmed() == currentContent.trimmed()) {
        return;
    }

    QString backupPath = getBackupFileName(filePath);
    if (backupPath.isEmpty()) return;

    // 更新备份文件
    QFile backupFile(backupPath);
    if (!backupFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QStringConverter::Encoding encoding = fileEncodings.value(filePath, DEFAULT_ENCODING);

    // 写入BOM
    if (encoding == QStringConverter::Utf8) {
        backupFile.write("\xEF\xBB\xBF");
    } else if (encoding == QStringConverter::Utf16LE) {
        backupFile.write("\xFF\xFE");
    } else if (encoding == QStringConverter::Utf16BE) {
        backupFile.write("\xFE\xFF");
    }

    QTextStream out(&backupFile);
    out.setEncoding(encoding);
    out << currentContent;
    backupFile.close();

    // 更新初始内容记录
    m_initialFileContent = currentContent;

    QString displayName = QFileInfo(filePath).fileName();
    qDebug() << tr("已更新备份（内容有变化）：") << displayName;
}

void Widget::createInitialBackup(const QString &filePath, const QString &content)
{
    if (filePath.isEmpty() || content.isEmpty()) return;

    // 如果是说明文件，直接跳过备份
    QFileInfo fileInfo(filePath);
    if (isReadmeFile(fileInfo.fileName())) {
        return;
    }

    QString backupPath = getBackupFileName(filePath);
    if (backupPath.isEmpty()) return;

    // 如果备份已存在，不重复创建
    if (QFile::exists(backupPath)) {
        m_initialFileContent = content;
        m_hasInitialBackup = true;
        return;
    }

    // 创建备份目录
    QString backupDir = QFileInfo(backupPath).absolutePath();
    QDir().mkpath(backupDir);

    // 写入备份文件
    QFile backupFile(backupPath);
    if (!backupFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << tr("无法创建初始备份：") << backupPath;
        return;
    }

    QStringConverter::Encoding encoding = fileEncodings.value(filePath, DEFAULT_ENCODING);

    // 写入BOM
    if (encoding == QStringConverter::Utf8) {
        backupFile.write("\xEF\xBB\xBF");
    } else if (encoding == QStringConverter::Utf16LE) {
        backupFile.write("\xFF\xFE");
    } else if (encoding == QStringConverter::Utf16BE) {
        backupFile.write("\xFE\xFF");
    }

    QTextStream out(&backupFile);
    out.setEncoding(encoding);
    out << content;
    backupFile.close();

    // 清理旧备份格式
    cleanupOldBackups(filePath);

    // 记录初始状态
    m_initialFileContent = content;
    m_hasInitialBackup = true;

    QString displayName = QFileInfo(filePath).fileName();
    cleanupExcessiveBackups();
    qDebug() << tr("已创建初始备份：") << displayName;
}

void Widget::performSmartBackup(const QString &content)
{
    // 如果当前文件是说明文件，直接跳过备份
    if (!currentFilePath.isEmpty()) {
        QFileInfo fileInfo(currentFilePath);
        if (isReadmeFile(fileInfo.fileName())) {
            return;
        }
    }

    // 检查内容是否有实际变化
    if (m_hasInitialBackup && m_initialFileContent.trimmed() == content.trimmed()) {
        return;
    }

    QString backupPath = getBackupFileName(currentFilePath);
    if (backupPath.isEmpty()) {
        return;
    }

    // 确保备份目录存在
    QString backupDir = QFileInfo(backupPath).absolutePath();
    bool isNewlyCreated = !QDir(backupDir).exists();

    if (!QDir().mkpath(backupDir)) {
        return;
    }

    // 如果是新创建的目录，或者说明文件不存在，创建说明文件
    if (backupDir.startsWith(QCoreApplication::applicationDirPath())) {
        if (isNewlyCreated) {
            createProgramDirReadme(backupDir);
        } else {
            checkReadmeIntegrity(backupDir);
        }
    }

    // 写入备份文件
    QFile backupFile(backupPath);
    if (!backupFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        showAutoBackupStatus(false, QFileInfo(currentFilePath).fileName());
        return;
    }

    QStringConverter::Encoding encoding = fileEncodings.value(currentFilePath, DEFAULT_ENCODING);

    // 写入BOM
    if (encoding == QStringConverter::Utf8) {
        backupFile.write("\xEF\xBB\xBF");
    } else if (encoding == QStringConverter::Utf16LE) {
        backupFile.write("\xFF\xFE");
    } else if (encoding == QStringConverter::Utf16BE) {
        backupFile.write("\xFE\xFF");
    }

    QTextStream out(&backupFile);
    out.setEncoding(encoding);
    out << content;
    backupFile.close();

    // 更新初始内容记录
    m_initialFileContent = content;
    m_hasInitialBackup = true;

    // 清理旧备份
    cleanupOldBackups(currentFilePath);

    QString fileName = QFileInfo(currentFilePath).fileName();
    showAutoBackupStatus(true, fileName);
    cleanupExcessiveBackups();
}

void Widget::saveNewDocumentToTemp(const QString &content)
{
    QString savePath;
    QString fileName;

    // 获取用户选择的编码
    QStringConverter::Encoding encoding = encodingFromString(ui->fileCombobox->currentText());

    QSettings s(ORG, APP);
    bool markdownOn = s.value(KEY_MD, false).toBool();

    // 启用自动保存时直接保存到指定路径，不创建子文件夹
    if (asEnabled && !asPath.isEmpty()) {
        // 启用自动保存：直接保存到用户指定路径
        QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        if (markdownOn) {
            fileName = QString(tr("新文档_自动保存_%1.md")).arg(timeStamp);
        } else {
            fileName = QString(tr("新文档_自动保存_%1.txt")).arg(timeStamp);
        }
        savePath = QDir(asPath).filePath(fileName);
    } else {
        // 未启用自动保存：使用程序目录的AutoSave子文件夹作为临时保存
        QString appDir = QCoreApplication::applicationDirPath();
        QString tempDir = QDir(appDir).filePath("AutoSave");

        // 测试程序目录是否可写
        QDir testDir(appDir);
        if (!testDir.mkdir("test_write_permission")) {
            // 程序目录不可写，回退到文档目录
            QString docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            tempDir = QDir(docsPath).filePath("LTNoteBook_AutoSave");
        } else {
            testDir.rmdir("test_write_permission");
        }

        if (!QDir().mkpath(tempDir)) {
            showAutoSaveStatus(false, tr("新文档"));
            return;
        }

        QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        if (markdownOn) {
            fileName = QString(tr("新文档_临时保存_%1.md")).arg(timeStamp);
        } else {
            fileName = QString(tr("新文档_临时保存_%1.txt")).arg(timeStamp);
        }
        savePath = QDir(tempDir).filePath(fileName);
    }

    // 确保保存目录存在
    QDir saveDir = QFileInfo(savePath).absoluteDir();
    if (!saveDir.exists() && !saveDir.mkpath(saveDir.absolutePath())) {
        showAutoSaveStatus(false, fileName);
        return;
    }

    QFile saveFile(savePath);
    if (saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // 根据用户选择的编码写入BOM
        if (encoding == QStringConverter::Utf8) {
            saveFile.write("\xEF\xBB\xBF");
        } else if (encoding == QStringConverter::Utf16LE) {
            saveFile.write("\xFF\xFE");
        } else if (encoding == QStringConverter::Utf16BE) {
            saveFile.write("\xFE\xFF");
        }

        QTextStream out(&saveFile);
        out.setEncoding(encoding);  // 使用用户选择的编码
        out << content;
        saveFile.close();

        if (QFile::exists(savePath)) {
            currentFilePath = savePath;
            openedFiles[savePath] = content;
            fileEncodings[savePath] = encoding;  // 保存用户选择的编码

            // 持久化编码设置
            QSettings settings(ORG, APP);
            settings.setValue("FileEncoding/" + savePath, encodingToString(encoding));

            if (!fileOrder.contains(savePath)) {
                fileOrder.append(savePath);
            }
            addToHistory(savePath);
            updateFileArrangement();
        }

        ui->textEdit->document()->setModified(false);
        updateWindowTitle();
        showAutoSaveStatus(true, fileName);
    } else {
        showAutoSaveStatus(false, fileName);
    }
}

void Widget::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        setupToolTips();
        updateFileArrangement();
        updateCharCount();
        updateCursorPosition();
    }
    QWidget::changeEvent(e);
}

void Widget::closeEvent(QCloseEvent *event)
{
    if (!checkUnsavedChanges()) {
        event->ignore();
        return;
    }

    // 智能备份检查
    if (!currentFilePath.isEmpty() && m_smartBackupEnabled) {
        updateBackupIfChanged(currentFilePath, ui->textEdit->toPlainText());
    }

    // 如果启用了自动保存且有未保存修改，显示保存信息
    if (asEnabled && autoSaveDirty && !currentFilePath.isEmpty()) {
        QString fileName = QFileInfo(currentFilePath).fileName();
        QString message = tr("正在保存文件: %1").arg(fileName);
        setWindowTitle(message);

        onAutoSaveTimeout();

        // 短暂显示保存完成信息
        QTimer::singleShot(500, [this]() {
            setWindowTitle(tr("保存完成"));
        });
    }

    // 程序关闭时清理所有临时文件
    cleanupTimer.stop();
    cleanupTemporaryFiles();

    event->accept();
}

void Widget::onLanguageChanged(const QString &code)
{
    QSettings(ORG, APP).setValue(KEY_LANG, code);
    TranslationManager::instance().switchLanguage(code);
    ui->retranslateUi(this);
    setupToolTips();
    updateFileArrangement();
    updateCharCount();
    updateCursorPosition();
    QString backupDir = getBackupDirectory();
    if (!backupDir.isEmpty() && QDir(backupDir).exists()) {
        cleanupOldReadmeFiles(backupDir);
        createProgramDirReadme(backupDir);
    }
}

void Widget::on_btnOpenSet_clicked()
{
    ShortSetting dlg(this);
    dlg.setWindowTitle(tr("设置"));

    connect(&dlg, &ShortSetting::shortcutUpdated,
            this, &Widget::loadShortcuts);

    connect(&dlg, &ShortSetting::languageChanged,
            this, &Widget::onLanguageChanged);

    connect(&dlg, &ShortSetting::languageChanged,
            &dlg, [&](const QString&){
                QEvent ev(QEvent::LanguageChange);
                QCoreApplication::sendEvent(&dlg, &ev);
            });

    if (dlg.exec() == QDialog::Accepted) {
        QSettings s(ORG, APP);
        bool markdownOn = s.value(KEY_MD, false).toBool();
        if (markdownOn && !mdHighlighter) {
            mdHighlighter = new MarkdownHighlighter(ui->textEdit->document());
        } else if (!markdownOn && mdHighlighter) {
            delete mdHighlighter;
            mdHighlighter = nullptr;
        }
        dlg.saveShortcuts();
        loadShortcuts();

        dlg.saveAutoSave();
        asEnabled  = dlg.autosaveEnabled();
        asInterval = dlg.autosaveInterval();
        asPath     = dlg.autosavePath();
        updateAutosaveTimer();
    }
}

bool Widget::checkUnsavedChanges()
{
    if (!ui->textEdit->document()->isModified()) {
        return true;
    }

    // 如果启用了自动保存，显示正在保存的文件名
    if (asEnabled) {
        QString fileName;
        if (!currentFilePath.isEmpty()) {
            fileName = QFileInfo(currentFilePath).fileName();
            // 显示正在自动保存的文件
            QString message = tr("正在自动保存文件: %1").arg(fileName);
            setWindowTitle(message);

            on_Save_clicked();

            // 恢复标题
            QTimer::singleShot(1000, this, [this]() {
                updateWindowTitle();
            });

            return !ui->textEdit->document()->isModified();
        } else {
            // 新文档自动保存到设置的路径
            saveNewDocumentToTemp(ui->textEdit->toPlainText());
            return !ui->textEdit->document()->isModified();
        }
    }

    // 未启用自动保存时显示具体文件名
    QString fileName = currentFilePath.isEmpty() ?
                           tr("新文档") :
                           QFileInfo(currentFilePath).fileName();

    // 检查是否真的有修改需要保存
    if (!currentFilePath.isEmpty()) {
        QString currentContent = ui->textEdit->toPlainText();
        QString originalContent = openedFiles.value(currentFilePath, "");

        // 如果内容没有实际变化，提示无需保存
        if (currentContent.trimmed() == originalContent.trimmed()) {
            QMessageBox::information(this, tr("提示"),
                                     tr("文件 \"%1\" 没有修改，无需保存。").arg(fileName));

            // 重置修改状态
            ui->textEdit->document()->setModified(false);
            updateWindowTitle();
            return true;
        }
    } else {
        // 新文档，检查是否只是空白内容
        QString content = ui->textEdit->toPlainText().trimmed();
        if (content.isEmpty()) {
            QMessageBox::information(this, tr("提示"),
                                     tr("新文档内容为空，无需保存。"));
            return true;
        }
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("未保存的修改"),
        tr("文件 \"%1\" 有未保存的修改，是否保存？").arg(fileName),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save
        );

    switch (reply) {
    case QMessageBox::Cancel:
        return false;
    case QMessageBox::Save:
        on_Save_clicked();
        return !ui->textEdit->document()->isModified();
    default:
        return true;  // Discard
    }
}

void Widget::setupToolTips()
{
    ui->Open->setToolTip(tr("打开文件"));
    ui->Save->setToolTip(tr("保存文件"));
    ui->Delete->setToolTip(tr("删除当前文件"));
    ui->Reset->setToolTip(tr("撤销"));
    ui->Withdraw->setToolTip(tr("重做"));
    ui->Exit->setToolTip(tr("关闭当前文件"));
    ui->btnOpenSet->setToolTip(tr("设置"));
    ui->fontBox->setToolTip(tr("选择字体"));
    ui->fileCombobox->setToolTip(tr("文件编码格式"));
    ui->fileArrangement->setToolTip(tr("文件历史记录"));
}
