#ifndef MULTICOLUMNFILTERPROXYMODEL_H
#define MULTICOLUMNFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QRegularExpression>

class MultiColumnFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit MultiColumnFilterProxyModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent) {}

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        for (int col = 1; col <= 5; ++col) { // 1~5열만 검색
            QModelIndex idx = sourceModel()->index(sourceRow, col, sourceParent);
            QString data = sourceModel()->data(idx, filterRole()).toString();
            if (data.contains(filterRegularExpression()))
                return true;
        }
        return false;
    }
};
#endif // MULTICOLUMNFILTERPROXYMODEL_H
