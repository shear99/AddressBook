#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include "addressbookmodel.h"
#include "util.h"
#include "loadingdialog.h"

AddressBookModel::AddressBookModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    // Load address book from AWS Lambda
    m_entries = loadAddressBookFromAWS(getAwsLoadUrl());
}

int AddressBookModel::rowCount(const QModelIndex& /*parent*/) const {
    // Number of entries
    return m_entries.size();
}

int AddressBookModel::columnCount(const QModelIndex& /*parent*/) const {
    // Columns: (0) favorite icon + (1) name, (2) phone, (3) email, (4) company, (5) position, (6) nickname
    return 7;
}

QVariant AddressBookModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_entries.size())
        return QVariant();

    const AddressEntry &entry = m_entries.at(index.row());

    // First column: favorite status
    if (index.column() == 0) {
        if (role == Qt::CheckStateRole) {
            return entry.favorite() ? Qt::Checked : Qt::Unchecked;
        }
        // Return empty string for DisplayRole (custom delegate handles display)
        else if (role == Qt::DisplayRole) {
            return QString("");
        }
        return QVariant();
    }

    // Other columns: return text data
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 1: return entry.name();
        case 2: return entry.phoneNumber();
        case 3: return entry.email();
        case 4: return entry.company();
        case 5: return entry.position();
        case 6: return entry.nickname();
        default: return QVariant();
        }
    }
    return QVariant();
}

QVariant AddressBookModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "❤";
        case 1: return "Name";
        case 2: return "Phone";
        case 3: return "Email";
        case 4: return "Company";
        case 5: return "Position";
        case 6: return "Nickname";
        default: return QVariant();
        }
    } else {
        return section + 1;
    }
    return QVariant();
}

bool AddressBookModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid())
        return false;

    AddressEntry &entry = m_entries[index.row()];

    // Toggle checkbox (favorite status)
    if (index.column() == 0 && role == Qt::CheckStateRole) {
        entry.setFavorite(value.toInt() == Qt::Checked);
        emit dataChanged(index, index, {Qt::CheckStateRole});
        saveAddressBookToAWS(m_entries, getAwsSaveUrl());
        return true;
    }

    // Handle edits for other columns
    if (role != Qt::EditRole)
        return false;
    switch (index.column()) {
    case 1: entry.setName(value.toString()); break;
    case 2: entry.setPhoneNumber(value.toString()); break;
    case 3: entry.setEmail(value.toString()); break;
    case 4: entry.setCompany(value.toString()); break;
    case 5: entry.setPosition(value.toString()); break;
    case 6: entry.setNickname(value.toString()); break;
    default: return false;
    }

    // Emit data changed signal
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    saveAddressBookToAWS(m_entries, getAwsSaveUrl());

    return true;
}

Qt::ItemFlags AddressBookModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = QAbstractTableModel::flags(index);

    // First column (favorite) is editable
    if (index.column() == 0)
        flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;

    return flags;
}

void AddressBookModel::removeEntry(int row) {
    if (row < 0 || row >= m_entries.size())
        return;

    AddressEntry removedEntry = m_entries.at(row);

    beginRemoveRows(QModelIndex(), row, row);
    m_entries.removeAt(row);
    endRemoveRows();

    // Show loading dialog during network operation
    LoadingDialog dialog;
    dialog.show();
    QCoreApplication::processEvents();

    if (!deleteAddressEntryFromAWS(removedEntry, getAwsSaveUrl())) {
        qWarning("Failed to delete entry from AWS.");
    }

    dialog.close();
}

AddressEntry AddressBookModel::getEntry(int row) const {
    if (row < 0 || row >= m_entries.size())
        return AddressEntry();
    return m_entries.at(row);
}

void AddressBookModel::updateEntry(int row, const AddressEntry& entry) {
    if (row < 0 || row >= m_entries.size())
        return;

    // Direct access to member variables managed by DetailPageWidget
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

void AddressBookModel::addEntry(const AddressEntry& entry) {
    // Find appropriate insertion position based on name
    int insertPos = 0;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (entry.name().compare(m_entries[i].name(), Qt::CaseInsensitive) < 0) {
            // Insert before current entry if alphabetically earlier
            insertPos = i;
            break;
        }
        // If reached end, append to the end
        if (i == m_entries.size() - 1) {
            insertPos = m_entries.size();
        }
    }

    // If list is empty, add at first position
    if (m_entries.isEmpty()) {
        insertPos = 0;
    }

    qDebug() << "Adding entry" << entry.name() << "at position" << insertPos;

    // Insert entry at calculated position
    beginInsertRows(QModelIndex(), insertPos, insertPos);
    m_entries.insert(insertPos, entry);
    endInsertRows();

    // Show loading dialog during network operation
    LoadingDialog dialog;
    dialog.show();
    QCoreApplication::processEvents();

    if (!saveAddressBookToAWS(m_entries, getAwsSaveUrl())) {
        qWarning("Failed to save data to AWS.");
    }

    dialog.close();
}
