#include <QMessageBox>
#include <QCloseEvent>
#include "util.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QMessageBox>
#include <QBuffer>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include "detailpagewidget.h"

DetailPageWidget::DetailPageWidget(const AddressEntry& entry, QWidget* parent, bool isAddMode)
    : QWidget(parent), m_entry(entry),ui(new Ui::DetailPageWidget),m_isAddMode(isAddMode)
{

    // Initialize network manager for AWS communication
    m_networkManager = new QNetworkAccessManager(this);
    
    // Setup UI and event filters
    ui->setupUi(this);
    ui->detailpageImage->installEventFilter(this);
    ui->detailpageImage->setCursor(Qt::PointingHandCursor);


    //Font change
    FontUpdate::applyFontToAllChildren(this, ":/fonts/fonts/GmarketSansTTFMedium.ttf");

    // Initialize fields and load data
    populateFields();
    
    // Load image from AWS if not in add mode
    if (m_isAddMode == false) {
        fetchImageUrlFromDB();
    }
    else if (!m_entry.imageUrl().isEmpty()) {
        downloadImageFromS3(m_entry.imageUrl());
    }

    // Connect signals for data synchronization
    connect(ui->detailpagesaveButton, &QPushButton::clicked, this, &DetailPageWidget::onSaveClicked);
    connect(ui->detailpageexitButton, &QPushButton::clicked, this, &DetailPageWidget::closeWindow);
    connect(ui->detailpageeditButton, &QPushButton::clicked, this, &DetailPageWidget::onEditButtonClicked);

    // Store original values for change tracking
    setOriginalName(m_entry.name());
    setOriginalPhoneNumber(m_entry.phoneNumber());

    // Store original styles for UI state management
    origNameStyle = ui->detailpageName->styleSheet();
    origPhoneStyle = ui->detailpagePhone->styleSheet();
    origMailStyle = ui->detailpageMail->styleSheet();
    origCompanyStyle = ui->detailpageCompany->styleSheet();
    origPositionStyle = ui->detailpagePosition->styleSheet();
    origNicknameStyle = ui->detailpageNickname->styleSheet();
    origMemoStyle = ui->detailpageNotice->styleSheet();

    modeSetting(m_isAddMode);
}

// 모델을 받는 생성자 구현
DetailPageWidget::DetailPageWidget(const AddressEntry& entry, QWidget* parent, bool isAddMode, AddressBookModel* model)
    : QWidget(parent), m_entry(entry), ui(new Ui::DetailPageWidget), m_isAddMode(isAddMode)
{
    // Setup duplicate check function if model is provided
    if (model) {
        m_duplicateCheckFunc = [model](const QString& name, const QString& phone) -> bool {
            for (int i = 0; i < model->rowCount(); i++) {
                AddressEntry entry = model->getEntry(i);
                if (entry.name() == name && entry.phoneNumber() == phone) {
                    return true;
                }
            }
            return false;
        };
    }

    // Initialize network manager for AWS communication
    m_networkManager = new QNetworkAccessManager(this);
    
    // Setup UI and event filters
    ui->setupUi(this);
    ui->detailpageImage->installEventFilter(this);
    ui->detailpageImage->setCursor(Qt::PointingHandCursor);

    //Font change
    FontUpdate::applyFontToAllChildren(this, ":/fonts/fonts/GmarketSansTTFMedium.ttf");

    // Initialize fields and load data
    populateFields();
    
    // Load image from AWS if not in add mode
    if (m_isAddMode == false) {
        fetchImageUrlFromDB();
    }
    else if (!m_entry.imageUrl().isEmpty()) {
        downloadImageFromS3(m_entry.imageUrl());
    }

    // Connect signals for data synchronization
    connect(ui->detailpagesaveButton, &QPushButton::clicked, this, &DetailPageWidget::onSaveClicked);
    connect(ui->detailpageexitButton, &QPushButton::clicked, this, &DetailPageWidget::closeWindow);
    connect(ui->detailpageeditButton, &QPushButton::clicked, this, &DetailPageWidget::onEditButtonClicked);

    // Store original values for change tracking
    setOriginalName(m_entry.name());
    setOriginalPhoneNumber(m_entry.phoneNumber());

    // Store original styles for UI state management
    origNameStyle = ui->detailpageName->styleSheet();
    origPhoneStyle = ui->detailpagePhone->styleSheet();
    origMailStyle = ui->detailpageMail->styleSheet();
    origCompanyStyle = ui->detailpageCompany->styleSheet();
    origPositionStyle = ui->detailpagePosition->styleSheet();
    origNicknameStyle = ui->detailpageNickname->styleSheet();
    origMemoStyle = ui->detailpageNotice->styleSheet();

    modeSetting(m_isAddMode);
}

