#ifndef WIDGET_H
#define WIDGET_H

// Qt核心组件
#include <QWidget>
#include <QApplication>
#include <QTimer>
#include <QSettings>
#include <QStringConverter>

// Qt界面组件
#include <QComboBox>
#include <QFontComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QShortcut>

// Qt文件和IO
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QStringDecoder>

// Qt数据结构
#include <QMap>
#include <QStringList>
#include <QString>
#include <QDateTime>

// Qt系统和工具
#include <QStandardPaths>
#include <QProcess>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QFileSystemWatcher>

// 自定义组件
#include "MarkdownHighlighter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

/**
 * @brief 主窗口类 - LTNoteBook 记事本应用
 *
 * 功能特性：
 * - 多文件管理和历史记录
 * - 多种编码格式支持
 * - Markdown语法高亮
 * - 自动保存和智能备份
 * - 自定义快捷键
 * - 多语言支持
 * - 程序目录优先的备份策略
 */
class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

    void retranslate();

public:
    // 外部文件打开接口
    void openFileFromPath(const QString &path);
    void resetToDefaultShortcuts();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *e) override;

private slots:
    // === 文件操作槽函数 ===
    void on_Open_clicked();                     // 打开文件
    void on_Save_clicked();                     // 保存文件
    void on_Delete_clicked();                   // 删除文件
    void on_Exit_clicked();                     // 关闭文件

    // === 界面交互槽函数 ===
    void on_fileArrangement_activated(int index);      // 文件历史选择
    void on_fileCombobox_activated(int index);         // 编码格式切换
    void on_fontBox_currentFontChanged(const QFont &f); // 字体切换
    void on_btnOpenSet_clicked();                       // 打开设置

    // === 编辑操作槽函数 ===
    void on_Reset_clicked();                    // 撤销
    void on_Withdraw_clicked();                 // 重做
    void clearCurrentFile();                    //通过快捷键撤除当前文件
    void deleteCurrentFileByShortcut();         //通过快捷键删除当前文件
    void updateCharCount();                     // 更新字符数
    void updateCursorPosition();                // 更新光标位置

    // === 系统功能槽函数 ===
    void onLanguageChanged(const QString &code); // 语言切换
    void updateEncodingSelection(const QString &path); // 更新编码选择

public slots:
    // === 文件夹管理功能（供设置界面调用） ===
    void openBackupFolder();                   // 打开备份文件夹
    void openAutoSaveFolder();                 // 打开自动保存文件夹

private:
    // === 文件管理功能 ===
    void updateFileArrangement();               // 更新文件历史显示
    void addToHistory(const QString &path);     // 添加到历史记录
    void cleanupMemoryIfNeeded();               // 内存清理
    void updateWindowTitle();                   // 更新窗口标题
    void updateUndoRedoButtons();
    void checkReadmeIntegrity(const QString &backupDir) const;
    void cleanupBackupFolder();
    bool isReadmeFile(const QString &filePath) const;

    // === 编码处理功能 ===
    static QStringConverter::Encoding encodingFromString(const QString& s);
    static QString encodingToString(QStringConverter::Encoding e);
    QStringConverter::Encoding detectEncodingImproved(const QByteArray &data);
    bool isBinaryContent(const QByteArray &data) const;
    void on_fileCombobox_currentTextChanged(const QString &text);

    // === 自动保存核心功能 ===
    QString getBackupDirectory() const;         // 获取备份目录（程序目录优先）
    void updateAutosaveTimer();                 // 更新自动保存定时器
    void onAutoSaveTimeout();                   // 自动保存超时处理

    // === 状态显示功能 ===
    void showAutoSaveStatus(bool success, const QString &fileName);
    void showAutoBackupStatus(bool success, const QString &fileName);

    // === 具体的保存和备份功能 ===
    void saveToOriginalFile(const QString &content);           // 保存到原文件
    void performSmartBackup(const QString &content);           // 执行智能备份
    void saveNewDocumentToTemp(const QString &content);        // 保存新文档到临时位置
    void createNewDocumentBackup(const QString &content);      // 创建新文档备份
    void cleanupOldNewDocumentBackups(const QString &backupDir) const; // 清理旧的新文档备份

    // === 智能备份功能 ===
    void createInitialBackup(const QString &filePath, const QString &content);  // 创建初始备份
    void updateBackupIfChanged(const QString &filePath, const QString &currentContent); // 检查并更新备份
    QString getBackupFileName(const QString &originalPath) const;  // 获取备份文件名
    void cleanupOldBackups(const QString &originalPath) const;     // 清理旧备份
    void cleanupTemporaryFiles();  // 清理临时文件
    void cleanupExcessiveBackups() const;

    // === 说明文件创建功能 ===
    void createProgramDirReadme(const QString &backupDir) const;   // 创建程序目录说明文件
    QString getReadmeFileName() const;  // 获取多语言说明文件名
    void cleanupOldReadmeFiles(const QString &backupDir) const;  // 清理旧的说明文件

    // === 系统功能 ===
    void loadShortcuts();                       // 加载快捷键
    void setupToolTips();                       // 设置工具提示
    bool checkUnsavedChanges();                 // 检查未保存修改
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool isUsingCustomShortcut(int key);

    // === 测试和调试功能 ===
    void testBackupFunction();                  // 测试备份功能

