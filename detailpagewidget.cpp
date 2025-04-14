#include "detailpagewidget.h"
#include <QMessageBox>
#include <QCloseEvent>
#include "util.h"

DetailPageWidget::DetailPageWidget(const AddressEntry& entry, QWidget* parent)
    : QWidget(parent), m_entry(entry)
{
    setupUI();
    populateFields();

    // 만약 기존 원본 키가 비어있다면, 현재 값을 최초 원본값으로 저장.
    if(m_entry.originalName().isEmpty())
        m_entry.setOriginalName(m_entry.name());
    if(m_entry.originalPhoneNumber().isEmpty())
        m_entry.setOriginalPhoneNumber(m_entry.phoneNumber());
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

    // 사용자가 입력한 새 값을 임시로 보관
    QString newName = nameEdit->text();
    QString newPhone = phoneEdit->text();

    // 수정 여부 판단: 원본과 새 값이 다른지 확인
    bool isModified = (m_entry.originalName() != newName) || (m_entry.originalPhoneNumber() != newPhone);

    // 만약 수정된 경우, 기존 원본값을 바탕으로 삭제 요청
    if (isModified) {
        // 기존 원본 키로 삭제 요청 (deleteAddressEntryFromAWS는 util.h에 정의되어 있음)
        if (!deleteAddressEntryFromAWS(m_entry, getAwsSaveUrl())) {
            QMessageBox::warning(this, tr("삭제 오류"), tr("이전 튜플 삭제에 실패했습니다."));
            return;  // 삭제 실패하면 저장 진행 중단
        }
    }

    // 사용자가 입력한 값으로 현재 entry 업데이트
    m_entry.setName(newName);
    m_entry.setPhoneNumber(newPhone);
    m_entry.setEmail(emailEdit->text());
    m_entry.setCompany(companyEdit->text());
    m_entry.setPosition(positionEdit->text());
    m_entry.setNickname(nicknameEdit->text());

    // 만약 수정이 일어났다면, 삭제 후 정상적으로 저장이 이뤄지면 원본 키를 새 값으로 갱신
    if(isModified) {
        m_entry.setOriginalName(newName);
        m_entry.setOriginalPhoneNumber(newPhone);
    }

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
