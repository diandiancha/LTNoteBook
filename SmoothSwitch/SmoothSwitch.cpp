#include "SmoothSwitch.h"
#include <QPainter>
#include <QMouseEvent>

SmoothSwitch::SmoothSwitch(QWidget *parent)
    : QCheckBox(parent)
    , m_anim(new QPropertyAnimation(this, "offset", this))
{
    m_anim->setDuration(200);
    m_anim->setEasingCurve(QEasingCurve::InOutQuad);

    setCursor(Qt::PointingHandCursor);
    setChecked(false);

    connect(this, &QCheckBox::toggled, this, [this](bool checked){
        m_anim->stop();
        m_anim->setStartValue(m_offset);
        m_anim->setEndValue(checked ? 1.0 : 0.0);
        m_anim->start();
    });
}

void SmoothSwitch::setOffset(qreal v)
{
    m_offset = v;
    update();
}

QSize SmoothSwitch::sizeHint() const
{
    return QSize(64, 36);
}

void SmoothSwitch::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int W = width(), H = height();
    const int trackH = 28, trackY = (H - trackH) / 2, trackR = trackH / 2;
    const int knobR = trackR - 3;

    QRectF trackRect(0, trackY, W, trackH);
    p.setPen(Qt::NoPen);
    p.setBrush(isChecked() ? QColor(0,255,255) : QColor(120,120,120));
    p.drawRoundedRect(trackRect, trackR, trackR);

    qreal minX = 2, maxX = W - 2*knobR - 2;
    qreal x = minX + m_offset * (maxX - minX);
    QRectF knobRect(x, trackY + 2, 2*knobR, 2*knobR);
    p.setBrush(Qt::white);
    p.drawEllipse(knobRect);
}

void SmoothSwitch::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && rect().contains(e->pos())) {
        toggle();
        e->accept();
        return;
    }
    QCheckBox::mousePressEvent(e);
}
