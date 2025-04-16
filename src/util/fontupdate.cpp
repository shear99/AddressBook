#include "fontupdate.h"
#include <QFontDatabase>
#include <QFont>

// Applies a custom font to a single widget
void FontUpdate::applyFontToWidget(QWidget* widget, const QString& fontResourcePath) {
    if (!widget) return;

    // Load the font file and get its ID
    int fontId = QFontDatabase::addApplicationFont(fontResourcePath);
    if (fontId == -1) return; // Font loading failed

    // Get the font family name
    QString family = QFontDatabase::applicationFontFamilies(fontId).value(0);
    if (family.isEmpty()) return;

    // Apply the font while preserving the widget's current font size
    QFont font = widget->font();
    font.setFamily(family);
    widget->setFont(font);
}

// Applies a custom font to a parent widget and all its children
void FontUpdate::applyFontToAllChildren(QWidget* parent, const QString& fontResourcePath) {
    if (!parent) return;

    // Load the font file and get its ID
    int fontId = QFontDatabase::addApplicationFont(fontResourcePath);
    if (fontId == -1) return;

    // Get the font family name
    QString family = QFontDatabase::applicationFontFamilies(fontId).value(0);
    if (family.isEmpty()) return;

    // Apply the font to the parent widget
    QFont font = parent->font();
    font.setFamily(family);
    parent->setFont(font);

    // Apply the font to all child widgets
    QList<QWidget*> children = parent->findChildren<QWidget*>();
    for (QWidget* child : children) {
        QFont childFont = child->font();
        childFont.setFamily(family);
        child->setFont(childFont);
    }
}
