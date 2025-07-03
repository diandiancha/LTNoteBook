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
#include <QTimer>
#include <QHelpEvent>

const QStringConverter::Encoding DEFAULT_ENCODING = QStringConverter::Utf8;

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

QStringConverter::Encoding Widget::encodingFromString(const QString& s)
{
    if (s.compare("UTF-8", Qt::CaseInsensitive) == 0)    return QStringConverter::Utf8;
    if (s.compare("UTF-16 LE", Qt::CaseInsensitive) == 0) return QStringConverter::Utf16LE;
    if (s.compare("UTF-16 BE", Qt::CaseInsensitive) == 0) return QStringConverter::Utf16BE;
    if (s.compare("ANSI", Qt::CaseInsensitive) == 0)     return QStringConverter::System;
    return DEFAULT_ENCODING;
}

Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    QSettings s(ORG, APP);

    ui->setupUi(this);

    ui->retranslateUi(this);
    qApp->installEventFilter(this);

    bool markdownOn = s.value(KEY_MD, false).toBool();
    if (markdownOn) {
        mdHighlighter = new MarkdownHighlighter(ui->textEdit->document());
    }

    ui->textEdit->installEventFilter(this);

    connect(ui->fileCombobox, QOverload<int>::of(&QComboBox::activated), this, &Widget::on_fileCombobox_activated);
    connect(ui->textEdit, &QTextEdit::textChanged, this, &Widget::updateCharCount);
    connect(ui->textEdit, &QTextEdit::cursorPositionChanged, this, &Widget::updateCursorPosition);
    connect(ui->fileArrangement, QOverload<int>::of(&QComboBox::activated), this, &Widget::on_fileArrangement_activated);
    connect(ui->fontBox, QOverload<const QFont&>::of(&QFontComboBox::currentFontChanged), this, &Widget::on_fontBox_currentFontChanged);
    connect(ui->textEdit->document(), &QTextDocument::undoAvailable, ui->Reset, &QPushButton::setEnabled);
    connect(ui->textEdit->document(), &QTextDocument::redoAvailable, ui->Withdraw, &QPushButton::setEnabled);
    loadShortcuts();

    updateCharCount();
    updateCursorPosition();
    updateFileArrangement();

    updateCharCount();
    updateCursorPosition();
    updateFileArrangement();

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

    asEnabled = s.value("AutoSave/enabled", false).toBool();
    asInterval = s.value("AutoSave/interval", 30).toInt();
    asPath = s.value("AutoSave/path").toString();
    if (asPath.isEmpty())
        asPath = QStandardPaths::writableLocation(
            QStandardPaths::DesktopLocation);

    updateAutosaveTimer();

    debounceTimer.setSingleShot(true);
    debounceTimer.setInterval(500);
    connect(&debounceTimer, &QTimer::timeout, this, [this]() {
        if (asEnabled) onAutoSaveTimeout();
        });

    connect(ui->textEdit, &QTextEdit::textChanged, this, [this]() {
        autoSaveDirty = true;
        debounceTimer.start();
        });
    setupToolTips();
}

Widget::~Widget() {
    delete ui;
    if (mdHighlighter) delete mdHighlighter;
}

void Widget::openFileFromPath(const QString& path)
{
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray raw = file.readAll();
    QString text;
    QStringConverter::Encoding encoding = QStringConverter::Utf8;
    int bom = 0;

    if (raw.startsWith("\xEF\xBB\xBF")) {
        encoding = QStringConverter::Utf8;
        bom = 3;
    }
    else if (raw.startsWith("\xFF\xFE")) {
        encoding = QStringConverter::Utf16LE;
        bom = 2;
    }
    else if (raw.startsWith("\xFE\xFF")) {
        encoding = QStringConverter::Utf16BE;
        bom = 2;
    }
    else {
        encoding = detectEncoding(raw);
    }

    QByteArray content = raw.mid(bom);
    QStringDecoder decoder(encoding);
    text = decoder(content);

    if (text.contains(QChar::ReplacementCharacter)) {
        QStringConverter::Encoding fallbackEncoding = tryFallbackEncoding(raw, encoding);
        if (fallbackEncoding != encoding) {
            encoding = fallbackEncoding;
            QStringDecoder fallbackDecoder(encoding);
            text = fallbackDecoder(content);
        }
    }

    currentFilePath = path;
    openedFiles[path] = text;
    fileEncodings[path] = encoding;
    if (!fileOrder.contains(path)) fileOrder.append(path);
    fileHistory.clear();
    fileHistory.append(path);
    fileHistoryPos = 0;
    ui->textEdit->setPlainText(text);

    QString encStr = encodingToString(encoding);
    int idx = ui->fileCombobox->findText(encStr);
    if (idx >= 0)
        ui->fileCombobox->setCurrentIndex(idx);
    else
        ui->fileCombobox->setCurrentText(encStr);

    updateFileArrangement();
}

