#include "detailpagewidget.h"

DetailPageWidget::DetailPageWidget(const AddressEntry& entry, QWidget* parent)
    : QWidget(parent), m_entry(entry)
{
    setupUI();
    populateFields();
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
    layout->addRow("Name", nameEdit);
    layout->addRow("Phone", phoneEdit);
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
    m_entry.setName(nameEdit->text());
    m_entry.setPhoneNumber(phoneEdit->text());
    m_entry.setEmail(emailEdit->text());
    m_entry.setCompany(companyEdit->text());
    m_entry.setPosition(positionEdit->text());
    m_entry.setNickname(nicknameEdit->text());

    m_saved = true;
    emit entryUpdated(m_entry);
    emit detailPageClosed(); // ✅ 메인 페이지에 알려주기
    this->close();
}

AddressEntry DetailPageWidget::updatedEntry() const {
    return m_entry;
}

void DetailPageWidget::closeEvent(QCloseEvent* event) {
    if (!m_saved) {
        emit closedWithoutSaving(); // 저장 안 한 경우
        emit detailPageClosed();    // ✅ 그래도 무조건 닫혔다고 알려줌
    }
    QWidget::closeEvent(event);
}
