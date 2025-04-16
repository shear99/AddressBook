#ifndef HEARTDELEGATE_H
#define HEARTDELEGATE_H

#include <QStyledItemDelegate>
#include <QIcon>
#include <QSize>

// Custom delegate for displaying and handling heart/favorite icons in a table view
// Used to represent the favorite status of contacts in the address book
class HeartDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit HeartDelegate(QObject* parent = nullptr);

    // Paints the heart icon in the table cell
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    // Handles mouse events for toggling the favorite status
    bool editorEvent(QEvent* event, QAbstractItemModel* model,
                     const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    QIcon m_checkedIcon;
    QIcon m_uncheckedIcon;
    QSize m_iconSize;
};

#endif // HEARTDELEGATE_H
