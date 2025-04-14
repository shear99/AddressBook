#include "HeartDelegate.h"
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QStyleOptionButton>
#include <QAbstractItemView>

HeartDelegate::HeartDelegate(QObject* parent)
    : QStyledItemDelegate(parent),
    m_checkedIcon(":/asset/asset/yes.png"),
    m_uncheckedIcon(":/asset/asset/no.png")
{
    m_iconSize = QSize(16, 16);  // 아이콘 크기를 작게 설정
}

void HeartDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const
{
    if (index.column() == 0) {
        bool checked = index.model()->data(index, Qt::CheckStateRole).toInt() == Qt::Checked;
        QIcon icon = checked ? m_checkedIcon : m_uncheckedIcon;

        // 셀 중심에 아이콘을 배치
        QRect iconRect = option.rect;
        iconRect.setSize(m_iconSize);
        iconRect.moveCenter(option.rect.center());

        icon.paint(painter, iconRect, Qt::AlignCenter);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

bool HeartDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                                const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (index.column() != 0 || !(event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick))
        return false;

    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

    // 클릭한 좌표가 셀 내부인지 확인
    QRect iconRect = option.rect;
    iconRect.setSize(m_iconSize);
    iconRect.moveCenter(option.rect.center());

    if (!iconRect.contains(mouseEvent->pos()))
        return false;

    // 체크 상태 토글
    bool checked = index.model()->data(index, Qt::CheckStateRole).toInt() == Qt::Checked;
    return model->setData(index, !checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
}