void Widget::updateEncodingSelection(const QString& path)
{
    auto enc = fileEncodings.value(path, DEFAULT_ENCODING);
    QString name = encodingToString(enc);
    int idx = ui->fileCombobox->findText(name);
    if (idx >= 0)
        ui->fileCombobox->setCurrentIndex(idx);
}

void Widget::on_Open_clicked()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        tr("打开文件"),
        {},
        tr("文本文件 (*.txt *.md *.cpp *.h);;所有文件 (*.*)")
    );
    QFileInfo info(path);
    if (info.suffix() == "exe" || info.suffix() == "dll") {
        QMessageBox::warning(this, tr("警告"), tr("不能打开二进制文件！"));
        return;
    }
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("错误"), tr("文件打开失败！"));
        return;
    }
    QByteArray raw = file.readAll();
    QString text;
    QStringConverter::Encoding encoding = QStringConverter::Utf8;
    int bom = 0;

    if (raw.startsWith("\xEF\xBB\xBF")) {
        encoding = QStringConverter::Utf8;
        bom = 3;
    }
    else if (raw.startsWith("\xFF\xFE")) {
        encoding = QStringConverter::Utf16LE;
        bom = 2;
    }
    else if (raw.startsWith("\xFE\xFF")) {
        encoding = QStringConverter::Utf16BE;
        bom = 2;
    }
    else {
        encoding = detectEncoding(raw);
    }

    QByteArray content = raw.mid(bom);
    QStringDecoder decoder(encoding);
    text = decoder(content);

    if (text.contains(QChar::ReplacementCharacter)) {
        QStringConverter::Encoding fallbackEncoding = tryFallbackEncoding(raw, encoding);
        if (fallbackEncoding != encoding) {
            encoding = fallbackEncoding;
            QStringDecoder fallbackDecoder(encoding);
            text = fallbackDecoder(content);
        }
    }

    fileEncodings[path] = encoding;
    openedFiles[path] = text;
    if (!fileOrder.contains(path)) fileOrder.append(path);
    if (fileHistoryPos == -1 || fileHistory.value(fileHistoryPos) != path) {
        fileHistory = fileHistory.mid(0, fileHistoryPos + 1);
        fileHistory.append(path);
        fileHistoryPos = fileHistory.size() - 1;
    }
    currentFilePath = path;
    ui->textEdit->setPlainText(text);
    QString encStr = encodingToString(encoding);
    int idx = ui->fileCombobox->findText(encStr);
    if (idx >= 0)
        ui->fileCombobox->setCurrentIndex(idx);
    else
        ui->fileCombobox->setCurrentText(encStr);
    updateFileArrangement();
}

QStringConverter::Encoding Widget::detectEncoding(const QByteArray& data)
{
    QByteArray sample = data.left(qMin(4096, data.size()));

    // Check for BOM
    if (sample.startsWith("\xFF\xFE")) return QStringConverter::Utf16LE;
    if (sample.startsWith("\xFE\xFF")) return QStringConverter::Utf16BE;
    if (sample.startsWith("\xEF\xBB\xBF")) return QStringConverter::Utf8;

    // ASCII check
    bool isAscii = std::all_of(sample.begin(), sample.end(), [](char c) {
        return static_cast<unsigned char>(c) < 128;
        });
    if (isAscii) return QStringConverter::Utf8;

    // UTF-8 validation
    QStringDecoder utf8Decoder(QStringConverter::Utf8);
    QString utf8Text = utf8Decoder(sample);
    if (!utf8Text.contains(QChar::ReplacementCharacter) && isLikelyText(utf8Text)) {
        return QStringConverter::Utf8;
    }

    // System validation
    QStringDecoder systemDecoder(QStringConverter::System);
    QString systemText = systemDecoder(sample);
    if (!systemText.contains(QChar::ReplacementCharacter) && isLikelyText(systemText)) {
        return QStringConverter::System;
    }

    // UTF-16 validation
    QStringConverter::Encoding utf16Encoding = detectUtf16ByteOrder(sample);
    QStringDecoder utf16Decoder(utf16Encoding);
    QString utf16Text = utf16Decoder(sample);
    if (!utf16Text.contains(QChar::ReplacementCharacter) && isLikelyText(utf16Text)) {
        return utf16Encoding;
    }

    return QStringConverter::Utf8;
}