// DB에서 이미지 URL 조회
void DetailPageWidget::fetchImageUrlFromDB()
{
    qDebug() << "[DB Image] Fetching image URL for name:" << m_entry.name() 
             << "phone:" << m_entry.phoneNumber();
    
    // Prepare AWS Lambda request
    QUrl loadUrl = getAwsLoadUrl();
    QNetworkRequest request(loadUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject queryObj;
    queryObj["phoneNumber"] = m_entry.phoneNumber();
    queryObj["name"] = m_entry.name();
    
    QJsonObject root;
    root["query"] = queryObj;
    
    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson();
    
    // Send request to AWS
    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    
    // Handle AWS response
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        
        // Check for network errors
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[DB Image] Error fetching image URL:" << reply->errorString();
            return;
        }
        
        QByteArray responseData = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        QJsonObject responseObj = responseDoc.object();
        
        // Check if response contains results array
        if (responseObj.contains("results") && responseObj["results"].isArray()) {
            QJsonArray results = responseObj["results"].toArray();
            
            // Process results if any exist
            if (results.size() > 0) {
                for (int i = 0; i < results.size(); i++) {
                    QJsonObject entryObj = results[i].toObject();
                    
                    // Find matching entry by phone number and name
                    if (entryObj["phoneNumber"].toString() == m_entry.phoneNumber() && 
                        entryObj["name"].toString() == m_entry.name()) {
                        
                        // Process image URL from AWS response
                        if (entryObj.contains("image")) {
                            QString imageUrl = entryObj["image"].toString();
                            if (!imageUrl.isEmpty()) {
                                m_currentImageUrl = imageUrl;
                                m_entry.setImageUrl(imageUrl);
                            }
                        }
                        
                        // Process original image URL from AWS response
                        if (entryObj.contains("original_image_url")) {
                            QString originalUrl = entryObj["original_image_url"].toString();
                            if (!originalUrl.isEmpty()) {
                                m_entry.setOriginalImageUrl(originalUrl);
                            }
                        }
                        
                        // Process original S3 key from AWS response
                        if (entryObj.contains("original_key")) {
                            m_originalS3Key = entryObj["original_key"].toString();
                            if (m_entry.originalImageUrl().isEmpty()) {
                                QString bucketUrl = "https://contact-photo-bucket-001.s3.amazonaws.com/";
                                QString originalUrl = bucketUrl + m_originalS3Key;
                                m_entry.setOriginalImageUrl(originalUrl);
                            }
                        }
                        
                        // Download and display image
                        if (!m_currentImageUrl.isEmpty()) {
                            downloadImageFromS3(m_currentImageUrl);
                        }
                        
                        break;
                    }
                }
            }
        }
    });
}

// 새로운 함수 추가
void DetailPageWidget::updateImageUI(const QString& imageUrl)
{
    qDebug() << "[Image] Updating UI with image URL:" << imageUrl;
    m_currentImageUrl = imageUrl;
    ui->detailpageImage->setScaledContents(true);
}

DetailPageWidget::~DetailPageWidget()
{
    delete ui;
}

