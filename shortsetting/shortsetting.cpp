#include "shortsetting.h"
#include "ui_shortsetting.h"
#include "RestartDialog.h"
#include "SeparatorDelegate.h"
#include "config.h"
#include <QCheckBox>
#include <limits>
#include <QSettings>
#include <QKeySequenceEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QTableWidgetItem>
#include <QScrollBar>
#include <QTranslator>
#include <QMessageBox>
#include <QMouseEvent>
#include <QFileDialog>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QLineEdit>
#include <QFontMetrics>
#include <QGraphicsDropShadowEffect>
#include <QTimer>

ShortSetting::ShortSetting(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ShortSetting)
{
    ui->setupUi(this);
    ui->textBrowserQuote->setOpenExternalLinks(true);
    ui->textBrowserQuote->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_quoteManager = new DailyQuoteManager(ui->textBrowserQuote, this);

    m_quoteManager->refresh();

    QScrollBar *vbar = ui->textBrowserQuote->verticalScrollBar();
    vbar->setStyleSheet(R"(
    /* 整个滚动条槽背景透明 */
    QScrollBar:vertical {
        background: transparent;
        width: 10px;
        margin: 0px;
    }
    /* 滑块（handle）样式 */
    QScrollBar::handle:vertical {
        background: #08FDD8;
        height: 30px;
        border-radius: 4px;
    }
    /* 禁用上下箭头 */
    QScrollBar::sub-line:vertical,
    QScrollBar::add-line:vertical {
        height: 0px;
    }
    /* 拖动槽上下侧无特殊背景 */
    QScrollBar::sub-page:vertical,
    QScrollBar::add-page:vertical {
        background: none;
    }
)");


    ui->pageAbout->setStyleSheet(R"(
        QWidget#pageAbout {
            background-color: #1e1e1e;
            border: 2px solid #08FDD8;
            border-radius: 8px;
            padding: 16px;
        }
    )");

    ui->textBrowserLinks->setFrameShape(QFrame::NoFrame);
    ui->textBrowserLinks->setOpenExternalLinks(true);
    ui->textBrowserLinks->setHtml(
        QString(R"(
        <div style="text-align:center; font-size:9pt; color:#08FDD8;">
          <a href="https://github.com/diandiancha/LTNoteBook">%1</a><br>
          <a href="https://opensource.org/licenses/MIT">%2</a><br>
          <a href="https://github.com/diandiancha/LTNoteBook">%3</a>
        </div>
    )")
            .arg(tr("项目主页"))
            .arg(tr("MIT 许可证"))
            .arg(tr("反馈与建议"))
        );

    ui->textBrowserLinks->setStyleSheet(R"(
        border:none;
        background:transparent;
        margin-top:8px;
    )");

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setOffset(0, 0);
    shadow->setColor(QColor(0,0,0,100));
    ui->frame->setGraphicsEffect(shadow);

    ui->comboBoxLanguage->addItem(tr("Chinese"),"zh_CN");
    ui->comboBoxLanguage->addItem(tr("English"),"en");
    ui->comboBoxLanguage->addItem(tr("Japanese"),"ja");

    m_initializing = true;
    QSettings s(ORG,APP);

    QString lastCode = s.value(KEY_LANG, "zh_CN").toString();
    int idx = ui->comboBoxLanguage->findData(lastCode, Qt::UserRole, Qt::MatchExactly);
    if (idx >= 0) {
        ui->comboBoxLanguage->blockSignals(true);
        ui->comboBoxLanguage->setCurrentIndex(idx);
        ui->comboBoxLanguage->blockSignals(false);
    }


    origMarkdownChecked = s.value(KEY_MD, false).toBool();
    ui->switchMarkdown->setChecked(origMarkdownChecked);
    m_initializing = false;

    connect(ui->switchMarkdown, &SmoothSwitch::toggled, this, [this](bool checked) {
        if (m_initializing)
            return;

        RestartDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            QSettings s2(ORG, APP);
            s2.setValue(KEY_MD, checked);
#if defined(Q_OS_WIN)
            QString program = QCoreApplication::applicationFilePath();
            QProcess::startDetached(program, {});
#endif
            QCoreApplication::quit();
        } else {
            m_initializing = true;
            ui->switchMarkdown->setChecked(origMarkdownChecked);
            m_initializing = false;
            ui->switchMarkdown->setEnabled(false);
            QTimer::singleShot(500, ui->switchMarkdown, [this]{
                ui->switchMarkdown->setEnabled(true);
            });
        }
    });

    bool savedOn = s.value(KEY_AE, false).toBool();
    QString savedPath = s.value(KEY_AP).toString();
    if (savedPath.isEmpty()) {
        savedPath = QStandardPaths::writableLocation(
            QStandardPaths::DesktopLocation);
    }
    int savedInterval = s.value(KEY_AT, 30).toInt();

    ui->switchAutoSave->setChecked(savedOn);
    ui->lineEditautoSavePath->setText(savedPath);
    ui->spinBoxinterval->setRange(1, std::numeric_limits<int>::max());
    ui->spinBoxinterval->setValue(savedInterval);

    ui->lineEditautoSavePath->setEnabled(savedOn);
    ui->btnbrowsePath->setEnabled(savedOn);
    ui->spinBoxinterval->setEnabled(savedOn);

    connect(ui->switchAutoSave, &SmoothSwitch::toggled, this,
            [this](bool checked){
                ui->lineEditautoSavePath->setEnabled(checked);
                ui->btnbrowsePath->setEnabled(checked);
                ui->spinBoxinterval->setEnabled(checked);
            });

    connect(ui->btnbrowsePath, &QPushButton::clicked, this, [this](){
        QString dir = QFileDialog::getExistingDirectory(
            this, tr("选择保存目录"),
            ui->lineEditautoSavePath->text());
        if (!dir.isEmpty())
            ui->lineEditautoSavePath->setText(dir);
    });

    connect(ui->btnOk, &QPushButton::clicked, this, [this](){
        saveShortcuts();
        saveAutoSave();
        accept();
    });
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->listWidgetNav, &QListWidget::clicked, this, &ShortSetting::onListWidgetClicked);

    m_items = {
        { "Shortcut/Open",  tr("打开文件"), "Ctrl+O" },
        { "Shortcut/Save",  tr("保存文件"), "Ctrl+S" },
        { "Shortcut/Undo",  tr("撤销"),     "Ctrl+Z" },
        { "Shortcut/Redo",  tr("重做"),     "Ctrl+Y" }
    };

    ui->tblShortcuts->setColumnCount(2);
    ui->tblShortcuts->setHorizontalHeaderLabels({ tr("操作"), tr("快捷键") });
    ui->tblShortcuts->horizontalHeader()->setStretchLastSection(true);
    ui->tblShortcuts->verticalHeader()->hide();
    ui->tblShortcuts->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tblShortcuts->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto hdr = ui->tblShortcuts->horizontalHeader();
    hdr->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(1, QHeaderView::Stretch);

    ui->tblShortcuts->setItemDelegate(new SeparatorDelegate(ui->tblShortcuts));
    ui->tblShortcuts->setStyleSheet(R"(
    QTableWidget#tblShortcuts {
        font: 12pt "Microsoft YaHei UI";
        border: 1px solid #18e2cf;
        background-color: #2c3e50;
        border-radius: 11px;
        padding: 4px 8px 8px 8px;
        gridline-color: transparent;
    }
    QTableWidget#tblShortcuts QHeaderView::section {
        background-color: #232931;
        color: #ffffff;
        font-size: 10pt;
        font-weight: bold;
        padding: 8px 12px;
        border: none;
    }
    QTableWidget#tblShortcuts QHeaderView::section:first {
        border-top-left-radius: 8px;
    }
    QTableWidget#tblShortcuts QHeaderView::section:last {
        border-top-right-radius: 8px;
    }
    QTableWidget#tblShortcuts QTableWidget::item {
        background-color: #232931;
        color: #ffffff;
        font-size: 9pt;
        padding: 6px 8px;
        border: none;
        border-bottom: 1px solid #505050;
    }
    QTableWidget#tblShortcuts QTableWidget::item:selected {
        background-color: rgb(60,60,60);
    }
)");

    loadShortcuts();

    connect(ui->listWidgetNav, &QListWidget::currentRowChanged,
            ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this,
            [this](int i){ ui->listWidgetNav->setCurrentRow(i); });
    ui->listWidgetNav->setCurrentRow(0);

    connect(ui->btnEdit, &QPushButton::clicked, this, &ShortSetting::editCurrent);
    connect(ui->btnSave, &QPushButton::clicked, this, &ShortSetting::saveShortcuts);
    connect(ui->btnResetDefault, &QPushButton::clicked, this, &ShortSetting::resetDefaults);

    connect(ui->comboBoxLanguage,
            QOverload<int>::of(&QComboBox::activated),
            this, [this](int index){
                if (m_initializing) return;
                QString code = ui->comboBoxLanguage->itemData(index).toString();
                emit languageChanged(code);
                accept();
            });

    ui->listWidgetNav->setStyleSheet(R"(
    QListWidget#listWidgetNav {
        background-color: #232931;
        border-top:    2px solid #08FDD8;
        border-left:   2px solid #08FDD8;
        border-right:   2px solid #08FDD8;
        border-bottom: 2px solid #08FDD8;
        border-top-left-radius:     10px;
        border-bottom-left-radius:  10px;
        border-top-right-radius:     10px;
        border-bottom-right-radius:  10px;
    }
    QListWidget#listWidgetNav {
        background-color: #232931;
        border-top-left-radius:     10px;
        border-bottom-left-radius:  10px;
    }
    QListWidget#listWidgetNav::item {
        padding: 8px 12px;   /* 上下 6px，左右 12px */
    }
    QListWidget#listWidgetNav::item:selected {
        background-color: rgba(100, 100, 100, 0.3);
    }
)");

    m_defaultFont   = ui->listWidgetNav->font();
    m_defaultFont.setPointSize(11);
    m_highlightFont = m_defaultFont;
    m_highlightFont.setPointSize(m_defaultFont.pointSize() + 2);

    ui->listWidgetNav->viewport()->installEventFilter(this);

    ui->listWidgetNav->setCurrentRow(0);

    if (ui->listWidgetNav->count() > 0) {
        auto *itm = ui->listWidgetNav->item(0);
        itm->setFont(m_highlightFont);
        m_lastIndex = 0;
    }


    setFixedSize(510, 400);
}

