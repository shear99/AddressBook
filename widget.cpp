#include "widget.h"
#include "detailpagewidget.h"
#include "addressbookmodel.h"
#include "mainpagewidget.h"
#include "./ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    MainPageWidget* mainPage = new MainPageWidget(this);
    mainPage->show();

    this->hide(); // Widget은 숨겨두기 (진입용)
}

Widget::~Widget()
{
    delete ui;
}
