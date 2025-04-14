#ifndef UTIL_H
#define UTIL_H

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>
#include <QDebug>
#include "AddressEntry.h"

/// JSON 파일에서 주소록 불러오기
inline QVector<AddressEntry> loadAddressBookFromJson(const QString& filePath) {
    QVector<AddressEntry> entries;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("JSON 파일 열기 실패: %s", qUtf8Printable(file.errorString()));
        return entries;
    }

    QByteArray jsonData = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isArray()) {
        qWarning("JSON 루트가 배열이 아님");
        return entries;
    }

    QJsonArray jsonArray = doc.array();
    for (const QJsonValue& value : jsonArray) {
        if (!value.isObject()) continue;

        QJsonObject obj = value.toObject();
        AddressEntry entry;
        entry.setName(obj["name"].toString());
        entry.setPhoneNumber(obj["phoneNumber"].toString());
        entry.setEmail(obj["email"].toString());
        entry.setCompany(obj["company"].toString());
        entry.setPosition(obj["position"].toString());
        entry.setNickname(obj["nickname"].toString());
        entry.setFavorite(obj["favorite"].toBool());
        entries.append(entry);
    }

    return entries;
}

/// 주소록을 JSON 파일로 저장
inline bool saveAddressBookToJson(const QVector<AddressEntry>& entries, const QString& filePath) {
    QJsonArray array;
    for (const AddressEntry& entry : entries) {
        QJsonObject obj;
        obj["name"] = entry.name();
        obj["phoneNumber"] = entry.phoneNumber();
        obj["email"] = entry.email();
        obj["company"] = entry.company();
        obj["position"] = entry.position();
        obj["nickname"] = entry.nickname();
        obj["favorite"] = entry.favorite();
        array.append(obj);
    }

    QJsonDocument doc(array);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning("JSON 파일 쓰기 실패: %s", qUtf8Printable(file.errorString()));
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

#endif // UTIL_H