bool DetailPageWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->detailpageImage && event->type() == QEvent::MouseButtonRelease) {
        onProfileImageClicked();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void DetailPageWidget::onProfileImageClicked()
{
    // Handle image click in add mode
    if (m_isAddMode) {
        QMessageBox::information(this, tr("알림"), 
            tr("지금은 이미지를 업로드할 수 없습니다. 주소록에 추가한 후 사진을 업로드해주세요."));
        return;
    }

    // Handle image click in read-only mode
    if (ui->detailpageName->isReadOnly() && !m_isAddMode) {
        if (!m_entry.originalImageUrl().isEmpty()) {
            showImageDialog(m_entry.originalImageUrl());
        }
        else if (!m_currentImageUrl.isEmpty()) {
            showImageDialog(m_currentImageUrl);
        }
        else {
            QMessageBox::information(this, tr("이미지 없음"), 
                tr("표시할 이미지가 없습니다. 편집 모드에서 이미지를 업로드해주세요."));
        }
    } 
    // Handle image selection in edit mode
    else {
        QString filePath = QFileDialog::getOpenFileName(this, 
            tr("프로필 이미지 선택"), 
            QString(), 
            tr("이미지 파일 (*.jpg *.jpeg *.png)"));
            
        if (!filePath.isEmpty()) {
            QFileInfo fileInfo(filePath);
            QString extension = fileInfo.suffix().toLower();
            // Validate file extension
            if (extension != "jpg" && extension != "jpeg" && extension != "png") {
                QMessageBox::warning(this, tr("파일 형식 오류"), 
                    tr("지원하지 않는 파일 형식입니다. JPG, JPEG, PNG 파일만 선택 가능합니다."));
                return;
            }
            uploadImageToLambda(filePath);
        }
    }
}

void DetailPageWidget::uploadImageToLambda(const QString& imagePath)
{
    qDebug() << "[Image] Starting image upload process for:" << imagePath;
    
    // Show loading dialog during upload
    LoadingDialog* loadingDialog = new LoadingDialog(this);
    loadingDialog->show();
    QCoreApplication::processEvents();
    
    // Load and process image
    QImage originalImage(imagePath);
    if (originalImage.isNull()) {
        qDebug() << "[Image] Failed to load image:" << imagePath;
        QMessageBox::warning(this, tr("파일 오류"), tr("이미지 파일을 열 수 없습니다."));
        loadingDialog->close();
        loadingDialog->deleteLater();
        return;
    }
    
    // Resize image if needed
    int maxWidth = 1024;
    int maxHeight = 1024;
    QImage resizedImage = originalImage;
    
    if (originalImage.width() > maxWidth || originalImage.height() > maxHeight) {
        resizedImage = originalImage.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    // Prepare image data for upload
    QByteArray fileData;
    QBuffer buffer(&fileData);
    buffer.open(QIODevice::WriteOnly);
    resizedImage.save(&buffer, "JPEG", 80);
    buffer.close();
    
    // Generate S3 key for upload
    QString fileName = QString("uploads/%1_%2.jpg")
                      .arg(m_entry.phoneNumber().replace("-", ""))
                      .arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
    
    // Prepare AWS Lambda request
    QUrl lambdaUrl = getAwsImageResizeUrl();
    QNetworkRequest request(lambdaUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QByteArray base64Data = fileData.toBase64();
    
    QJsonObject jsonObj;
    jsonObj["key"] = fileName;
    jsonObj["phoneNumber"] = m_entry.phoneNumber();
    jsonObj["name"] = m_entry.name();
    jsonObj["image_data"] = QString(base64Data);
    
    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson();
    
    // Send request to AWS
    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    
    // Handle AWS response
    connect(reply, &QNetworkReply::finished, this, [this, reply, loadingDialog]() {
        loadingDialog->close();
        loadingDialog->deleteLater();
        onImageUploadFinished(reply);
    });
}

void DetailPageWidget::onImageUploadFinished(QNetworkReply* reply)
{
    qDebug() << "[Image] Received response from Lambda";
    
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "[Image] Network error:" << reply->errorString();
        QMessageBox::warning(this, tr("업로드 오류"), 
            tr("이미지 업로드 중 오류가 발생했습니다: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "[Image] JSON parse error:" << parseError.errorString();
        QMessageBox::warning(this, tr("응답 오류"), tr("서버 응답을 파싱할 수 없습니다."));
        return;
    }
    
    QJsonObject responseObj = doc.object();
    
    if (responseObj.contains("s3_url")) {
        QString imageUrl = responseObj["s3_url"].toString();
        m_currentImageUrl = imageUrl;
        
        // Process original image key from AWS response
        if (responseObj.contains("original_key")) {
            m_originalS3Key = responseObj["original_key"].toString();
            QString originalUrl = "https://contact-photo-bucket-001.s3.amazonaws.com/" + m_originalS3Key;
            m_entry.setOriginalImageUrl(originalUrl);
        }
        
        try {
            // Download and display image
            downloadImageFromS3(imageUrl);
            
            // Update AddressEntry
            m_entry.setImageUrl(imageUrl);
            
            // Update database
            LoadingDialog dialog(this);
            dialog.show();
            QCoreApplication::processEvents();
            
            QJsonObject entryObj;
            entryObj["name"] = m_entry.name();
            entryObj["phoneNumber"] = m_entry.phoneNumber();
            entryObj["email"] = m_entry.email();
            entryObj["company"] = m_entry.company();
            entryObj["position"] = m_entry.position();
            entryObj["nickname"] = m_entry.nickname();
            entryObj["favorite"] = m_entry.favorite();
            entryObj["memo"] = m_entry.memo();
            entryObj["image"] = imageUrl;
            entryObj["original_key"] = m_originalS3Key;
            
            if (!m_entry.originalImageUrl().isEmpty()) {
                entryObj["original_image_url"] = m_entry.originalImageUrl();
            }
            
            if (!saveToAWS(entryObj, getAwsSaveUrl())) {
                dialog.close();
                QMessageBox::warning(this, tr("DB 업데이트 오류"), tr("이미지 URL을 DB에 저장하는데 실패했습니다."));
            }
            
            dialog.close();
        } catch (const std::exception& e) {
            qDebug() << "[Image] Exception during image processing:" << e.what();
            QMessageBox::warning(this, tr("이미지 처리 오류"), tr("이미지 처리 중 오류가 발생했습니다."));
        }
    } else {
        qDebug() << "[Image] Response missing s3_url";
        QMessageBox::warning(this, tr("응답 오류"), tr("서버 응답에 이미지 URL이 없습니다."));
    }
}

void DetailPageWidget::downloadImageFromS3(const QString& imageUrl)
{
    qDebug() << "[Image] Starting image download from:" << imageUrl;
    
    if (imageUrl.isEmpty()) {
        qDebug() << "[Image] Empty URL provided";
        return;
    }
    
    // Show loading dialog during download
    LoadingDialog* loadingDialog = new LoadingDialog(this);
    loadingDialog->show();
    QCoreApplication::processEvents();
    
    // Extract S3 key from URL
    QString key = imageUrl.split(".s3.amazonaws.com/")[1];
    
    // Prepare AWS Lambda request
    QUrl lambdaUrl = getAwsImageResizeUrl();
    QNetworkRequest request(lambdaUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject jsonObj;
    jsonObj["action"] = "get_image";
    jsonObj["key"] = key;
    
    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson();
    
    // Send request to AWS
    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    
    // Set timeout for request
    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [reply]() {
        if (!reply->isFinished()) {
            reply->abort();
        }
    });
    timer->start(10000);
    
    // Handle AWS response
    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, loadingDialog]() {
        timer->stop();
        timer->deleteLater();
        loadingDialog->close();
        loadingDialog->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[Image] Download error:" << reply->errorString();
            reply->deleteLater();
            return;
        }
        
        QByteArray responseData = reply->readAll();
        reply->deleteLater();
        
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject responseObj = doc.object();
        
        if (responseObj.contains("image_data")) {
            QString base64Data = responseObj["image_data"].toString();
            QByteArray imageData = QByteArray::fromBase64(base64Data.toUtf8());
            
            QPixmap pixmap;
            if (!pixmap.loadFromData(imageData)) {
                qDebug() << "[Image] Failed to load image data into QPixmap";
                return;
            }
            
            // Display image
            ui->detailpageImage->setStyleSheet("");
            ui->detailpageImage->setPixmap(pixmap);
            ui->detailpageImage->setScaledContents(true);
        } else {
            qDebug() << "[Image] Response missing image_data";
        }
    });
}

void DetailPageWidget::modeSetting(bool _AddMode)
{
    // Add mode configuration
    if (_AddMode) {
        ui->detailpageTitle->setText("연락처 추가");
        ui->detailpageCall->hide();
        ui->detailpagemailImage->hide();
        ui->detailpageMessage->hide();
        ui->detailpageInvite->hide();
        ui->detailpageLabel->hide();
        ui->detailpageeditButton->hide();
        ui->detailpagesaveButton->hide();
        ui->detailpageiconOutline->hide();

        QString editStyle = "QLineEdit { background: white; }";
        ui->detailpageName->setStyleSheet(editStyle);
        ui->detailpagePhone->setStyleSheet(editStyle);
        ui->detailpageMail->setStyleSheet(editStyle);
        ui->detailpageNickname->setStyleSheet(editStyle);
        ui->detailpageNotice->setStyleSheet(editStyle);

        ui->infoVLayout->setContentsMargins(5, 0, 0, 0);
        
        QLabel* nameLabel = new QLabel("Name:", this);
        QLabel* phoneLabel = new QLabel("Phone:", this);
        QLabel* positionLabel = new QLabel("Position:", this);
        QLabel* nicknameLabel = new QLabel("Nickname:", this);

        ui->infoFormLayout->setLabelAlignment(Qt::AlignRight);

        ui->infoFormLayout->addRow(nameLabel, ui->detailpageName);
        ui->infoFormLayout->addRow(phoneLabel, ui->detailpagePhone);
        ui->infoFormLayout->addRow(positionLabel, ui->detailpagePosition);
        ui->infoFormLayout->addRow(nicknameLabel, ui->detailpageNickname);

        ui->detailpageCompany->setStyleSheet(editStyle);
        ui->detailpagePosition->setStyleSheet(editStyle);
        connect(ui->detailpageaddnewButton, &QPushButton::clicked, this, &DetailPageWidget::onSaveClicked);
    }
    // Normal mode configuration
    else
    {
        editInitialSettings();
        ui->detailpageaddnewButton->hide();
        ui->detailpageaddnewLabel->hide();
    }
}

void DetailPageWidget::populateFields() {
    ui->detailpageName->setText(m_entry.name());
    ui->detailpagePhone->setText(m_entry.phoneNumber());
    ui->detailpageMail->setText(m_entry.email());
    ui->detailpageCompany->setText(m_entry.company());
    ui->detailpagePosition->setText(m_entry.position());
    ui->detailpageNickname->setText(m_entry.nickname());
    ui->detailpageNotice->setText(m_entry.memo());
    
    if (!m_entry.imageUrl().isEmpty()) {
        m_currentImageUrl = m_entry.imageUrl();
        downloadImageFromS3(m_currentImageUrl);
    }
}

void DetailPageWidget::onSaveClicked() {
    // Validate required fields
    if (ui->detailpageName->text().trimmed().isEmpty() || ui->detailpagePhone->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("입력 오류"), tr("이름과 전화번호는 반드시 입력되어야 합니다."));
        return;
    }

    // Validate phone number format
    QString phone = ui->detailpagePhone->text().trimmed();
    QRegularExpression regex("^\\d{3}-\\d{4}-\\d{4}$");
    QRegularExpressionMatch match = regex.match(phone);
    if (!match.hasMatch()) {
        QMessageBox::warning(this, tr("입력 오류"), tr("전화번호는 000-0000-0000 형식이어야 합니다."));
        return;
    }

    // Store new values
    QString newName = ui->detailpageName->text();
    QString newPhone = phone;
    
    // Check for duplicates in add mode
    if (m_isAddMode && m_duplicateCheckFunc) {
        bool isDuplicate = m_duplicateCheckFunc(newName, newPhone);
        
        if (isDuplicate) {
            QMessageBox::warning(this, tr("중복 오류"), 
                tr("동일한 이름과 전화번호를 가진 연락처가 이미 존재합니다."));
            return;
        }
    }
    
    // Check if name or phone number was modified
    bool isModified = (m_originalName != newName) || (m_originalPhoneNumber != newPhone);

    // Show loading dialog during save
    LoadingDialog dialog(this);
    dialog.show();
    QCoreApplication::processEvents();

    // Delete existing entry if modified (not in add mode)
    if (isModified && !m_isAddMode) {
        if (!deleteAddressEntryFromAWS(m_entry, getAwsSaveUrl())) {
            dialog.close();
            QMessageBox::warning(this, tr("삭제 오류"), tr("이전 튜플 삭제에 실패했습니다."));
            return;
        }
    }

    // Update entry data
    m_entry.setName(newName);
    m_entry.setPhoneNumber(newPhone);
    m_entry.setEmail(ui->detailpageMail->text());
    m_entry.setCompany(ui->detailpageCompany->text());
    m_entry.setPosition(ui->detailpagePosition->text());
    m_entry.setNickname(ui->detailpageNickname->text());
    m_entry.setMemo(ui->detailpageNotice->text());

    // Update image URL if changed
    if (!m_currentImageUrl.isEmpty()) {
        m_entry.setImageUrl(m_currentImageUrl);
    }

    // Update original values if modified
    if (isModified) {
        setOriginalName(newName);
        setOriginalPhoneNumber(newPhone);
    }

    dialog.close();

    m_saved = true;
    emit entryUpdated(m_entry);
    emit detailPageClosed();
    this->close();
}

