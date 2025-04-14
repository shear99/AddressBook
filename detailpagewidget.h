#ifndef DETAILPAGEWIDGET_H
#define DETAILPAGEWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include "addressentry.h"

class DetailPageWidget : public QWidget {
    Q_OBJECT

public:
    explicit DetailPageWidget(const AddressEntry& entry, QWidget* parent = nullptr);
    AddressEntry updatedEntry() const; // 수정된 데이터를 외부에서 가져올 수 있도록

signals:
    void entryUpdated(const AddressEntry& updatedEntry); // 저장 클릭 시 signal로 알림
    void closedWithoutSaving(); // 저장 안 하고 닫힐 때용
    void detailPageClosed();    // save 버튼을 눌렀을때 시그널

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onSaveClicked();

private:
    AddressEntry m_entry;

    QLineEdit* nameEdit;
    QLineEdit* phoneEdit;
    QLineEdit* emailEdit;
    QLineEdit* companyEdit;
    QLineEdit* positionEdit;
    QLineEdit* nicknameEdit;

    QPushButton* saveButton;

    void setupUI();
    void populateFields();

    bool m_saved = false;
};

#endif // DETAILPAGEWIDGET_H
