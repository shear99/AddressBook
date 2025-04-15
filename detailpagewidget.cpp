#include <QMessageBox>
#include <QCloseEvent>
#include "util.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QMessageBox>
#include "detailpagewidget.h"

DetailPageWidget::DetailPageWidget(const AddressEntry& entry, QWidget* parent, bool isAddMode)
    : QWidget(parent), m_entry(entry),ui(new Ui::DetailPageWidget),m_isAddMode(isAddMode)
{
    ui->setupUi(this);

    setWindowFlags(Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    FontUpdate::applyFontToAllChildren(this, ":/fonts/fonts/GmarketSansTTFMedium.ttf");

    // setupUI();
    populateFields();
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

    modeSetting(m_isAddMode);

}

void DetailPageWidget::modeSetting(bool _AddMode)
{
    //추가 모드
    if (_AddMode) {
        // 전화, , 약속초대 버튼 숨기기
        ui->detailpageCall->hide();
        ui->detailpagemailImage->hide();
        ui->detailpageMessage->hide();
        ui->detailpageInvite->hide();
        ui->detailpageLabel->hide();
        ui->detailpageeditButton->hide();
        ui->detailpagesaveButton->hide();
        ui->detailpageiconOutline->hide();
        //

        QString editStyle = "QLineEdit { background: white; }";
        ui->detailpageName->setStyleSheet(editStyle);
        ui->detailpagePhone->setStyleSheet(editStyle);
        ui->detailpageMail->setStyleSheet(editStyle);
        ui->detailpageNickname->setStyleSheet(editStyle);

        ui->infoLayout->setContentsMargins(20, 10, 10, 10); // 왼쪽 마진
        // ui->infoLayout->add("이름", ui->detailpageName);
        // ui->infoLayout->addRow("전화번호", ui->detailpagePhone);
        // ui->infoLayout->addRow("메일", ui->detailpageMail);
        // ui->infoLayout->addRow("별명", ui->detailpageNickname);
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
    m_entry.setEmail(ui->detailpageMail->text());
    m_entry.setCompany(ui->detailpageCompany->text());
    m_entry.setPosition(ui->detailpagePosition->text());
    m_entry.setNickname(ui->detailpageNickname->text());

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
