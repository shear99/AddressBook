#ifndef DETAILPAGEWIDGET_H
#define DETAILPAGEWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QFileDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QPixmap>
#include <QDesktopServices>
#include <QTimer>
#include <QApplication>
#include "addressentry.h"
#include "fontupdate.h"
#include "ui_detailpagewidget.h"
#include "loadingdialog.h"

namespace Ui {
class DetailPageWidget;
}

class DetailPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit DetailPageWidget(const AddressEntry& entry, QWidget* parent = nullptr, bool isAddMode = false);
    ~DetailPageWidget();
    AddressEntry updatedEntry() const; // 수정된 데이터를 외부에서 가져올 수 있도록
    void setOriginalName(const QString& name);
    void setOriginalPhoneNumber(const QString& phoneNumber);
    void modeSetting(bool _AddMode);
    void showImageDialog(const QString& imageUrl);
    void updateImageUI(const QString& imageUrl);

signals:
    void entryUpdated(const AddressEntry& updatedEntry); // 저장 클릭 시 signal로 알림
    void closedWithoutSaving(); // 저장 안 하고 닫힐 때용
    void detailPageClosed();    // save 버튼을 눌렀을때 시그널

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onSaveClicked();
    void closeWindow();
    void onEditButtonClicked();
    void onProfileImageClicked();
    void uploadImageToLambda(const QString& imagePath);
    void onImageUploadFinished(QNetworkReply* reply);
    void downloadImageFromS3(const QString& imageUrl);
    void fetchImageUrlFromDB();

private:
    bool m_isAddMode = false;
    QString origNameStyle, origPhoneStyle, origMailStyle, origCompanyStyle, origPositionStyle, origNicknameStyle, origMemoStyle;
    AddressEntry m_entry;

    QPushButton* saveButton;

    void setupUI();
    void populateFields();
    void editInitialSettings();
    bool m_saved = false;

    QString m_originalName;
    QString m_originalPhoneNumber;
    
    // 프로필 이미지 관련 변수
    QString m_currentImageUrl;
    QString m_originalS3Key;
    QNetworkAccessManager* m_networkManager;

    Ui::DetailPageWidget *ui;
};

#endif // DETAILPAGEWIDGET_H
