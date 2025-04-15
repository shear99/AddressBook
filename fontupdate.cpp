#include "fontupdate.h"
#include <QFontDatabase>
#include <QFont>

void FontUpdate::applyFontToWidget(QWidget* widget, const QString& fontResourcePath) {
    if (!widget) return;

    int fontId = QFontDatabase::addApplicationFont(fontResourcePath);
    if (fontId == -1) return; // 폰트 로드 실패

    QString family = QFontDatabase::applicationFontFamilies(fontId).value(0);
    if (family.isEmpty()) return;

    QFont font = widget->font();
    font.setFamily(family);
    widget->setFont(font);
}

void FontUpdate::applyFontToAllChildren(QWidget* parent, const QString& fontResourcePath) {
    if (!parent) return;

    int fontId = QFontDatabase::addApplicationFont(fontResourcePath);
    if (fontId == -1) return;

    QString family = QFontDatabase::applicationFontFamilies(fontId).value(0);
    if (family.isEmpty()) return;

    QFont font = parent->font();
    font.setFamily(family);
    parent->setFont(font);

    // 모든 자식 위젯에 폰트 적용
    QList<QWidget*> children = parent->findChildren<QWidget*>();
    for (QWidget* child : children) {
        QFont childFont = child->font();
        childFont.setFamily(family);
        child->setFont(childFont);
    }
}
