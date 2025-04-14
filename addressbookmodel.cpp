#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include "AddressBookModel.h"
#include "util.h"

AddressBookModel::AddressBookModel(QObject* parent)
    : QAbstractTableModel(parent),
    // 사용자가 말한 경로 그대로
    m_favTrue(":/asset/asset/yes.png"),
    m_favFalse(":/asset/asset/no.png")
{
    // AWS Lambda 통해 주소록 로드
    m_entries = loadAddressBookFromAWS(getAwsLoadUrl());
}

int AddressBookModel::rowCount(const QModelIndex& /*parent*/) const {
    return m_entries.size();
}

int AddressBookModel::columnCount(const QModelIndex& /*parent*/) const {
    // (0)favorite 아이콘 + (1)name, (2)phone, (3)email, (4)company, (5)position, (6)nickname
    return 7;
}

QVariant AddressBookModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_entries.size())
        return QVariant();

    const AddressEntry& entry = m_entries.at(index.row());

    // ---------- 아이콘 표시 (DecorationRole) ----------
    if (role == Qt::DecorationRole && index.column() == 0) {
        // favorite 값에 따라 다른 아이콘 리턴
        return QIcon(entry.favorite() ? m_favTrue : m_favFalse);
    }

    // ---------- 텍스트 표시 (DisplayRole) ----------
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:
            // 아이콘 열이지만, DisplayRole일 때는 굳이 텍스트를 보여줄 필요가 없으므로 빈 문자열
            return QString("");
        case 1: return entry.name();
        case 2: return entry.phoneNumber();
        case 3: return entry.email();
        case 4: return entry.company();
        case 5: return entry.position();
        case 6: return entry.nickname();
        }
    }

    return QVariant();
}

QVariant AddressBookModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "★";  // 즐겨찾기 열
        case 1: return "Name";
        case 2: return "Phone";
        case 3: return "Email";
        case 4: return "Company";
        case 5: return "Position";
        case 6: return "Nickname";
        }
    } else {
        return section + 1;
    }
    return QVariant();
}

// ---------- 여기서 favorite 토글 ----------
bool AddressBookModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    AddressEntry& entry = m_entries[index.row()];

    // 0번 열이라면 => favorite 토글
    if (index.column() == 0) {
        entry.setFavorite(!entry.favorite());
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});

        // AWS 저장
        if (!saveAddressBookToAWS(m_entries, getAwsSaveUrl())) {
            qWarning("Failed to save data to AWS.");
        }
        return true;
    }

    // 그 외 열(1~6)은 문자열 편집
    switch (index.column()) {
    case 1: entry.setName(value.toString()); break;
    case 2: entry.setPhoneNumber(value.toString()); break;
    case 3: entry.setEmail(value.toString()); break;
    case 4: entry.setCompany(value.toString()); break;
    case 5: entry.setPosition(value.toString()); break;
    case 6: entry.setNickname(value.toString()); break;
    default: return false;
    }

    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});

    // AWS 저장
    if (!saveAddressBookToAWS(m_entries, getAwsSaveUrl())) {
        qWarning("Failed to save data to AWS.");
    }

    return true;
}

// ---------- 열 0도 편집 가능하도록 flags 수정 ----------
Qt::ItemFlags AddressBookModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return Qt::NoItemFlags;

    // 기본 플래그
    Qt::ItemFlags f = QAbstractTableModel::flags(index);

    // 모든 열을 편집 가능하게 해도 되고, column 0만 특별히 처리해도 됨
    // 여기서는 전부 편집 가능하게:
    f |= Qt::ItemIsEditable;

    return f;
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