QStringConverter::Encoding Widget::tryFallbackEncoding(const QByteArray& data, QStringConverter::Encoding currentEncoding)
{
    QByteArray sample = data.left(qMin(4096, data.size()));
    QList<QStringConverter::Encoding> fallbacks;

    if (currentEncoding == QStringConverter::Utf8) {
        fallbacks = { QStringConverter::System, QStringConverter::Latin1 };
    }
    else if (currentEncoding == QStringConverter::System) {
        fallbacks = { QStringConverter::Utf8, QStringConverter::Latin1 };
    }
    else {
        fallbacks = { QStringConverter::Utf8, QStringConverter::System, QStringConverter::Latin1 };
    }

    for (auto encoding : fallbacks) {
        if (encoding == currentEncoding) continue;

        QStringDecoder decoder(encoding);
        QString text = decoder(sample);
        if (!text.contains(QChar::ReplacementCharacter) && isLikelyText(text)) {
            return encoding;
        }
    }

    return currentEncoding;
}

bool Widget::isLikelyUtf16(const QByteArray& data)
{
    if (data.size() < 4) return false;
    int nullCount = 0;
    int evenNulls = 0;
    int oddNulls = 0;

    for (int i = 0; i < qMin(1000, data.size()); ++i) {
        if (data[i] == 0) {
            nullCount++;
            if (i % 2 == 0) evenNulls++;
            else oddNulls++;
        }
    }
    if (nullCount > data.size() / 20) {
        return (evenNulls > oddNulls * 2) || (oddNulls > evenNulls * 2);
    }

    return false;
}

QStringConverter::Encoding Widget::detectUtf16ByteOrder(const QByteArray& data)
{
    int leScore = 0;
    int beScore = 0;
    int length = qMin(1000, data.size() - (data.size() % 2));

    for (int i = 0; i < length; i += 2) {
        unsigned char byte1 = static_cast<unsigned char>(data[i]);
        unsigned char byte2 = static_cast<unsigned char>(data[i + 1]);

        if (byte1 == 0 && byte2 != 0) beScore++;
        else if (byte2 == 0 && byte1 != 0) leScore++;
    }

    if (leScore > beScore * 2) return QStringConverter::Utf16LE;
    if (beScore > leScore * 2) return QStringConverter::Utf16BE;

    return QStringConverter::Utf16LE;
}


bool Widget::isLikelyText(const QString& text)
{
    if (text.isEmpty()) return false;

    int printableCount = 0;
    int controlCount = 0;

    for (const QChar& c : text) {
        if (c.isPrint() || c.isSpace()) printableCount++;
        else if (c.isNull() || c.category() == QChar::Other_Control) controlCount++;
    }

    double printableRatio = double(printableCount) / text.length();
    return printableRatio > 0.85 && controlCount < text.length() * 0.05;
}

void Widget::on_Save_clicked()
{
    QString path = currentFilePath;

    if (path.isEmpty()) {
        path = QFileDialog::getSaveFileName(this,
            tr("保存文件"),
            tr("新建文件.txt"),
            tr("所有文件 (*.*);;文本文件 (*.txt);;文档 (*.doc);;Markdown (*.md);;C++ 源码 (*.cpp *.h)"));
        if (path.isEmpty()) return;

        currentFilePath = path;

        if (!fileOrder.contains(path))
            fileOrder.append(path);

        fileHistory = fileHistory.mid(0, fileHistoryPos + 1);
        fileHistory.append(path);
        fileHistoryPos = fileHistory.size() - 1;
    }

    if (!ui->textEdit->document()->isModified()) {
        QMessageBox::information(this, tr("提示"), tr("文件无修改，无需保存。"));
        return;
    }

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("错误"), tr("无法写入文件！"));
        return;
    }

    QStringConverter::Encoding encoding = encodingFromString(ui->fileCombobox->currentText());
    fileEncodings[path] = encoding;

    QTextStream out(&f);
    out.setEncoding(encoding);
    QString text = ui->textEdit->toPlainText();
    out << text;
    f.close();

    openedFiles[path] = text;
    updateFileArrangement();

    ui->textEdit->document()->setModified(false);
    QMessageBox::information(this, tr("提示"), tr("保存成功！"));
}

