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

/**
 * AWS Lambda 및 DynamoDB 구조
 * 
 * DynamoDB Table: contacts
 * - Primary Key:
 *   - Partition Key: phoneNumber (String)
 *   - Sort Key: name (String)
 * - Attributes:
 *   - email (String)
 *   - company (String)
 *   - position (String)
 *   - nickname (String)
 *   - favorite (Boolean)
 *   - memo (String)
 *   - image (String) - S3의 처리된 이미지 URL
 *   - original_key (String) - 원본 이미지의 S3 키
 *   - original_image_url (String) - S3의 원본 이미지 전체 URL
 * 
 * AWS Lambda 엔드포인트:
 * - AWS_LAMBDA_LOAD_URL: 주소록 데이터 로드
 * - AWS_LAMBDA_SAVE_URL: 주소록 데이터 저장
 * - AWS_LAMBDA_RESIZE_URL: 이미지 처리 작업
 */


// Returns the Lambda URL for loading address book data
// Uses AWS_LAMBDA_LOAD_URL environment variable
inline QUrl getAwsLoadUrl() {
    return QUrl(QString::fromUtf8(qgetenv("AWS_LAMBDA_LOAD_URL")));
}

// Returns the Lambda URL for saving address book data
// Uses AWS_LAMBDA_SAVE_URL environment variable
inline QUrl getAwsSaveUrl() {
    return QUrl(QString::fromUtf8(qgetenv("AWS_LAMBDA_SAVE_URL")));
}

// Returns the Lambda URL for image processing operations
// Uses AWS_LAMBDA_RESIZE_URL environment variable
inline QUrl getAwsImageResizeUrl() {
    return QUrl(QString::fromUtf8(qgetenv("AWS_LAMBDA_RESIZE_URL")));
}

/**
 * Sends a delete request to AWS Lambda to remove a specific contact
 * 
 * Request format:
 * {
 *   "delete": {
 *     "phoneNumber": "(contact phone)",
 *     "name": "(contact name)"
 *   }
 * }
 * 
 * DynamoDB uses phoneNumber and name as composite key for deletion
 */
inline bool deleteAddressEntryFromAWS(const AddressEntry& entry, const QUrl& url) {
    // Create deletion request object with phone and name
    QJsonObject deleteObj;
    deleteObj["phoneNumber"] = entry.phoneNumber();
    deleteObj["name"] = entry.name();
    
    // Wrap in root object with "delete" key to trigger deletion flow
    QJsonObject root;
    root["delete"] = deleteObj;
    
    // Convert to JSON and prepare request
    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Send synchronous request
    QNetworkReply* reply = manager.post(request, jsonData);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    // Handle response
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Failed to delete entry on AWS:" << reply->errorString();
        reply->deleteLater();
        return false;
    }
    
    reply->deleteLater();
    return true;
}

/**
 * Saves a single address entry to AWS Lambda
 * 
 * Request format:
 * {
 *   "phoneNumber": "...",
 *   "name": "...",
 *   "email": "...",
 *   "company": "...",
 *   ...
 *   "image": "https://bucket-name.s3.amazonaws.com/path/to/image",
 *   "original_key": "path/to/original/image"
 * }
 * 
 * Lambda identifies this as a single item save when it contains phoneNumber 
 * and name but no "results" array
 */
inline bool saveToAWS(const QJsonObject &entryObj, const QUrl &url) {
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Handle original image URL conversion to original_key for Lambda compatibility
    // The Lambda code expects the original_key field to identify the S3 object
    QJsonObject modifiedObj = entryObj;
    if (modifiedObj.contains("original_image_url") && !modifiedObj.contains("original_key")) {
        QString originalUrl = modifiedObj["original_image_url"].toString();
        QStringList parts = originalUrl.split(".s3.amazonaws.com/");
        if (parts.size() > 1) {
            modifiedObj["original_key"] = parts[1];
        }
    }
    
    // Create JSON request
    QJsonDocument doc(modifiedObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    
    qDebug() << "[SAVE] Posting single entry to:" << url.toString();
    qDebug() << "[SAVE] JSON payload:" << jsonData;
    
    // Send synchronous request
    QNetworkReply *reply = manager.post(request, jsonData);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    // Handle response
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[SAVE] Failed to save entry to AWS:" << reply->errorString();
        reply->deleteLater();
        return false;
    }
    
    QByteArray responseData = reply->readAll();
    qDebug() << "[SAVE] Server response:" << responseData;
    reply->deleteLater();
    return true;
}

/**
 * Loads the address book data from AWS Lambda
 * 
 * Response format:
 * {
 *   "results": [
 *     {
 *       "phoneNumber": "...",
 *       "name": "...",
 *       "email": "...",
 *       ...
 *       "image": "https://bucket-name.s3.amazonaws.com/processed/image.jpg",
 *       "original_key": "uploads/original.jpg",
 *       "original_image_url": "https://bucket-name.s3.amazonaws.com/uploads/original.jpg"
 *     },
 *     ...
 *   ]
 * }
 */
