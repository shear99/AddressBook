#ifndef MAINPAGEWIDGET_H
#define MAINPAGEWIDGET_H

#include <QWidget>
#include "src/core/addressbookmodel.h"
#include "src/core/multicolumnfilterproxymodel.h"
#include <Qt>

namespace Ui {
class MainPageWidget;
}

class MainPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainPageWidget(QWidget *parent = nullptr);
    ~MainPageWidget();

private:
    Ui::MainPageWidget *ui;
    AddressBookModel* model = nullptr;
    MultiColumnFilterProxyModel* proxyModel;
};

#endif // MAINPAGEWIDGET_H
