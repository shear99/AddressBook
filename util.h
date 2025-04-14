#ifndef UTIL_H
#define UTIL_H

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>
#include "AddressEntry.h"

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


#endif // UTIL_H
