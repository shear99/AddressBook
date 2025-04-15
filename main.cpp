#include "widget.h"
#include "envloader.h"


#include <QApplication>

int main(int argc, char *argv[])
{
    loadEnvFile();
    qDebug() << "AWS_LAMBDA_LOAD_URL:" << qgetenv("AWS_LAMBDA_LOAD_URL");
    qDebug() << "AWS_LAMBDA_SAVE_URL:" << qgetenv("AWS_LAMBDA_SAVE_URL");

    QApplication a(argc, argv);
    Widget w;
    w.show();

    return a.exec();
}
