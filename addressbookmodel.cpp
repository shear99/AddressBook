#include <QCoreApplication>
#include <QDir>
#include "AddressBookModel.h"
#include "util.h"

AddressBookModel::AddressBookModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    // AWS Lambda를 통해 주소록 데이터를 로드
    m_entries = loadAddressBookFromAWS(getAwsLoadUrl());
}

int AddressBookModel::rowCount(const QModelIndex& /*parent*/) const {
    return m_entries.size();
}

int AddressBookModel::columnCount(const QModelIndex& /*parent*/) const {
    return 6; // name, phone, email, company, position, nickname
}

QVariant AddressBookModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_entries.size() || role != Qt::DisplayRole)
        return QVariant();

    const AddressEntry& entry = m_entries.at(index.row());

    switch (index.column()) {
    case 0: return entry.name();
    case 1: return entry.phoneNumber();
    case 2: return entry.email();
    case 3: return entry.company();
    case 4: return entry.position();
    case 5: return entry.nickname();
    default: return QVariant();
    }
}

QVariant AddressBookModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "Name";
        case 1: return "Phone";
        case 2: return "Email";
        case 3: return "Company";
        case 4: return "Position";
        case 5: return "Nickname";
        default: return QVariant();
        }
    } else {
        return section + 1;
    }
}

bool AddressBookModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    AddressEntry& entry = m_entries[index.row()];
    switch (index.column()) {
    case 0: entry.setName(value.toString()); break;
    case 1: entry.setPhoneNumber(value.toString()); break;
    case 2: entry.setEmail(value.toString()); break;
    case 3: entry.setCompany(value.toString()); break;
    case 4: entry.setPosition(value.toString()); break;
    case 5: entry.setNickname(value.toString()); break;
    default: return false;
    }

    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});

    if (!saveAddressBookToAWS(m_entries, getAwsSaveUrl())) {
        qWarning("Failed to save data to AWS.");
    }

    return true;
}

Qt::ItemFlags AddressBookModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

void AddressBookModel::addEntry(const AddressEntry& entry) {
    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size());
    m_entries.append(entry);
    endInsertRows();

    if (!saveAddressBookToAWS(m_entries, getAwsSaveUrl())) {
        qWarning("Failed to save data to AWS.");
    }
}

void AddressBookModel::removeEntry(int row) {
    if (row < 0 || row >= m_entries.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_entries.removeAt(row);
    endRemoveRows();

    if (!saveAddressBookToAWS(m_entries, getAwsSaveUrl())) {
        qWarning("Failed to save data to AWS.");
    }
}

AddressEntry AddressBookModel::getEntry(int row) const {
    if (row < 0 || row >= m_entries.size())
        return AddressEntry();
    return m_entries.at(row);
}

void AddressBookModel::updateEntry(int row, const AddressEntry& entry) {
    if (row < 0 || row >= m_entries.size())
        return;

    m_entries[row] = entry;
    emit dataChanged(index(row, 0), index(row, columnCount() - 1));

    if (!saveAddressBookToAWS(m_entries, getAwsSaveUrl())) {
        qWarning("Failed to save data to AWS.");
    }
}

void AddressBookModel::setEntries(const QList<AddressEntry>& entries) {
    beginResetModel();
    m_entries = entries;
    endResetModel();

    if (!saveAddressBookToAWS(m_entries, getAwsSaveUrl())) {
        qWarning("Failed to save data to AWS.");
    }
}
