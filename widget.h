#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "AddressBookModel.h"
#include "./ui_widget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;
    AddressBookModel* model = nullptr;
};

#endif // WIDGET_H
