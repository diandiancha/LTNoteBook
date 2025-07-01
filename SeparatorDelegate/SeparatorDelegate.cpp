#include "SeparatorDelegate.h"
#include <QPainter>
#include <QAbstractItemModel>

void SeparatorDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    painter->save();
    QPen pen(QColor(80, 80, 80));
    pen.setWidth(1);
    painter->setPen(pen);

    QRect r = option.rect;
    int y = r.bottom() - 2;
    int x1 = r.left() + 8;
    int x2 = r.right() - 8;

    QRect clipRect(x1, r.top(), x2 - x1, r.height());
    painter->setClipRect(clipRect);

    painter->drawLine(x1, y, x2, y);
    painter->restore();
}
