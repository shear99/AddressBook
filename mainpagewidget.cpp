#include "mainpagewidget.h"
#include "ui_mainpagewidget.h"
#include "AddressBookModel.h"
#include "detailpagewidget.h"
#include "HeartDelegate.h"

#include <QHeaderView>
#include <QMessageBox>

MainPageWidget::MainPageWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainPageWidget)
{
    ui->setupUi(this);

    // 모델 생성 및 테이블뷰에 연결
    AddressBookModel* model = new AddressBookModel(this);
    ui->addressTableView->setModel(model);

    // 첫 번째 열(즐겨찾기)에는 커스텀 하트 delegate 적용
    HeartDelegate* heartDelegate = new HeartDelegate(this);
    ui->addressTableView->setItemDelegateForColumn(0, heartDelegate);

    // 첫 번째 열 너비를 줄임 (예: 24 픽셀)
    ui->addressTableView->setColumnWidth(0, 24);

    // 단일 클릭 시 편집(체크박스 토글) 허용
    ui->addressTableView->setEditTriggers(QAbstractItemView::SelectedClicked);

    // 테이블뷰 기본 설정
    ui->addressTableView->horizontalHeader()->setStretchLastSection(true);
    ui->addressTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->addressTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->addressTableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // 더블클릭 시 상세 편집 페이지로 이동 (기존 기능)
    connect(ui->addressTableView, &QTableView::doubleClicked, this, [=](const QModelIndex &index) {
        if (!index.isValid())
            return;

        int row = index.row();
        AddressEntry entry = model->getEntry(row);
        DetailPageWidget* detailPage = new DetailPageWidget(entry);
        this->hide();
        detailPage->show();

        connect(detailPage, &DetailPageWidget::entryUpdated, this, [=](const AddressEntry &updatedEntry) {
            model->updateEntry(row, updatedEntry);
        });
        connect(detailPage, &DetailPageWidget::detailPageClosed, this, [=]() {
            this->show();
        });
    });

    // addButton 클릭 시 빈 DetailPageWidget을 열어서 새 항목 추가
    connect(ui->addButton, &QPushButton::clicked, this, [=]() {
        // 빈 AddressEntry 생성. 필수 필드(이름, 전화번호)는 DetailPageWidget에서 검증
        AddressEntry newEntry;
        DetailPageWidget* detailPage = new DetailPageWidget(newEntry);
        this->hide();
        detailPage->show();

        connect(detailPage, &DetailPageWidget::entryUpdated, this, [=](const AddressEntry &updatedEntry) {
            model->addEntry(updatedEntry);
        });
        connect(detailPage, &DetailPageWidget::detailPageClosed, this, [=]() {
            this->show();
        });
    });

    // deleteButton 클릭 시, 현재 선택된 항목 삭제 (삭제 확인 다이얼로그 포함)
    connect(ui->deleteButton, &QPushButton::clicked, this, [=]() {
        QItemSelectionModel* selectionModel = ui->addressTableView->selectionModel();
        QModelIndexList selectedRows = selectionModel->selectedRows();
        if (selectedRows.isEmpty()) {
            QMessageBox::information(this, tr("삭제"), tr("삭제할 항목을 선택하세요."));
            return;
        }

        int row = selectedRows.first().row();
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  tr("삭제 확인"),
                                                                  tr("삭제하시겠습니까?"),
                                                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            model->removeEntry(row);
        }
    });
}

MainPageWidget::~MainPageWidget()
{
    delete ui;
}
