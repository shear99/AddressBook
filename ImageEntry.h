#ifndef IMAGEENTRY_H
#define IMAGEENTRY_H

#include <QLabel>

class ImageEntry : public QLabel
{
    QString m_image;
    QString m_original_key;
    QString m_processed_key;

public:
    QString getImage() const { return m_image; }
    QString getOriginal_key() const { return m_original_key; }
    QString getProcessed_key() const { return m_processed_key; }

    // Setters
    void setImage(const QString& image) { m_image = image; }
    void setOriginal_key(const QString& original_key) { m_original_key = original_key; }
    void setProcessed_key(const QString& processed_key) { m_processed_key = processed_key; }
};


#endif // IMAGEENTRY_H
