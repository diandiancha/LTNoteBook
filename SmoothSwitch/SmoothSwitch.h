#pragma once
#include <QCheckBox>
#include <QPropertyAnimation>

class SmoothSwitch : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY(qreal offset READ offset WRITE setOffset)

public:
    explicit SmoothSwitch(QWidget *parent = nullptr);
    QSize sizeHint() const override;

    qreal offset() const { return m_offset; }
    void setOffset(qreal v);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    qreal               m_offset = 0;
    QPropertyAnimation *m_anim;
};