inline QList<AddressEntry> loadAddressBookFromAWS(const QUrl &url) {
    QList<AddressEntry> entries;
    
    // Show loading dialog during network operation
    LoadingDialog dialog;
    dialog.show();
    QCoreApplication::processEvents();
    
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    qDebug() << "[LOAD] Requesting GET from:" << url.toString();
    
    // Send synchronous request
    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    dialog.close();
    
    // Handle network errors
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[LOAD] Network error:" << reply->errorString();
        reply->deleteLater();
        return entries;
    }
    
    // Process response data
    QByteArray jsonData = reply->readAll();
    qDebug() << "[LOAD] Raw response:" << jsonData;
    reply->deleteLater();
    
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    QJsonArray jsonArray;
    
    // Handle different JSON response formats:
    // 1. Object with "results" array: { "results": [...] }
    // 2. Direct array: [...]
    if (doc.isObject()) {
        QJsonObject rootObj = doc.object();
        if (rootObj.contains("results") && rootObj["results"].isArray()) {
            jsonArray = rootObj["results"].toArray();
        } else {
            qWarning() << "[LOAD] 'results' key not found or not an array.";
            return entries;
        }
    }
    else if (doc.isArray()) {
        jsonArray = doc.array();
    }
    else {
        qWarning() << "[LOAD] JSON root is neither an object nor an array.";
        return entries;
    }
    
    // Parse each entry from the JSON array
    for (const QJsonValue &value : jsonArray) {
        if (!value.isObject()) continue;
        QJsonObject obj = value.toObject();
        
        // Create and populate address entry
        AddressEntry entry;
        entry.setName(obj["name"].toString());
        entry.setPhoneNumber(obj["phoneNumber"].toString());
        entry.setEmail(obj["email"].toString());
        entry.setCompany(obj["company"].toString());
        entry.setPosition(obj["position"].toString());
        entry.setNickname(obj["nickname"].toString());
        entry.setFavorite(obj["favorite"].toBool());
        entry.setMemo(obj["memo"].toString());
        
        // Handle image URLs
        if (obj.contains("image")) {
            entry.setImageUrl(obj["image"].toString());
        }
        
        // Handle original image URLs with backward compatibility
        if (obj.contains("original_image_url")) {
            entry.setOriginalImageUrl(obj["original_image_url"].toString());
        }
        else if (obj.contains("original_key")) {
            // Construct original URL from S3 bucket and key
            QString bucketUrl = "https://contact-photo-bucket-001.s3.amazonaws.com/";
            QString originalUrl = bucketUrl + obj["original_key"].toString();
            entry.setOriginalImageUrl(originalUrl);
        }
        
        entries.append(entry);
    }
    
    qDebug() << "[LOAD] Parsed entry count:" << entries.size();
    return entries;
}

/**
 * Saves the entire address book to AWS Lambda
 * 
 * Request format:
 * {
 *   "results": [
 *     {
 *       "phoneNumber": "...",
 *       "name": "...",
 *       "email": "...",
 *       ...
 *     },
 *     ...
 *   ]
 * }
 * 
 * Lambda processes this as a batch update for all contacts
 */
inline bool saveAddressBookToAWS(const QList<AddressEntry> &entries, const QUrl &url) {
    // Create JSON array of all entries
    QJsonArray array;
    for (const AddressEntry &entry : entries) {
        QJsonObject obj;
        // Add required fields
        obj["name"] = entry.name();
        obj["phoneNumber"] = entry.phoneNumber();
        obj["email"] = entry.email();
        obj["company"] = entry.company();
        obj["position"] = entry.position();
        obj["nickname"] = entry.nickname();
        obj["favorite"] = entry.favorite();
        obj["memo"] = entry.memo();
        
        // Add image URL if available
        if (!entry.imageUrl().isEmpty()) {
            obj["image"] = entry.imageUrl();
        }
        
        // Add original image URL and extract original_key for Lambda compatibility
        if (!entry.originalImageUrl().isEmpty()) {
            obj["original_image_url"] = entry.originalImageUrl();
            
            // Extract S3 key from the URL for the Lambda
            QStringList parts = entry.originalImageUrl().split(".s3.amazonaws.com/");
            if (parts.size() > 1) {
                obj["original_key"] = parts[1];
            }
        }
        
        array.append(obj);
    }
    
    // Wrap array in "results" object for Lambda format
    QJsonObject rootObj;
    rootObj["results"] = array;
    
    // Convert to JSON and prepare request
    QJsonDocument doc(rootObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);
    
    qDebug() << "[SAVE] Posting to:" << url.toString();
    
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Send synchronous request
    QNetworkReply *reply = manager.post(request, jsonData);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    // Handle response
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