void DetailPageWidget::editInitialSettings()
{
        ui->detailpageName->setReadOnly(true);
        ui->detailpagePhone->setReadOnly(true);
        ui->detailpageMail->setReadOnly(true);
        ui->detailpageCompany->setReadOnly(true);
        ui->detailpagePosition->setReadOnly(true);
        ui->detailpageNickname->setReadOnly(true);
        ui->detailpageNotice->setReadOnly(true);
}

void DetailPageWidget::onEditButtonClicked()
{
    bool isReadOnly = ui->detailpageName->isReadOnly();
    QString editStyle = "QLineEdit { background: white; }";

    // Toggle edit mode for all fields
    ui->detailpageName->setReadOnly(!isReadOnly);
    ui->detailpageName->setStyleSheet(isReadOnly ? editStyle : origNameStyle);

    ui->detailpagePhone->setReadOnly(!isReadOnly);
    ui->detailpagePhone->setStyleSheet(isReadOnly ? editStyle : origPhoneStyle);

    ui->detailpageMail->setReadOnly(!isReadOnly);
    ui->detailpageMail->setStyleSheet(isReadOnly ? editStyle : origMailStyle);

    ui->detailpageCompany->setReadOnly(!isReadOnly);
    ui->detailpageCompany->setStyleSheet(isReadOnly ? editStyle : origCompanyStyle);

    ui->detailpagePosition->setReadOnly(!isReadOnly);
    ui->detailpagePosition->setStyleSheet(isReadOnly ? editStyle : origPositionStyle);

    ui->detailpageNickname->setReadOnly(!isReadOnly);
    ui->detailpageNickname->setStyleSheet(isReadOnly ? editStyle : origNicknameStyle);

    ui->detailpageNotice->setReadOnly(!isReadOnly);
    ui->detailpageNotice->setStyleSheet(isReadOnly ? editStyle : origMemoStyle);
}

