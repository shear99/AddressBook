#include "mainpagewidget.h"
#include "ui_mainpagewidget.h"
#include "detailpagewidget.h"

MainPageWidget::MainPageWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainPageWidget)
{
    ui->setupUi(this);

    // 모델 생성 및 설정
    model = new AddressBookModel(this);
    ui->addressTableView->setModel(model);

    // ⭐ 단일 클릭으로 편집(하트 클릭) 가능하게
    ui->addressTableView->setEditTriggers(QAbstractItemView::SelectedClicked);

    // 테이블 옵션 설정
    ui->addressTableView->horizontalHeader()->setStretchLastSection(true);
    ui->addressTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->addressTableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // 더블클릭 시 상세 페이지 열기
    connect(ui->addressTableView, &QTableView::doubleClicked, this, [=](const QModelIndex& index) {
        if (!index.isValid()) return;

        int row = index.row();
        AddressEntry entry = model->getEntry(row);
        DetailPageWidget* detailPage = new DetailPageWidget(entry);

        this->hide(); // 메인 페이지 숨기기
        detailPage->show();

        connect(detailPage, &DetailPageWidget::entryUpdated, this, [=](const AddressEntry& updatedEntry) {
            model->updateEntry(row, updatedEntry);
        });

        connect(detailPage, &DetailPageWidget::detailPageClosed, this, [=]() {
            this->show(); // 저장 여부와 무관하게 항상 다시 보이게 함
        });
    });
}

MainPageWidget::~MainPageWidget()
{
    delete ui;
}
