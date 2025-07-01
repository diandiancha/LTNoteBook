#ifndef RESTARTDIALOG_H
#define RESTARTDIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;
class QDialogButtonBox;

class RestartDialog : public QDialog {
    Q_OBJECT
public:
    explicit RestartDialog(QWidget *parent = nullptr);

private:
    QLabel            *m_iconLabel;
    QLabel            *m_textLabel;
    QDialogButtonBox  *m_buttonBox;
};

#endif // RESTARTDIALOG_H
