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
    ui->setupUi(this);

    // 최대화/최소화 버튼 제거
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint & ~Qt::WindowMinimizeButtonHint);

    setWindowFlags(Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    FontUpdate::applyFontToAllChildren(this, ":/fonts/fonts/GmarketSansTTFMedium.ttf");

    // 네트워크 매니저 초기화
    m_networkManager = new QNetworkAccessManager(this);
    
    // 프로필 이미지 이벤트 필터 설치
    ui->detailpageImage->installEventFilter(this);
    ui->detailpageImage->setCursor(Qt::PointingHandCursor); // 클릭 가능함을 나타내는 커서
    
    // 기본 필드 설정 (텍스트 정보)
    populateFields();
    
    // DB에서 이미지 URL 조회 및 로드 (비동기)
    if (m_isAddMode == false) {
        fetchImageUrlFromDB();
    }
    else {
        // 기본 이미지 로드 (추가 모드)
        if (!m_entry.imageUrl().isEmpty()) {
            downloadImageFromS3(m_entry.imageUrl());
        }
    }

    connect(ui->detailpagesaveButton, &QPushButton::clicked, this, &DetailPageWidget::onSaveClicked);
    connect(ui->detailpageexitButton, &QPushButton::clicked, this, &DetailPageWidget::closeWindow);
    connect(ui->detailpageeditButton, &QPushButton::clicked, this, &DetailPageWidget::onEditButtonClicked);

    // 현재 값을 최초 원본값으로 저장.
    setOriginalName(m_entry.name());
    setOriginalPhoneNumber(m_entry.phoneNumber());

    //초기 스타일 시트 저장
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
    // 모델이 제공되면 중복 체크 함수 설정
    if (model) {
        m_duplicateCheckFunc = [model](const QString& name, const QString& phone) -> bool {
            for (int i = 0; i < model->rowCount(); i++) {
                AddressEntry entry = model->getEntry(i);
                if (entry.name() == name && entry.phoneNumber() == phone) {
                    return true; // 중복 발견
                }
            }
            return false; // 중복 없음
        };
    }

    ui->setupUi(this);

    // 최대화/최소화 버튼 제거
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint & ~Qt::WindowMinimizeButtonHint);

    setWindowFlags(Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    FontUpdate::applyFontToAllChildren(this, ":/fonts/fonts/GmarketSansTTFMedium.ttf");

    // 네트워크 매니저 초기화
    m_networkManager = new QNetworkAccessManager(this);
    
    // 프로필 이미지 이벤트 필터 설치
    ui->detailpageImage->installEventFilter(this);
    ui->detailpageImage->setCursor(Qt::PointingHandCursor); // 클릭 가능함을 나타내는 커서
    
    // 기본 필드 설정 (텍스트 정보)
    populateFields();
    
    // DB에서 이미지 URL 조회 및 로드 (비동기)
    if (m_isAddMode == false) {
        fetchImageUrlFromDB();
    }
    else {
        // 기본 이미지 로드 (추가 모드)
        if (!m_entry.imageUrl().isEmpty()) {
            downloadImageFromS3(m_entry.imageUrl());
        }
    }

    connect(ui->detailpagesaveButton, &QPushButton::clicked, this, &DetailPageWidget::onSaveClicked);
    connect(ui->detailpageexitButton, &QPushButton::clicked, this, &DetailPageWidget::closeWindow);
    connect(ui->detailpageeditButton, &QPushButton::clicked, this, &DetailPageWidget::onEditButtonClicked);

    // 현재 값을 최초 원본값으로 저장.
    setOriginalName(m_entry.name());
    setOriginalPhoneNumber(m_entry.phoneNumber());

    //초기 스타일 시트 저장
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
    
    // AWS Lambda URL 가져오기
    QUrl loadUrl = getAwsLoadUrl();
    
    // 요청 보내기
    QNetworkRequest request(loadUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject queryObj;
    queryObj["phoneNumber"] = m_entry.phoneNumber();
    queryObj["name"] = m_entry.name();
    
    QJsonObject root;
    root["query"] = queryObj;
    
    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson();
    
    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    
    // 응답 처리
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[DB Image] Error fetching image URL:" << reply->errorString();
            return;
        }
        
        QByteArray responseData = reply->readAll();
        qDebug() << "[DB Image] Response:" << responseData;
        
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        QJsonObject responseObj = responseDoc.object();
        
        if (responseObj.contains("results") && responseObj["results"].isArray()) {
            QJsonArray results = responseObj["results"].toArray();
            qDebug() << "[DB Image] Results count:" << results.size();
            
            if (results.size() > 0) {
                for (int i = 0; i < results.size(); i++) {
                    QJsonObject entryObj = results[i].toObject();
                    
                    // 연락처 확인
                    if (entryObj["phoneNumber"].toString() == m_entry.phoneNumber() && 
                        entryObj["name"].toString() == m_entry.name()) {
                        
                        qDebug() << "[DB Image] Found matching entry at index:" << i;
                        
                        if (entryObj.contains("image")) {
                            QString imageUrl = entryObj["image"].toString();
                            qDebug() << "[DB Image] Found image URL:" << imageUrl;
                            
                            // 원본 이미지 키 저장
                            if (entryObj.contains("original_key")) {
                                m_originalS3Key = entryObj["original_key"].toString();
                                qDebug() << "[DB Image] Original S3 key:" << m_originalS3Key;
                            }
                            
                            // 이미지 URL 저장 및 다운로드
                            if (!imageUrl.isEmpty()) {
                                m_currentImageUrl = imageUrl;
                                m_entry.setImageUrl(imageUrl);
                                
                                // 이미지 다운로드 및 표시
                                downloadImageFromS3(imageUrl);
                            }
                        } else {
                            qDebug() << "[DB Image] Entry doesn't have image URL";
                        }
                        break;
                    }
                }
            } else {
                qDebug() << "[DB Image] No results found";
            }
        } else {
            qDebug() << "[DB Image] Response doesn't contain results array";
        }
    });
}

