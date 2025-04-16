#include "HeartDelegate.h"
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QStyleOptionButton>
#include <QAbstractItemView>

HeartDelegate::HeartDelegate(QObject* parent)
    : QStyledItemDelegate(parent),
    m_checkedIcon(":/asset/asset/yes.png"),    // Icon for favorite contacts
    m_uncheckedIcon(":/asset/asset/no.png")    // Icon for non-favorite contacts
{
    m_iconSize = QSize(16, 16);  // Set small icon size for table cells
}

// Custom painting of heart icons in the first column
void HeartDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const
{
    if (index.column() == 0) {
        // Get the favorite status from the model
        bool checked = index.model()->data(index, Qt::CheckStateRole).toInt() == Qt::Checked;
        QIcon icon = checked ? m_checkedIcon : m_uncheckedIcon;

        // Center the icon in the cell
        QRect iconRect = option.rect;
        iconRect.setSize(m_iconSize);
        iconRect.moveCenter(option.rect.center());

        // Paint the icon
        icon.paint(painter, iconRect, Qt::AlignCenter);
    } else {
        // Use default painting for other columns
        QStyledItemDelegate::paint(painter, option, index);
    }
}

// Handle mouse clicks to toggle favorite status
bool HeartDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                                const QStyleOptionViewItem& option, const QModelIndex& index)
{
    // Only handle clicks in the first column
    if (index.column() != 0 || !(event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick))
        return false;

    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

    // Check if click was within the icon area
    QRect iconRect = option.rect;
    iconRect.setSize(m_iconSize);
    iconRect.moveCenter(option.rect.center());

    if (!iconRect.contains(mouseEvent->pos()))
        return false;

    // Toggle the favorite status
    bool checked = index.model()->data(index, Qt::CheckStateRole).toInt() == Qt::Checked;
    return model->setData(index, !checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
}
