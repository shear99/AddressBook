#ifndef HEARTDELEGATE_H
#define HEARTDELEGATE_H

#include <QStyledItemDelegate>
#include <QIcon>
#include <QSize>

class HeartDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit HeartDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    bool editorEvent(QEvent* event, QAbstractItemModel* model,
                     const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    QIcon m_checkedIcon;
    QIcon m_uncheckedIcon;
    QSize m_iconSize;
};

#endif // HEARTDELEGATE_H
