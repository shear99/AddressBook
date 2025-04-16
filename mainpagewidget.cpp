#include "mainpagewidget.h"
#include "ui_mainpagewidget.h"
#include "addressbookmodel.h"
#include "detailpagewidget.h"
#include "heartdelegate.h"
#include "fontupdate.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>

MainPageWidget::MainPageWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainPageWidget)
{
    ui->setupUi(this);

    // 1. 모델 및 프록시 모델 생성
    model = new AddressBookModel(this);
    proxyModel = new MultiColumnFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    
    // 정렬 가능하도록 설정
    proxyModel->setSortRole(Qt::DisplayRole);
    proxyModel->setDynamicSortFilter(true);

    // 2. 테이블뷰에 프록시 모델 연결
    ui->addressTableView->setModel(proxyModel);
    
    // 정렬 기능 활성화
    ui->addressTableView->setSortingEnabled(true);
    
    // 초기 정렬 설정 (이름 기준 오름차순)
    ui->addressTableView->sortByColumn(1, Qt::AscendingOrder); // 1은 name 컬럼

    // 3. 검색창 연결
    connect(ui->searchText, &QLineEdit::textChanged, this, [=](const QString &text) {
        proxyModel->setFilterRegularExpression(QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));
    });

    // 4. 폰트 적용
    FontUpdate::applyFontToAllChildren(this, ":/fonts/fonts/GmarketSansTTFMedium.ttf");

    // 5. 첫 번째 열(즐겨찾기)에는 커스텀 하트 delegate 적용
    HeartDelegate* heartDelegate = new HeartDelegate(this);
    ui->addressTableView->setItemDelegateForColumn(0, heartDelegate);

    // 6. 첫 번째 열 너비를 줄임 (예: 24 픽셀)
    ui->addressTableView->setColumnWidth(0, 24);

    // 7. 단일 클릭 시 편집(체크박스 토글) 허용
    ui->addressTableView->setEditTriggers(QAbstractItemView::SelectedClicked);

    // 8. 테이블뷰 기본 설정
    ui->addressTableView->horizontalHeader()->setStretchLastSection(true);
    ui->addressTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->addressTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->addressTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // 정렬 가능한 컬럼 설정 (name, phone, email, company, position만 정렬 가능)
    QHeaderView* header = ui->addressTableView->horizontalHeader();
    header->setSectionsClickable(true);
    
    // 첫 번째 열(즐겨찾기) 정렬 비활성화
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    
    // 각 컬럼의 정렬 설정
    for (int i = 0; i < ui->addressTableView->model()->columnCount(); ++i) {
        if (i >= 1 && i <= 5) { 
            // 1:name, 2:phone, 3:email, 4:company, 5:position은 정렬 가능
            header->setSortIndicatorShown(true);
        } else {
            // 다른 컬럼은 정렬 불가
            if (i != 0) { // 즐겨찾기 열은 위에서 이미 설정함
                header->setSectionResizeMode(i, QHeaderView::Interactive);
            }
        }
    }

    // 9. 더블클릭 시 상세 편집 페이지로 이동
    connect(ui->addressTableView, &QTableView::doubleClicked, this, [=](const QModelIndex &proxyIndex) {
        if (!proxyIndex.isValid() || proxyIndex.column() == 0)
            return;
        QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
        int row = sourceIndex.row();
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

    // 10. addButton 클릭 시 빈 DetailPageWidget을 열어서 새 항목 추가
    connect(ui->addButton, &QPushButton::clicked, this, [=]() {
        AddressEntry newEntry;
        DetailPageWidget* detailPage = new DetailPageWidget(newEntry, nullptr, true);
        this->hide();
        detailPage->show();
        connect(detailPage, &DetailPageWidget::entryUpdated, this, [=](const AddressEntry &updatedEntry) {
            model->addEntry(updatedEntry);
        });
        connect(detailPage, &DetailPageWidget::detailPageClosed, this, [=]() {
            this->show();
        });
    });

    // 11. deleteButton 클릭 시, 현재 선택된 항목 삭제 (삭제 확인 다이얼로그 포함)
    connect(ui->deleteButton, &QPushButton::clicked, this, [=]() {
        QItemSelectionModel* selectionModel = ui->addressTableView->selectionModel();
        QModelIndexList selectedRows = selectionModel->selectedRows();
        if (selectedRows.isEmpty()) {
            QMessageBox::information(this, tr("삭제"), tr("삭제할 항목을 선택하세요."));
            return;
        }
        int proxyRow = selectedRows.first().row();
        QModelIndex sourceIndex = proxyModel->mapToSource(proxyModel->index(proxyRow, 0));
        int row = sourceIndex.row();
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
