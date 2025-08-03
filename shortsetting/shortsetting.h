#ifndef SHORTSETTING_H
#define SHORTSETTING_H
#include <QDialog>
#include <QVector>
#include <QTextBrowser>
#include "DailyQuoteManager.h"

struct ShortcutItem {
    QString id;
    QString display;
    QString defSeq;
};

QT_BEGIN_NAMESPACE
namespace Ui { class ShortSetting; }
QT_END_NAMESPACE

class ShortSetting : public QDialog
{
    Q_OBJECT
public:
    explicit ShortSetting(QWidget *parent = nullptr);
    ~ShortSetting();
    void retranslate();

    void saveShortcuts();
    bool originalMarkdownChecked() const { return origMarkdownChecked; }

    bool        autosaveEnabled()  const;
    int         autosaveInterval() const;
    QString     autosavePath()     const;
    void        saveAutoSave();

signals:
    void shortcutUpdated();
    void languageChanged(const QString &langCode);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void editCurrent();
    void resetDefaults();
    void onListWidgetClicked(const QModelIndex &index);

private:
    void loadShortcuts();
    void saveShortcutsToSettings();
    void resetDefaultShortcuts();
    void changeEvent(QEvent *e) override;
    void reloadStaticTexts();

    Ui::ShortSetting *ui;
    QVector<ShortcutItem> m_items;

    bool origMdChecked = false;
    bool m_blockInit   = false;

    bool origAsEnabled = false;

    bool origMarkdownChecked = false;
    bool m_initializing = false;

    int m_lastIndex = -1;
    QFont m_defaultFont;
    QFont m_highlightFont;
    QTextBrowser *m_textBrowserQuote;
    DailyQuoteManager *m_quoteManager;

};

#endif // SHORTSETTING_H
