#ifndef WIDGET_H
#define WIDGET_H

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QFont>
#include <QFontComboBox>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMap>
#include <QProcess>
#include <QPushButton>
#include <QStringDecoder>
#include <QStandardPaths>
#include <QSettings>
#include <QStringList>
#include <QShortcut>
#include <QStringConverter>
#include <QTimer>
#include <QTranslator>
#include <QTextEdit>
#include <QTextStream>
#include <QWidget>

#include "MarkdownHighlighter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget* parent = nullptr);
    ~Widget();
    void retranslate();

public:
    void openFileFromPath(const QString& path);
    void resetToDefaultShortcuts();

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* e) override;

private slots:
    void updateEncodingSelection(const QString& path);
    void on_Open_clicked();
    void on_Save_clicked();
    void on_Exit_clicked();
    void on_fileArrangement_activated(int index);
    void on_fileCombobox_activated(int index);
    void on_fontBox_currentFontChanged(const QFont& f);
    void updateCharCount();
    void updateCursorPosition();
    void on_Reset_clicked();
    void on_Withdraw_clicked();
    void on_btnOpenSet_clicked();
    void onAutoSaveTimeout();
    void onLanguageChanged(const QString& code);

private:
    void updateFileArrangement();
    static QStringConverter::Encoding encodingFromString(const QString& s);
    static QString encodingToString(QStringConverter::Encoding e);
    void loadShortcuts();
    bool eventFilter(QObject* watched, QEvent* event) override;
    bool isUsingCustomShortcut(int key);
    void updateAutosaveTimer();
    void setupToolTips();

    bool isLikelyText(const QString& text);
    QStringConverter::Encoding tryFallbackEncoding(const QByteArray& data, QStringConverter::Encoding currentEncoding);
    QStringConverter::Encoding detectEncoding(const QByteArray& data);
    QStringConverter::Encoding detectUtf16ByteOrder(const QByteArray& data);
    bool isLikelyUtf16(const QByteArray& data);

private:
    Ui::Widget* ui;
    bool isCustomShortcuts = false;

    QMap<QString, QString> openedFiles;
    QMap<QString, QStringConverter::Encoding> fileEncodings;
    QStringList fileOrder;
    QString currentFilePath;
    bool isNewUnsavedEdit = false;

    QStringList fileHistory;
    int fileHistoryPos = -1;
    bool m_ignoreFileArrangement = false;

    MarkdownHighlighter* mdHighlighter = nullptr;

    QShortcut* scOpen = nullptr;
    QShortcut* scSave = nullptr;
    QShortcut* scUndo = nullptr;
    QShortcut* scRedo = nullptr;

    bool asEnabled = false;
    bool autoSaveDirty = false;
    int asInterval = 30;
    QString asPath;
    QString sessionAutoSaveFile;
    QTimer autosaveTimer;
    QTimer debounceTimer;
};

#endif
