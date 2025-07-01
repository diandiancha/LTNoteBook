#include "RestartDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QPixmap>
#include "RestartDialog.h"
#include <QLabel>
#include <QIcon>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

RestartDialog::RestartDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(280, 200);

    setStyleSheet(R"(
        RestartDialog {
            background-color: #1e1e1e;
            border: 2px solid #08FDD8;
            border-radius: 6px;
        }
        QLabel#textLabel {
            color: #E0E0E0;
            font-size: 15px;
        }
        QPushButton {
            background-color: #08FDD8;
            color: #1e1e1e;
            min-width: 70px;
            height: 30px;
            border: none;
            border-radius: 4px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #3afee0;
        }
        QPushButton:pressed {
            background-color: #06bfa0;
        }
    )");

    auto *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(16, 20, 16, 16);
    mainLay->setSpacing(12);

    m_iconLabel = new QLabel(this);
    QPixmap px(":/icon/256.ico");
    m_iconLabel->setPixmap(px.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_iconLabel->setAlignment(Qt::AlignCenter);
    mainLay->addWidget(m_iconLabel, 0, Qt::AlignHCenter);

    mainLay->addWidget(m_iconLabel, 0, Qt::AlignHCenter);

    m_textLabel = new QLabel(this);
    m_textLabel->setObjectName("textLabel");
    m_textLabel->setWordWrap(true);
    m_textLabel->setText(tr(
        "启用/关闭 Markdown 格式\n"
        "需要重启应用才能生效。\n"
        "是否立即重启？"
        ));
    m_textLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    m_buttonBox = new QDialogButtonBox(this);
    QPushButton *okBtn     = m_buttonBox->addButton(tr("确定"), QDialogButtonBox::AcceptRole);
    QPushButton *cancelBtn = m_buttonBox->addButton(tr("取消"), QDialogButtonBox::RejectRole);
    m_buttonBox->setCenterButtons(true);
    connect(okBtn,     &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    mainLay->setContentsMargins(16, 10, 16, 16);
    mainLay->setSpacing(12);
    mainLay->addWidget(m_iconLabel,  0, Qt::AlignHCenter);
    mainLay->addWidget(m_textLabel);
    mainLay->addStretch(1);
    mainLay->addWidget(m_buttonBox);
}