private:
    // === Windows默认应用支持 ===
#ifdef Q_OS_WIN
private:
    void registerFileAssociations();
    void unregisterFileAssociations();
    void registerSingleFileType(const QString &extension, const QString &appPath);
    void addContextMenuEntry();
    void removeContextMenuEntry();
#endif

    // === 界面组件 ===
    Ui::Widget *ui;
    MarkdownHighlighter *mdHighlighter = nullptr;  // Markdown高亮器

    // === 状态控制变量 ===
    bool m_ignoreEncodingChange = false;        // 忽略编码切换信号
    bool m_ignoreFileArrangement = false;       // 忽略文件排列信号
    bool isCustomShortcuts = false;             // 是否使用自定义快捷键
    bool isNewUnsavedEdit = false;              // 是否为新的未保存编辑
    bool m_readmeEncodingWarningShown = false;  // 记录是否已经显示过警告

    // === 文件管理数据 ===
    QMap<QString, QString> openedFiles;         // 已打开文件内容缓存
    QMap<QString, QStringConverter::Encoding> fileEncodings; // 文件编码映射
    QStringList fileOrder;                      // 文件打开顺序
    QStringList fileHistory;                    // 文件历史记录（最大15个）
    QString currentFilePath;                    // 当前文件路径
    int fileHistoryPos = -1;                    // 历史记录位置
    QMap<QString, QByteArray> fileRawData;  // 保存文件的原始字节数据

    // === 快捷键管理 ===
    QShortcut *scOpen = nullptr;                // 打开文件快捷键
    QShortcut *scSave = nullptr;                // 保存文件快捷键
    QShortcut *scClear = nullptr;               // 清除当前文件快捷键
    QShortcut *scDeleteFile = nullptr;          // 删除当前文件快捷键
    QShortcut *scUndo = nullptr;                // 撤销快捷键
    QShortcut *scRedo = nullptr;                // 重做快捷键

    // === 自动保存配置 ===
    bool asEnabled = false;                     // 自动保存是否启用
    bool autoSaveDirty = false;                 // 内容是否有修改需要自动保存
    int asInterval = 30;                        // 自动保存间隔（秒）
    QString asPath;                             // 自动保存路径
    QTimer autosaveTimer;                       // 自动保存定时器
    QTimer cleanupTimer;                        // 定时清理临时文件
    QTimer debounceTimer;                       // 防抖动定时器

    // === 智能备份相关变量 ===
    QString m_initialFileContent;               // 文件打开时的初始内容
    bool m_hasInitialBackup = false;            // 是否已创建初始备份
    bool m_smartBackupEnabled = true;           // 智能备份功能开关（默认开启）
};

#endif // WIDGET_H
