#ifndef FONTUPDATE_H
#define FONTUPDATE_H

#include <QWidget>
#include <QString>

class FontUpdate {
public:
    // 단일 위젯에 폰트 적용 (사이즈는 기존 위젯 폰트 크기 유지)
    static void applyFontToWidget(QWidget* widget, const QString& fontResourcePath);

    // 부모 및 모든 자식 위젯에 폰트 일괄 적용 (사이즈는 기존 크기 유지)
    static void applyFontToAllChildren(QWidget* parent, const QString& fontResourcePath);
};

#endif // FONTUPDATE_H
