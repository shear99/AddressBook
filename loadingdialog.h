// LoadingDialog.h
#ifndef LOADINGDIALOG_H
#define LOADINGDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>

class LoadingDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoadingDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        setModal(true);
        setFixedSize(200, 100);

        QLabel* label = new QLabel("통신 중...", this);
        label->setAlignment(Qt::AlignCenter);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(label);
        setLayout(layout);
    }
};

#endif // LOADINGDIALOG_H