void Widget::on_Exit_clicked()
{
    ui->textEdit->clear();
    openedFiles.remove(currentFilePath);
    fileEncodings.remove(currentFilePath);
    fileOrder.removeAll(currentFilePath);
    currentFilePath.clear();
    fileHistory.clear();
    fileHistoryPos = -1;
    isNewUnsavedEdit = true;
    ui->fileCombobox->setCurrentText("UTF-8");
    updateFileArrangement();
}

void Widget::on_fileArrangement_activated(int index)
{
    if (m_ignoreFileArrangement) return;
    if (index < 0 || index >= fileHistory.size()) return;

    QString path = fileHistory[index];
    if (!openedFiles.contains(path)) return;

    fileHistoryPos = index;
    currentFilePath = path;
    ui->textEdit->setPlainText(openedFiles[path]);
    ui->fileCombobox->setCurrentText(encodingToString(fileEncodings.value(path, DEFAULT_ENCODING)));
    updateFileArrangement();
}

void Widget::on_fileCombobox_activated(int idx)
{
    if (currentFilePath.isEmpty()) return;

    auto enc = encodingFromString(ui->fileCombobox->itemText(idx));
    fileEncodings[currentFilePath] = enc;

    QFile f(currentFilePath);
    if (!f.open(QIODevice::ReadOnly)) return;
    if (enc == QStringConverter::Utf8 && f.peek(3).startsWith("\xEF\xBB\xBF"))
        f.seek(3);

    QTextStream in(&f); in.setEncoding(enc);
    QString txt = in.readAll();
    f.close();

    openedFiles[currentFilePath] = txt;
    ui->textEdit->setPlainText(txt);
}

void Widget::updateFileArrangement()
{
    m_ignoreFileArrangement = true;
    ui->fileArrangement->clear();

    for (int i = 0; i < fileHistory.size(); ++i) {
        QString name = QFileInfo(fileHistory[i]).fileName();
        ui->fileArrangement->addItem(tr("历史：%1").arg(name), QVariant(i));
    }

    if (fileHistoryPos >= 0)
        ui->fileArrangement->setCurrentIndex(fileHistoryPos);

    m_ignoreFileArrangement = false;
}

void Widget::on_fontBox_currentFontChanged(const QFont& f)
{
    QFont cur = ui->textEdit->font();
    cur.setFamily(f.family());
    ui->textEdit->setFont(cur);
}

void Widget::updateCharCount()
{
    ui->Word->setText(tr("共 %1 个字符").arg(ui->textEdit->toPlainText().size()));
}

void Widget::updateCursorPosition()
{
    QTextCursor c = ui->textEdit->textCursor();
    ui->Position->setText(tr("行 %1, 列 %2").arg(c.blockNumber() + 1).arg(c.positionInBlock() + 1));
}

void Widget::on_Reset_clicked()
{
    if (ui->textEdit->document()->isUndoAvailable())
        ui->textEdit->undo();
}

void Widget::on_Withdraw_clicked() {
    if (ui->textEdit->document()->isRedoAvailable())
        ui->textEdit->redo();
}

void Widget::loadShortcuts()
{
    QSettings settings(ORG, APP);

    if (scOpen) {
        disconnect(scOpen, nullptr, nullptr, nullptr);
        delete scOpen;
        scOpen = nullptr;
    }
    if (scSave) {
        disconnect(scSave, nullptr, nullptr, nullptr);
        delete scSave;
        scSave = nullptr;
    }
    if (scUndo) {
        disconnect(scUndo, nullptr, nullptr, nullptr);
        delete scUndo;
        scUndo = nullptr;
    }
    if (scRedo) {
        disconnect(scRedo, nullptr, nullptr, nullptr);
        delete scRedo;
        scRedo = nullptr;
    }

    QString undoShortcut = settings.value("Shortcut/Undo", "Ctrl+Z").toString();
    QString redoShortcut = settings.value("Shortcut/Redo", "Ctrl+Y").toString();

    scOpen = new QShortcut(settings.value("Shortcut/Open", "Ctrl+O").value<QKeySequence>(), this);
    connect(scOpen, &QShortcut::activated, this, &Widget::on_Open_clicked);

    scSave = new QShortcut(settings.value("Shortcut/Save", "Ctrl+S").value<QKeySequence>(), this);
    connect(scSave, &QShortcut::activated, this, &Widget::on_Save_clicked);

    scUndo = new QShortcut(QKeySequence(undoShortcut), this);
    connect(scUndo, &QShortcut::activated, ui->textEdit, &QTextEdit::undo);

    scRedo = new QShortcut(QKeySequence(redoShortcut), this);
    connect(scRedo, &QShortcut::activated, ui->textEdit, &QTextEdit::redo);

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

    return false;
}

