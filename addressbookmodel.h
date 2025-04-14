#ifndef ADDRESSBOOKMODEL_H
#define ADDRESSBOOKMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include "AddressEntry.h"

class AddressBookModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit AddressBookModel(QObject* parent = nullptr);

    // QAbstractTableModel 필수 구현 함수
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // 커스텀 함수
    void addEntry(const AddressEntry& entry);
    void removeEntry(int row);
    AddressEntry getEntry(int row) const;
    void updateEntry(int row, const AddressEntry& entry);

private:
    QVector<AddressEntry> m_entries;
    bool m_favorite = false;

};

#endif // ADDRESSBOOKMODEL_H
