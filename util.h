#ifndef UTIL_H
#define UTIL_H
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <QProcessEnvironment>
#include <QCoreApplication>
#include "addressentry.h"
#include "loadingdialog.h"
/// Lambda URL을 환경 변수에서 가져오기
inline QUrl getAwsLoadUrl() {
    return QUrl(QString::fromUtf8(qgetenv("AWS_LAMBDA_LOAD_URL")));
}
inline QUrl getAwsSaveUrl() {
    return QUrl(QString::fromUtf8(qgetenv("AWS_LAMBDA_SAVE_URL")));
}
inline QUrl getAwsImageResizeUrl() {
    return QUrl(QString::fromUtf8(qgetenv("AWS_LAMBDA_RESIZE_URL")));
}
// lamdba에 delete키를 추가하여 DB에 삭제 요청
inline bool deleteAddressEntryFromAWS(const AddressEntry& entry, const QUrl& url) {
    QJsonObject deleteObj;
    deleteObj["phoneNumber"] = entry.phoneNumber();
    deleteObj["name"] = entry.name();
    QJsonObject root;
    // "delete" 키를 통해 삭제 요청임을 표시합니다.
    root["delete"] = deleteObj;
    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = manager.post(request, jsonData);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to delete entry on AWS:" << reply->errorString();
        reply->deleteLater();
        return false;
    }
    reply->deleteLater();
    return true;
}
/// AWS Lambda를 통해 주소록 데이터를 불러오기
inline QList<AddressEntry> loadAddressBookFromAWS(const QUrl &url) {
    QList<AddressEntry> entries;
    // 통신 중 다이얼로그 띄우기
    LoadingDialog dialog;
    dialog.show();
    QCoreApplication::processEvents();
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    qDebug() << "[LOAD] Requesting GET from:" << url.toString();
    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    dialog.close();  // 통신 끝나면 다이얼로그 닫기
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[LOAD] Network error:" << reply->errorString();
        reply->deleteLater();
        return entries;
    }
    QByteArray jsonData = reply->readAll();
    qDebug() << "[LOAD] Raw response:" << jsonData;
    reply->deleteLater();
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    QJsonArray jsonArray;
    // JSON 구조가 { "results": [ ... ] } 인 경우 처리
    if (doc.isObject()) {
        QJsonObject rootObj = doc.object();
        if (rootObj.contains("results") && rootObj["results"].isArray()) {
            jsonArray = rootObj["results"].toArray();
        } else {
            qWarning() << "[LOAD] 'results' key not found or not an array.";
            return entries;
        }
    }
    // 최상위가 배열인 경우 지원
    else if (doc.isArray()) {
        jsonArray = doc.array();
    }
    else {
        qWarning() << "[LOAD] JSON root is neither an object nor an array.";
        return entries;
    }
    for (const QJsonValue &value : jsonArray) {
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
        entry.setMemo(obj["memo"].toString());
        entries.append(entry);
    }
    qDebug() << "[LOAD] Parsed entry count:" << entries.size();
    return entries;
}
/// AWS Lambda를 통해 주소록 데이터를 저장하기
inline bool saveAddressBookToAWS(const QList<AddressEntry> &entries, const QUrl &url) {
    QJsonArray array;
    for (const AddressEntry &entry : entries) {
        QJsonObject obj;
        obj["name"] = entry.name();
        obj["phoneNumber"] = entry.phoneNumber();
        obj["email"] = entry.email();
        obj["company"] = entry.company();
        obj["position"] = entry.position();
        obj["nickname"] = entry.nickname();
        obj["favorite"] = entry.favorite();
        obj["memo"] = entry.memo();
        array.append(obj);
    }
    // 배열을 "results"라는 키로 감싸기
    QJsonObject rootObj;
    rootObj["results"] = array;
    QJsonDocument doc(rootObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    qDebug() << "[SAVE] Posting to:" << url.toString();
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager.post(request, jsonData);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[SAVE] Failed to save data to AWS:" << reply->errorString();
        reply->deleteLater();
        return false;
    }
    QByteArray responseData = reply->readAll();
    qDebug() << "[SAVE] Server response:" << responseData;
    reply->deleteLater();
    return true;
}
#endif // UTIL_H