void DetailPageWidget::showImageDialog(const QString& imageUrl)
{
    qDebug() << "[Image] Showing image dialog for URL:" << imageUrl;
    
    // Extract S3 key from URL
    QString s3Key;
    if (imageUrl.contains(".s3.amazonaws.com/")) {
        s3Key = imageUrl.split(".s3.amazonaws.com/")[1];
    } else {
        qDebug() << "[Image] Invalid image URL format";
        QMessageBox::warning(this, tr("이미지 오류"), tr("잘못된 이미지 URL 형식입니다."));
        return;
    }
    
    // Check if image is original
    bool isOriginalImage = false;
    if (!m_entry.originalImageUrl().isEmpty() && imageUrl == m_entry.originalImageUrl()) {
        isOriginalImage = true;
    }
    else if (!m_originalS3Key.isEmpty() && imageUrl.endsWith(m_originalS3Key)) {
        isOriginalImage = true;
    }
    
    // Prepare AWS Lambda request
    QUrl lambdaUrl = getAwsImageResizeUrl();
    QNetworkRequest request(lambdaUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject jsonObj;
    jsonObj["action"] = "get_image";
    jsonObj["key"] = s3Key;
    
    if (isOriginalImage) {
        jsonObj["original"] = true;
    }
    
    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson();
    
    // Show loading dialog during download
    LoadingDialog* loadingDialog = new LoadingDialog(this);
    loadingDialog->show();
    QCoreApplication::processEvents();
    
    // Send request to AWS
    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    
    // Handle AWS response
    connect(reply, &QNetworkReply::finished, this, [this, reply, loadingDialog, isOriginalImage]() {
        loadingDialog->close();
        loadingDialog->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[Image] Error downloading image:" << reply->errorString();
            QMessageBox::warning(this, tr("이미지 오류"), 
                tr("이미지를 다운로드할 수 없습니다: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }
        
        QByteArray responseData = reply->readAll();
        reply->deleteLater();
        
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject responseObj = doc.object();
        
        if (responseObj.contains("image_data")) {
            QString base64Data = responseObj["image_data"].toString();
            QByteArray imageData = QByteArray::fromBase64(base64Data.toUtf8());
            
            QPixmap pixmap;
            if (!pixmap.loadFromData(imageData)) {
                qDebug() << "[Image] Failed to load image data into QPixmap";
                QMessageBox::warning(this, tr("이미지 오류"), tr("이미지를 표시할 수 없습니다."));
                return;
            }
            
            // Create and show image dialog
            QDialog* imageDialog = new QDialog(this);
            imageDialog->setWindowTitle(isOriginalImage ? 
                                       tr("원본 프로필 이미지") : 
                                       tr("프로필 이미지"));
            imageDialog->setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
            
            QVBoxLayout* layout = new QVBoxLayout(imageDialog);
            QLabel* imageLabel = new QLabel(imageDialog);
            imageLabel->setPixmap(pixmap);
            imageLabel->setScaledContents(true);
            
            // Set dialog size based on screen size
            QScreen* screen = QGuiApplication::primaryScreen();
            QRect screenGeometry = screen->geometry();
            
            int maxWidth = screenGeometry.width() * 0.8;
            int maxHeight = screenGeometry.height() * 0.8;
            
            if (pixmap.width() > maxWidth || pixmap.height() > maxHeight) {
                imageLabel->setFixedSize(maxWidth, maxHeight);
            } else {
                imageLabel->setFixedSize(pixmap.size());
            }
            
            layout->addWidget(imageLabel);
            imageDialog->setLayout(layout);
            
            imageDialog->exec();
        } else {
            qDebug() << "[Image] Response missing image_data";
            QMessageBox::warning(this, tr("이미지 오류"), tr("이미지 데이터를 받을 수 없습니다."));
        }
    });
}

//창 닫기
void DetailPageWidget::closeWindow()
{
    // Check for unsaved changes
    bool isModified = false;
    
    // Check name and phone number changes
    if (m_originalName != ui->detailpageName->text() || 
        m_originalPhoneNumber != ui->detailpagePhone->text()) {
        isModified = true;
    }
    
    // Check other field changes
    if (m_entry.email() != ui->detailpageMail->text() ||
        m_entry.company() != ui->detailpageCompany->text() ||
        m_entry.position() != ui->detailpagePosition->text() ||
        m_entry.nickname() != ui->detailpageNickname->text() ||
        m_entry.memo() != ui->detailpageNotice->text()) {
        isModified = true;
    }
    
    // Check image URL changes
    if (!m_currentImageUrl.isEmpty() && m_currentImageUrl != m_entry.imageUrl()) {
        isModified = true;
    }
    
    // Show warning if there are unsaved changes
    if (isModified && !m_saved) {
        QMessageBox::StandardButton reply = QMessageBox::warning(this, 
            tr("변경사항 저장 안됨"), 
            tr("변경사항이 저장되지 않았습니다. 계속하시겠습니까?"),
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::No) {
            return;
        }
    }
    
    close();
}

AddressEntry DetailPageWidget::updatedEntry() const {
    return m_entry;
}

void DetailPageWidget::closeEvent(QCloseEvent* event) {
    if (!m_saved) {
        bool isModified = false;
        
        if (m_originalName != ui->detailpageName->text() || 
            m_originalPhoneNumber != ui->detailpagePhone->text()) {
            isModified = true;
        }
        
        if (m_entry.email() != ui->detailpageMail->text() ||
            m_entry.company() != ui->detailpageCompany->text() ||
            m_entry.position() != ui->detailpagePosition->text() ||
            m_entry.nickname() != ui->detailpageNickname->text() ||
            m_entry.memo() != ui->detailpageNotice->text()) {
            isModified = true;
        }
        
        if (!m_currentImageUrl.isEmpty() && m_currentImageUrl != m_entry.imageUrl()) {
            isModified = true;
        }
        
        if (isModified) {
            QMessageBox::StandardButton reply = QMessageBox::warning(this, 
                tr("변경사항 저장 안됨"), 
                tr("변경사항이 저장되지 않았습니다. 계속하시겠습니까?"),
                QMessageBox::Yes | QMessageBox::No);
                
            if (reply == QMessageBox::No) {
                event->ignore();
                return;
            }
        }
        
        emit closedWithoutSaving();
        emit detailPageClosed();
    }
    QWidget::closeEvent(event);
}

void DetailPageWidget::setOriginalName(const QString& name)
{
    this->m_originalName = name;
}

void DetailPageWidget::setOriginalPhoneNumber(const QString& phoneNumber)
{
    this->m_originalPhoneNumber = phoneNumber;
}