// 새로운 함수 추가
void DetailPageWidget::updateImageUI(const QString& imageUrl)
{
    qDebug() << "[Image] Updating UI with image URL:" << imageUrl;
    // 이 함수는 이미지 관련 UI만 업데이트
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
    // 추가 모드에서는 이미지 업로드 차단
    if (m_isAddMode) {
        QMessageBox::information(this, tr("알림"), 
            tr("지금은 이미지를 업로드할 수 없습니다. 주소록에 추가한 후 사진을 업로드해주세요."));
        return;
    }

    // 읽기 전용 모드에서는 원본 이미지 보기
    if (ui->detailpageName->isReadOnly() && !m_isAddMode) {
        if (!m_originalS3Key.isEmpty()) {
            QString originalUrl = QString("https://contact-photo-bucket-001.s3.amazonaws.com/%1")
                               .arg(m_originalS3Key);
            qDebug() << "[Image] Showing original image:" << originalUrl;
            showImageDialog(originalUrl);
        }
        else {
            qDebug() << "[Image] No original S3 key available";
        }
    } 
    // 편집 모드에서는 이미지 선택 다이얼로그
    else {
        QString filePath = QFileDialog::getOpenFileName(this, 
            tr("프로필 이미지 선택"), 
            QString(), 
            tr("이미지 파일 (*.jpg *.jpeg *.png)"));
            
        if (!filePath.isEmpty()) {
            qDebug() << "[Image] Selected file path:" << filePath;
            QFileInfo fileInfo(filePath);
            QString extension = fileInfo.suffix().toLower();
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
    
    // 로딩 다이얼로그 표시
    LoadingDialog* loadingDialog = new LoadingDialog(this);
    loadingDialog->show();
    QCoreApplication::processEvents();
    
    // 파일 읽기
    QFile file(imagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[Image] Failed to open file:" << file.errorString();
        QMessageBox::warning(this, tr("파일 오류"), tr("이미지 파일을 열 수 없습니다."));
        loadingDialog->close();
        loadingDialog->deleteLater();
        return;
    }
    
    // S3에 업로드할 임시 파일명 생성 (uploads/ 경로 사용)
    QString fileName = QString("uploads/%1_%2.jpg")
                      .arg(m_entry.phoneNumber().replace("-", ""))
                      .arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
    qDebug() << "[Image] Generated S3 key:" << fileName;
    
    // Lambda 요청 준비
    QUrl lambdaUrl = getAwsImageResizeUrl();
    qDebug() << "[Image] Lambda URL:" << lambdaUrl.toString();
    QNetworkRequest request(lambdaUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 이미지 데이터 읽기
    QByteArray fileData = file.readAll();
    file.close();
    qDebug() << "[Image] Read file size:" << fileData.size() << "bytes";
    
    // Base64로 이미지 인코딩
    QByteArray base64Data = fileData.toBase64();
    qDebug() << "[Image] Base64 encoded size:" << base64Data.size() << "bytes";
    
    QJsonObject jsonObj;
    jsonObj["key"] = fileName;
    jsonObj["phoneNumber"] = m_entry.phoneNumber();
    jsonObj["name"] = m_entry.name();
    jsonObj["image_data"] = QString(base64Data); // 이미지 데이터 Base64 인코딩
    
    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson();
    qDebug() << "[Image] JSON payload size:" << jsonData.size() << "bytes";
    
    // Lambda 함수 호출
    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    qDebug() << "[Image] Network request sent";
    
    // connect 블록 수정 - 로컬 변수를 레퍼런스로 접근하면 함수 종료 후 유효하지 않음
    connect(reply, &QNetworkReply::finished, this, [this, reply, loadingDialog]() {
        loadingDialog->close();
        loadingDialog->deleteLater();
        onImageUploadFinished(reply);
    });
}

void DetailPageWidget::onImageUploadFinished(QNetworkReply* reply)
{
    qDebug() << "[Image] Received response from Lambda";
    
    // 응답 파싱 전에 에러 체크
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "[Image] Network error:" << reply->errorString();
        QMessageBox::warning(this, tr("업로드 오류"), 
            tr("이미지 업로드 중 오류가 발생했습니다: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    // 응답 파싱
    QByteArray responseData = reply->readAll();
    reply->deleteLater(); // 여기서 일찍 deleteLater 호출
    
    qDebug() << "[Image] Raw response:" << responseData;
    
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
        qDebug() << "[Image] Received S3 URL:" << imageUrl;
        m_currentImageUrl = imageUrl;
        
        // 원본 이미지 키 저장
        if (responseObj.contains("original_key")) {
            m_originalS3Key = responseObj["original_key"].toString();
            qDebug() << "[Image] Original S3 key:" << m_originalS3Key;
        }
        
        try {
            // 이미지 다운로드 및 표시
            downloadImageFromS3(imageUrl);
            
            // AddressEntry 업데이트
            m_entry.setImageUrl(imageUrl);
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
    
    // 로딩 다이얼로그 표시
    LoadingDialog* loadingDialog = new LoadingDialog(this);
    loadingDialog->show();
    QCoreApplication::processEvents();
    
    // S3 URL에서 키 추출
    QString key = imageUrl.split(".s3.amazonaws.com/")[1];
    qDebug() << "[Image] Extracted S3 key:" << key;
    
    // Lambda 요청 준비
    QUrl lambdaUrl = getAwsImageResizeUrl();
    QNetworkRequest request(lambdaUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 이미지 요청 JSON 생성
    QJsonObject jsonObj;
    jsonObj["action"] = "get_image";
    jsonObj["key"] = key;
    
    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson();
    
    // Lambda 함수 호출
    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    
    // 연결 타임아웃 설정
    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [reply]() {
        if (!reply->isFinished()) {
            reply->abort();
        }
    });
    timer->start(10000); // 10초 타임아웃
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, timer, loadingDialog]() {
        qDebug() << "[Image] Download completed";
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
            
            // 프로필 이미지 라벨에 표시
            ui->detailpageImage->setStyleSheet("");
            ui->detailpageImage->setPixmap(pixmap);
            ui->detailpageImage->setScaledContents(true);
            qDebug() << "[Image] Image displayed successfully";
        } else {
            qDebug() << "[Image] Response missing image_data";
        }
    });
}

void DetailPageWidget::modeSetting(bool _AddMode)
{
    //추가 모드
    if (_AddMode) {
        ui->detailpageTitle->setText("연락처 추가");
        //편집모드 버튼 숨기기
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

        ui->infoVLayout->setContentsMargins(5, 0, 0, 0); // 왼쪽 마진
        // 레이블 생성 및 LineEdit 재배치
        QLabel* nameLabel = new QLabel("Name:", this);
        QLabel* phoneLabel = new QLabel("Phone:", this);
        QLabel* positionLabel = new QLabel("Position:", this);
        QLabel* nicknameLabel = new QLabel("Nickname:", this);

        // QFormLayout 오른쪽 정렬
        ui->infoFormLayout->setLabelAlignment(Qt::AlignRight);

        ui->infoFormLayout->addRow(nameLabel, ui->detailpageName);
        ui->infoFormLayout->addRow(phoneLabel, ui->detailpagePhone);
        ui->infoFormLayout->addRow(positionLabel, ui->detailpagePosition);
        ui->infoFormLayout->addRow(nicknameLabel, ui->detailpageNickname);

        ui->detailpageCompany->setStyleSheet(editStyle);
        ui->detailpagePosition->setStyleSheet(editStyle);
        connect(ui->detailpageaddnewButton, &QPushButton::clicked, this, &DetailPageWidget::onSaveClicked);
    }
    //일반 모드
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
    
    // 이미지 URL이 있으면 이미지 다운로드 및 표시
    if (!m_entry.imageUrl().isEmpty()) {
        m_currentImageUrl = m_entry.imageUrl();
        downloadImageFromS3(m_currentImageUrl);
    }
}

void DetailPageWidget::onSaveClicked() {
    // 필수 입력 검사
    if (ui->detailpageName->text().trimmed().isEmpty() || ui->detailpagePhone->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("입력 오류"), tr("이름과 전화번호는 반드시 입력되어야 합니다."));
        return;
    }

    // 전화번호 양식 검증
    QString phone = ui->detailpagePhone->text().trimmed();
    QRegularExpression regex("^\\d{3}-\\d{4}-\\d{4}$");
    QRegularExpressionMatch match = regex.match(phone);
    if (!match.hasMatch()) {
        QMessageBox::warning(this, tr("입력 오류"), tr("전화번호는 000-0000-0000 형식이어야 합니다."));
        return;
    }

    // 새 값 임시 저장
    QString newName = ui->detailpageName->text();
    QString newPhone = phone;
    
    // 추가 모드에서 중복 체크 함수 호출
    if (m_isAddMode && m_duplicateCheckFunc) {
        bool isDuplicate = m_duplicateCheckFunc(newName, newPhone);
        
        if (isDuplicate) {
            QMessageBox::warning(this, tr("중복 오류"), 
                tr("동일한 이름과 전화번호를 가진 연락처가 이미 존재합니다."));
            return;
        }
    }
    
    bool isModified = (m_originalName != newName) || (m_originalPhoneNumber != newPhone);

    // ---------- 다이얼로그 띄우기 ----------
    LoadingDialog dialog(this);
    dialog.show();
    QCoreApplication::processEvents();  // UI 업데이트 강제

    // 기존 항목 삭제 요청 (수정된 경우에만, 추가 모드가 아닐 때만)
    if (isModified && !m_isAddMode) {
        if (!deleteAddressEntryFromAWS(m_entry, getAwsSaveUrl())) {
            dialog.close();  // 닫기 꼭 해야 함!
            QMessageBox::warning(this, tr("삭제 오류"), tr("이전 튜플 삭제에 실패했습니다."));
            return;
        }
    }

    // 업데이트
    m_entry.setName(newName);
    m_entry.setPhoneNumber(newPhone);
    m_entry.setEmail(ui->detailpageMail->text());
    m_entry.setCompany(ui->detailpageCompany->text());
    m_entry.setPosition(ui->detailpagePosition->text());
    m_entry.setNickname(ui->detailpageNickname->text());
    m_entry.setMemo(ui->detailpageNotice->text());
    
    // 이미지 URL 저장 (프로필 이미지가 변경된 경우)
    if (!m_currentImageUrl.isEmpty()) {
        m_entry.setImageUrl(m_currentImageUrl);
    }

    if (isModified) {
        setOriginalName(newName);
        setOriginalPhoneNumber(newPhone);
    }

    dialog.close();  // 통신 종료 후 닫기

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
};

void DetailPageWidget::onEditButtonClicked()
{
    bool isReadOnly = ui->detailpageName->isReadOnly();
    QString editStyle = "QLineEdit { background: white; }";

    // 편집 모드: 흰색, 읽기 전용: 원래 스타일 복원
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
    
    // 이미지 다운로드 요청
    QUrl lambdaUrl = getAwsImageResizeUrl();
    QNetworkRequest request(lambdaUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject jsonObj;
    jsonObj["action"] = "get_image";
    jsonObj["key"] = imageUrl.split(".s3.amazonaws.com/")[1];
    
    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson();
    
    // 로딩 다이얼로그 표시 - 포인터로 생성
    LoadingDialog* loadingDialog = new LoadingDialog(this);
    loadingDialog->show();
    QCoreApplication::processEvents();
    
    QNetworkReply* reply = m_networkManager->post(request, jsonData);
    
    // 값으로 캡처하여 로컬 변수 참조 문제 해결
    connect(reply, &QNetworkReply::finished, this, [this, reply, loadingDialog]() {
        loadingDialog->close();
        loadingDialog->deleteLater(); // 메모리 누수 방지
        
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
            
            // 이미지 뷰어 다이얼로그 생성
            QDialog* imageDialog = new QDialog(this);
            imageDialog->setWindowTitle(tr("프로필 이미지"));
            imageDialog->setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
            
            QVBoxLayout* layout = new QVBoxLayout(imageDialog);
            QLabel* imageLabel = new QLabel(imageDialog);
            imageLabel->setPixmap(pixmap);
            imageLabel->setScaledContents(true);
            
            // 화면 크기 가져오기
            QScreen* screen = QGuiApplication::primaryScreen();
            QRect screenGeometry = screen->geometry();
            
            // 다이얼로그 크기 설정 (이미지 크기에 맞춤)
            int maxWidth = screenGeometry.width() * 0.8;
            int maxHeight = screenGeometry.height() * 0.8;
            
            if (pixmap.width() > maxWidth || pixmap.height() > maxHeight) {
                imageLabel->setFixedSize(maxWidth, maxHeight);
            } else {
                imageLabel->setFixedSize(pixmap.size());
            }
            
            layout->addWidget(imageLabel);
            imageDialog->setLayout(layout);
            
            // 다이얼로그를 모달로 표시
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
    close();
}

AddressEntry DetailPageWidget::updatedEntry() const {
    return m_entry;
}

void DetailPageWidget::closeEvent(QCloseEvent* event) {
    if (!m_saved) {
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