ShortSetting::~ShortSetting()
{
    delete ui;
}

bool ShortSetting::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->listWidgetNav->viewport()
        && event->type() == QEvent::MouseButtonPress)
    {
        auto *me = static_cast<QMouseEvent*>(event);
        QModelIndex idx = ui->listWidgetNav->indexAt(me->pos());

        if (m_lastIndex >= 0) {
            auto *oldItem = ui->listWidgetNav->item(m_lastIndex);
            oldItem->setFont(m_defaultFont);
        }

        if (idx.isValid()) {
            int row = idx.row();
            ui->listWidgetNav->setCurrentRow(row);

            auto *itm = ui->listWidgetNav->item(row);
            itm->setFont(m_highlightFont);
            m_lastIndex = row;
        } else {
            ui->listWidgetNav->clearSelection();
            m_lastIndex = -1;
        }
        return true;
    }
    return QDialog::eventFilter(watched, event);
}

void ShortSetting::onListWidgetClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        ui->listWidgetNav->clearSelection();
    } else {
        ui->listWidgetNav->setCurrentItem(ui->listWidgetNav->item(index.row()));
    }
}

void ShortSetting::reloadStaticTexts()
{
    ui->listWidgetNav->clear();
    ui->listWidgetNav->addItem(tr("Shortcuts"));
    ui->listWidgetNav->addItem(tr("Editor"));
    ui->listWidgetNav->addItem(tr("Language"));
    ui->listWidgetNav->addItem(tr("About"));
    ui->listWidgetNav->setCurrentRow(0);

    ui->tblShortcuts->setHorizontalHeaderLabels({ tr("操作"), tr("快捷键") });
    ui->btnOk->setText(tr("确定"));
    ui->btnCancel->setText(tr("取消"));
}


