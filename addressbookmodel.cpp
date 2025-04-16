#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include "addressbookmodel.h"
#include "util.h"
#include "loadingdialog.h"

AddressBookModel::AddressBookModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    // AWS Lambda 통해 주소록 로드
    m_entries = loadAddressBookFromAWS(getAwsLoadUrl());
}

int AddressBookModel::rowCount(const QModelIndex& /*parent*/) const {
    // 튜플의 개수
    return m_entries.size();
}

int AddressBookModel::columnCount(const QModelIndex& /*parent*/) const {
    // (0) favorite 아이콘 + (1) name, (2) phone, (3) email, (4) company, (5) position, (6) nickname
    return 7;
}

QVariant AddressBookModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_entries.size())
        return QVariant();

    const AddressEntry &entry = m_entries.at(index.row());

    // 첫 번째 열: 즐겨찾기 정보
    if (index.column() == 0) {
        if (role == Qt::CheckStateRole) {
            return entry.favorite() ? Qt::Checked : Qt::Unchecked;
        }
        // DisplayRole에는 빈 문자열 반환 (커스텀 delegate가 그림)
        else if (role == Qt::DisplayRole) {
            return QString("");
        }
        return QVariant();
    }

    // 나머지 열: 텍스트 데이터 반환
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
        case 0: return "★";  // 즐겨찾기 열
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

    // 체크박스(즐겨찾기) 토글
    if (index.column() == 0 && role == Qt::CheckStateRole) {
        entry.setFavorite(value.toInt() == Qt::Checked);
        emit dataChanged(index, index, {Qt::CheckStateRole});
        saveAddressBookToAWS(m_entries, getAwsSaveUrl());
        return true;
    }

    // 나머지 열의 수정 처리
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

    // 데이터 변경 시그널
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    saveAddressBookToAWS(m_entries, getAwsSaveUrl());

    return true;
}

Qt::ItemFlags AddressBookModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = QAbstractTableModel::flags(index);

    // 첫 번째 열(즐겨찾기) 편집 가능
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

    // 통신 중 다이얼로그
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

    // DetailPageWidget에서 관리하는 멤버 변수로 직접 접근
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
    // 이름 기준으로 적절한 삽입 위치 찾기
    int insertPos = 0;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (entry.name().compare(m_entries[i].name(), Qt::CaseInsensitive) < 0) {
            // 알파벳 순으로 현재 항목보다 먼저 오는 경우
            insertPos = i;
            break;
        }
        // 마지막까지 비교했는데 더 큰 경우, 맨 끝에 추가
        if (i == m_entries.size() - 1) {
            insertPos = m_entries.size();
        }
    }

    // 빈 목록이면 첫 번째 위치에 추가
    if (m_entries.isEmpty()) {
        insertPos = 0;
    }

    qDebug() << "Adding entry" << entry.name() << "at position" << insertPos;

    // 해당 위치에 항목 삽입
    beginInsertRows(QModelIndex(), insertPos, insertPos);
    m_entries.insert(insertPos, entry);
    endInsertRows();

    // 통신 중 다이얼로그
    LoadingDialog dialog;
    dialog.show();
    QCoreApplication::processEvents();

    if (!saveAddressBookToAWS(m_entries, getAwsSaveUrl())) {
        qWarning("Failed to save data to AWS.");
    }

    dialog.close();
}