bool Widget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->textEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        QSettings settings(ORG, APP);
        QString undoShortcut = settings.value("Shortcut/Undo", "Ctrl+Z").toString();
        QString redoShortcut = settings.value("Shortcut/Redo", "Ctrl+Y").toString();

        if (keyEvent->modifiers() == Qt::ControlModifier) {
            if (keyEvent->key() == Qt::Key_Z && undoShortcut != "Ctrl+Z") {
                event->accept();
                return true;
            }
            if (keyEvent->key() == Qt::Key_Y && redoShortcut != "Ctrl+Y") {
                event->accept();
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void Widget::updateAutosaveTimer()
{
    autosaveTimer.stop();
    disconnect(&autosaveTimer, nullptr, this, nullptr);

    if (!asEnabled)
        return;

    connect(&autosaveTimer, &QTimer::timeout,
        this, &Widget::onAutoSaveTimeout);
    autosaveTimer.start(asInterval * 1000);
}

void Widget::onAutoSaveTimeout()
{
    if (!autoSaveDirty)
        return;

    QString fullPath;
    if (!currentFilePath.isEmpty()) {
        fullPath = currentFilePath;
    }
    else {
        QString defaultDir = asPath.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
            : asPath;
        QDir dir(defaultDir);
        if (!dir.exists() && !dir.mkpath(".")) {
            qWarning() << tr("无法创建目录：") << defaultDir;
            return;
        }
        if (sessionAutoSaveFile.isEmpty()) {
            QString timeStamp = QDateTime::currentDateTime()
                .toString("yyyyMMddhhmmss");
            sessionAutoSaveFile = dir.filePath(
                QString("autosave_%1.txt").arg(timeStamp)
            );
        }
        fullPath = sessionAutoSaveFile;
    }
    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << tr("自动保存失败：") << fullPath;
        return;
    }
    QTextStream out(&file);
    out << ui->textEdit->toPlainText();
    file.close();

    qDebug() << tr("已自动保存到：") << fullPath;
}

void Widget::changeEvent(QEvent* e)
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

void Widget::closeEvent(QCloseEvent* event)
{
    if (asEnabled && autoSaveDirty)
        onAutoSaveTimeout();

    event->accept();
}

void Widget::onLanguageChanged(const QString& code)
{
    QSettings(ORG, APP).setValue(KEY_LANG, code);
    TranslationManager::instance().switchLanguage(code);
    ui->retranslateUi(this);
    setupToolTips();
    updateFileArrangement();
    updateCharCount();
    updateCursorPosition();
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
        &dlg, [&](const QString&) {
            QEvent ev(QEvent::LanguageChange);
            QCoreApplication::sendEvent(&dlg, &ev);
        });

    if (dlg.exec() == QDialog::Accepted) {
        QSettings s(ORG, APP);
        bool markdownOn = s.value(KEY_MD, false).toBool();
        if (markdownOn && !mdHighlighter) {
            mdHighlighter = new MarkdownHighlighter(ui->textEdit->document());
        }
        else if (!markdownOn && mdHighlighter) {
            delete mdHighlighter;
            mdHighlighter = nullptr;
        }
        dlg.saveShortcuts();
        loadShortcuts();

        dlg.saveAutoSave();
        asEnabled = dlg.autosaveEnabled();
        asInterval = dlg.autosaveInterval();
        asPath = dlg.autosavePath();
        updateAutosaveTimer();
    }
}

void Widget::setupToolTips()
{
    ui->Open->setToolTip(tr("打开文件"));
    ui->Save->setToolTip(tr("保存文件"));
    ui->Reset->setToolTip(tr("撤回"));
    ui->Withdraw->setToolTip(tr("重做"));
    ui->Exit->setToolTip(tr("清空内容"));
    ui->btnOpenSet->setToolTip(tr("设置"));
    ui->fontBox->setToolTip(tr("字体"));
    ui->fileCombobox->setToolTip(tr("编码格式"));
    ui->fileArrangement->setToolTip((tr("打开历史")));
}
