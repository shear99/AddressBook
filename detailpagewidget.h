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
#include "src/core/addressentry.h"
#include "src/util/fontupdate.h"
#include "ui_detailpagewidget.h"
#include "loadingdialog.h"
#include "src/core/addressbookmodel.h"

namespace Ui {
class DetailPageWidget;
}

// Function pointer type for duplicate contact checking
typedef std::function<bool(const QString&, const QString&)> DuplicateCheckFunction;

class DetailPageWidget : public QWidget {
    Q_OBJECT

public:
    // Constructors for different initialization scenarios
    explicit DetailPageWidget(const AddressEntry& entry, QWidget* parent = nullptr, bool isAddMode = false);
    explicit DetailPageWidget(const AddressEntry& entry, QWidget* parent, bool isAddMode, AddressBookModel* model);
    ~DetailPageWidget();

    // Core functionality for data management
    AddressEntry updatedEntry() const;
    void setOriginalName(const QString& name);
    void setOriginalPhoneNumber(const QString& phoneNumber);
    void modeSetting(bool _AddMode);
    void showImageDialog(const QString& imageUrl);
    void updateImageUI(const QString& imageUrl);

signals:
    // Signals for data synchronization with AddressBookModel
    void entryUpdated(const AddressEntry& updatedEntry);
    void closedWithoutSaving();
    void detailPageClosed();

protected:
    // Event handling for window management
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    // Core operations for data and image management
    void onSaveClicked();           // Save contact data to AWS
    void closeWindow();             // Handle window closing
    void onEditButtonClicked();     // Toggle edit mode
    void onProfileImageClicked();   // Handle image selection/display
    void uploadImageToLambda(const QString& imagePath);  // Upload image to AWS
    void onImageUploadFinished(QNetworkReply* reply);    // Handle image upload completion
    void downloadImageFromS3(const QString& imageUrl);   // Download image from AWS
    void fetchImageUrlFromDB();     // Retrieve image URL from AWS

private:
    // State management
    bool m_isAddMode = false;
    QString origNameStyle, origPhoneStyle, origMailStyle, origCompanyStyle, origPositionStyle, origNicknameStyle, origMemoStyle;
    AddressEntry m_entry;
    DuplicateCheckFunction m_duplicateCheckFunc = nullptr;

    // UI components
    QPushButton* saveButton;
    Ui::DetailPageWidget *ui;

    // UI management
    void setupUI();
    void populateFields();
    void editInitialSettings();
    bool m_saved = false;

    // Data tracking
    QString m_originalName;
    QString m_originalPhoneNumber;
    
    // AWS image management
    QString m_currentImageUrl;
    QString m_originalS3Key;
    QNetworkAccessManager* m_networkManager;
};

#endif // DETAILPAGEWIDGET_H
