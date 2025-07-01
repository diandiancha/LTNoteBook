#ifndef SEPARATORDELEGATE_H
#define SEPARATORDELEGATE_H

#include <QStyledItemDelegate>

class SeparatorDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit SeparatorDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

#endif // SEPARATORDELEGATE_H