void ShortSetting::loadShortcuts()
{
    QSettings s(ORG, APP);
    ui->tblShortcuts->setRowCount(m_items.size());
    for (int r = 0; r < m_items.size(); ++r) {
        QString op = QCoreApplication::translate(
            "ShortSetting", m_items[r].display.toUtf8().constData());
        ui->tblShortcuts->setItem(r, 0, new QTableWidgetItem(op));

        QString seq = s.value(m_items[r].id, m_items[r].defSeq).toString();
        ui->tblShortcuts->setItem(r, 1, new QTableWidgetItem(seq));
    }
}

void ShortSetting::saveShortcuts()
{
    QSettings s(ORG, APP);

    for (int r = 0; r < m_items.size(); ++r) {
        s.setValue(m_items[r].id, ui->tblShortcuts->item(r, 1)->text());
    }

    emit shortcutUpdated();

    s.sync();
}


void ShortSetting::editCurrent()
{
    int row = ui->tblShortcuts->currentRow();
    if (row < 0) return;

    QDialog dlg(this);
    dlg.setWindowTitle(tr("快捷键设置"));

    dlg.setStyleSheet(R"(
        QDialog {
            background-color: #2c3e50;
            border-radius: 10px;
            padding: 15px;
            min-width: 400px;
            min-height: 160px;
        }
        QKeySequenceEdit {
            background-color: #232931;
            color: white;
            border-radius: 6px;
            padding: 6px;
            font-size: 12pt;
            height: 35px; /* 控制高度 */
        }
        QPushButton {
            background-color: rgb(0,0,0);
            color: #18e2cf;
            border-radius: 8px;
            padding: 5px 16px;  /* 更小的内边距 */
            font-size: 12pt;
            min-width: 80px;    /* 控制按钮宽度 */
            height: 30px;      /* 控制按钮高度 */
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.2);
        }
        QPushButton:hover {
            background-color: #1e878f;
        }
        QPushButton:pressed {
            background-color: #146e77;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
        }
    )");

    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    QKeySequenceEdit *kse = new QKeySequenceEdit(&dlg);
    kse->setKeySequence(QKeySequence(ui->tblShortcuts->item(row, 1)->text()));
    lay->addWidget(kse);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    lay->addWidget(bb);

    bb->setStyleSheet("QPushButton {"
                      "    background-color: #18e2cf;"
                      "    border-radius: 8px;"
                      "    color: white;"
                      "    padding: 3px 6px;"
                      "    font-size: 12pt;"
                      "    min-width: 80px;"
                      "    height: 20px;"
                      "    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.2);"
                      "}"
                      "QPushButton:hover {"
                      "    background-color: #1e878f;"
                      "}"
                      "QPushButton:pressed {"
                      "    background-color: #146e77;"
                      "    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);"
                      "}");

    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        ui->tblShortcuts->item(row, 1)->setText(
            kse->keySequence().toString(QKeySequence::NativeText));
        emit shortcutUpdated();
    }
}



void ShortSetting::resetDefaults()
{
    for (int r = 0; r < m_items.size(); ++r) {
        ui->tblShortcuts->item(r, 1)->setText(m_items[r].defSeq);
    }

    emit shortcutUpdated();

}

bool ShortSetting::autosaveEnabled() const
{
    return ui->switchAutoSave->isChecked();
}

int ShortSetting::autosaveInterval() const
{
    return ui->spinBoxinterval->value();
}

QString ShortSetting::autosavePath() const
{
    return ui->lineEditautoSavePath->text();
}

void ShortSetting::saveAutoSave()
{
    QSettings s(ORG, APP);
    s.setValue(KEY_AE,   autosaveEnabled());
    s.setValue(KEY_AP,      autosavePath());
    s.setValue(KEY_AT,  autosaveInterval());
    s.sync();
}

void ShortSetting::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        loadShortcuts();
    }
    QDialog::changeEvent(e);
}

void ShortSetting::retranslate()
{
    ui->retranslateUi(this);
}




