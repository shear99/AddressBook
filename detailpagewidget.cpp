#include "detailpagewidget.h"
#include <QMessageBox>
#include <QCloseEvent>
#include "util.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QMessageBox>

DetailPageWidget::DetailPageWidget(const AddressEntry& entry, QWidget* parent)
    : QWidget(parent), m_entry(entry)
{
    setupUI();
    populateFields();

    // 현재 값을 최초 원본값으로 저장.
    setOriginalName(m_entry.name());
    setOriginalPhoneNumber(m_entry.phoneNumber());
}

void DetailPageWidget::setupUI() {
    nameEdit = new QLineEdit(this);
    phoneEdit = new QLineEdit(this);
    emailEdit = new QLineEdit(this);
    companyEdit = new QLineEdit(this);
    positionEdit = new QLineEdit(this);
    nicknameEdit = new QLineEdit(this);

    saveButton = new QPushButton("Save", this);
    connect(saveButton, &QPushButton::clicked, this, &DetailPageWidget::onSaveClicked);

    QFormLayout* layout = new QFormLayout(this);
    layout->addRow("Name*", nameEdit);
    layout->addRow("Phone*", phoneEdit);
    layout->addRow("Email", emailEdit);
    layout->addRow("Company", companyEdit);
    layout->addRow("Position", positionEdit);
    layout->addRow("Nickname", nicknameEdit);
    layout->addWidget(saveButton);

    setLayout(layout);
}

void DetailPageWidget::populateFields() {
    nameEdit->setText(m_entry.name());
    phoneEdit->setText(m_entry.phoneNumber());
    emailEdit->setText(m_entry.email());
    companyEdit->setText(m_entry.company());
    positionEdit->setText(m_entry.position());
    nicknameEdit->setText(m_entry.nickname());
}

void DetailPageWidget::onSaveClicked() {
    // 필수 입력 검사
    if (nameEdit->text().trimmed().isEmpty() || phoneEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("입력 오류"), tr("이름과 전화번호는 반드시 입력되어야 합니다."));
        return;
    }

    // 전화번호 양식 검증
    QString phone = phoneEdit->text().trimmed();
    QRegularExpression regex("^\\d{3}-\\d{4}-\\d{4}$");
    QRegularExpressionMatch match = regex.match(phone);
    if (!match.hasMatch()) {
        QMessageBox::warning(this, tr("입력 오류"), tr("전화번호는 000-0000-0000 형식이어야 합니다."));
        return;
    }

    // 새 값 임시 저장
    QString newName = nameEdit->text();
    QString newPhone = phone;
    bool isModified = (m_originalName != newName) || (m_originalPhoneNumber != newPhone);

    // ---------- 다이얼로그 띄우기 ----------
    LoadingDialog dialog(this);
    dialog.show();
    QCoreApplication::processEvents();  // UI 업데이트 강제

    // 기존 항목 삭제 요청 (수정된 경우에만)
    if (isModified) {
        if (!deleteAddressEntryFromAWS(m_entry, getAwsSaveUrl())) {
            dialog.close();  // 닫기 꼭 해야 함!
            QMessageBox::warning(this, tr("삭제 오류"), tr("이전 튜플 삭제에 실패했습니다."));
            return;
        }
    }

    // 업데이트
    m_entry.setName(newName);
    m_entry.setPhoneNumber(newPhone);
    m_entry.setEmail(emailEdit->text());
    m_entry.setCompany(companyEdit->text());
    m_entry.setPosition(positionEdit->text());
    m_entry.setNickname(nicknameEdit->text());

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
