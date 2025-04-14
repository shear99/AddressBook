#include "mainpagewidget.h"
#include "ui_mainpagewidget.h"
#include "AddressBookModel.h"
#include "detailpagewidget.h"
#include "HeartDelegate.h"  // 하트 이미지용 delegate

#include <QHeaderView>

MainPageWidget::MainPageWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainPageWidget)
{
    ui->setupUi(this);

    // 모델 생성 및 테이블뷰에 연결
    AddressBookModel* model = new AddressBookModel(this);
    ui->addressTableView->setModel(model);

    // 즐겨찾기(하트) 칼럼에 커스텀 delegate 설정
    HeartDelegate* heartDelegate = new HeartDelegate(this);
    ui->addressTableView->setItemDelegateForColumn(0, heartDelegate);

    // 컬럼 너비 설정: 하트 아이콘 칼럼만 좁게
    ui->addressTableView->setColumnWidth(0, 24);  // 하트 칼럼 너비

    // 체크박스 토글을 위해 클릭을 허용 (셀 클릭 시 수정되게)
    ui->addressTableView->setEditTriggers(QAbstractItemView::SelectedClicked);

    // 기본 테이블뷰 동작 설정
    ui->addressTableView->horizontalHeader()->setStretchLastSection(true);
    ui->addressTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->addressTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->addressTableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // 더블클릭 시 상세 페이지로 이동
    connect(ui->addressTableView, &QTableView::doubleClicked, this, [=](const QModelIndex &index) {
        if (!index.isValid())
            return;

        int row = index.row();
        AddressEntry entry = model->getEntry(row);

        // 상세 페이지 열기
        DetailPageWidget* detailPage = new DetailPageWidget(entry);
        //this->hide();
        detailPage->show();

        // 업데이트 후 반영
        connect(detailPage, &DetailPageWidget::entryUpdated, this, [=](const AddressEntry &updatedEntry) {
            model->updateEntry(row, updatedEntry);
        });

        // 닫기 시 원래 페이지 복귀
        connect(detailPage, &DetailPageWidget::detailPageClosed, this, [=]() {
            this->show();
        });
    });
}

MainPageWidget::~MainPageWidget()
{
    delete ui;
}
