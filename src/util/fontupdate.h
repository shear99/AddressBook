#ifndef FONTUPDATE_H
#define FONTUPDATE_H

#include <QWidget>
#include <QString>

// Utility class for managing font updates across the application
// Handles font loading and application to widgets
class FontUpdate {
public:
    // Applies a custom font to a single widget while preserving its size
    static void applyFontToWidget(QWidget* widget, const QString& fontResourcePath);

    // Applies a custom font to a parent widget and all its children while preserving their sizes
    static void applyFontToAllChildren(QWidget* parent, const QString& fontResourcePath);
};

#endif // FONTUPDATE_H
