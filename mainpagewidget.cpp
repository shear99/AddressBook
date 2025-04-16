#include "mainpagewidget.h"
#include "ui_mainpagewidget.h"
#include "src/core/addressbookmodel.h"
#include "detailpagewidget.h"
#include "heartdelegate.h"
#include "src/util/fontupdate.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>

MainPageWidget::MainPageWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainPageWidget)
{
    ui->setupUi(this);

    ui->addButton->setToolTip("Add new");
    ui->deleteButton->setToolTip("Delete");

    // Initialize model and proxy model
    model = new AddressBookModel(this);
    proxyModel = new MultiColumnFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    
    // Enable sorting
    proxyModel->setSortRole(Qt::DisplayRole);
    proxyModel->setDynamicSortFilter(true);
    ui->addressTableView->setModel(proxyModel);
    
    // Enable sorting in table view
    ui->addressTableView->setSortingEnabled(true);
    
    // Default sort by name (ascending)
    ui->addressTableView->sortByColumn(1, Qt::AscendingOrder);

    // Connect search field
    connect(ui->searchText, &QLineEdit::textChanged, this, [=](const QString &text) {
        proxyModel->setFilterRegularExpression(QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));
    });

    // Apply custom fonts
    FontUpdate::applyFontToAllChildren(this, ":/fonts/fonts/GmarketSansTTFMedium.ttf");

    // Apply heart delegate to favorite column
    HeartDelegate* heartDelegate = new HeartDelegate(this);
    ui->addressTableView->setItemDelegateForColumn(0, heartDelegate);

    // Reduce width of favorite column
    ui->addressTableView->setColumnWidth(0, 24);

    // Allow editing with single click (for checkbox toggle)
    ui->addressTableView->setEditTriggers(QAbstractItemView::SelectedClicked);

    // Configure table view
    ui->addressTableView->horizontalHeader()->setStretchLastSection(true);
    ui->addressTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->addressTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->addressTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // Configure sortable columns
    QHeaderView* header = ui->addressTableView->horizontalHeader();
    header->setSectionsClickable(true);
    
    // Configure favorite column as fixed size
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    
    // Set sorting per column
    for (int i = 0; i < ui->addressTableView->model()->columnCount(); ++i) {
        if (i >= 1 && i <= 5) { 
            // Allow sorting for name, phone, email, company, position
            header->setSortIndicatorShown(true);
        } else {
            // Disable sorting for other columns
            if (i != 0) { // Favorite column already configured above
                header->setSectionResizeMode(i, QHeaderView::Interactive);
            }
        }
    }

    // Open detail page on double-click
    connect(ui->addressTableView, &QTableView::doubleClicked, this, [=](const QModelIndex &proxyIndex) {
        if (!proxyIndex.isValid() || proxyIndex.column() == 0)
            return;
        QModelIndex sourceIndex = proxyModel->mapToSource(proxyIndex);
        int row = sourceIndex.row();
        AddressEntry entry = model->getEntry(row);
        DetailPageWidget* detailPage = new DetailPageWidget(entry, nullptr, false, nullptr);
        
        // Disable main page instead of hiding
        this->setEnabled(false);
        detailPage->show();

        connect(detailPage, &DetailPageWidget::entryUpdated, this, [=](const AddressEntry &updatedEntry) {
            model->updateEntry(row, updatedEntry);
        });
        connect(detailPage, &DetailPageWidget::detailPageClosed, this, [=]() {
            // Re-enable main page
            this->setEnabled(true);
            detailPage->deleteLater();
        });
    });

    // Handle add button click
    connect(ui->addButton, &QPushButton::clicked, this, [=]() {
        AddressEntry newEntry;
        DetailPageWidget* detailPage = new DetailPageWidget(newEntry, nullptr, true, model);
        
        // Disable main page instead of hiding
        this->setEnabled(false);
        detailPage->show();
        
        connect(detailPage, &DetailPageWidget::entryUpdated, this, [=](const AddressEntry &updatedEntry) {
            model->addEntry(updatedEntry);
        });
        connect(detailPage, &DetailPageWidget::detailPageClosed, this, [=]() {
            // Re-enable main page
            this->setEnabled(true);
            detailPage->deleteLater();
        });
    });

    // Handle delete button click
    connect(ui->deleteButton, &QPushButton::clicked, this, [=]() {
        QItemSelectionModel* selectionModel = ui->addressTableView->selectionModel();
        QModelIndexList selectedRows = selectionModel->selectedRows();
        if (selectedRows.isEmpty()) {
            QMessageBox::information(this, tr("Delete"), tr("Select an item to delete."));
            return;
        }
        int proxyRow = selectedRows.first().row();
        QModelIndex sourceIndex = proxyModel->mapToSource(proxyModel->index(proxyRow, 0));
        int row = sourceIndex.row();
        QMessageBox::StandardButton reply = QMessageBox::question(this,
                                                                  tr("Confirm Delete"),
                                                                  tr("Are you sure you want to delete?"),
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
