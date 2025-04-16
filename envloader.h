#ifndef ENVLOADER_H
#define ENVLOADER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QProcessEnvironment>

// Loads environment variables from .env file
// Default path: /build/Desktop_Qt_6_8_3_MSVC2022_64bit-Debug/.env
// This file contains AWS credentials and other configuration settings
inline void loadEnvFile(const QString& filePath = ".env") {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open .env file.";
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        // Parse key-value pairs from .env file
        QStringList parts = line.split('=', Qt::SkipEmptyParts);
        if (parts.size() != 2)
            continue;

        QString key = parts[0].trimmed();
        QString value = parts[1].trimmed();

        // Set environment variables based on OS
#ifdef _WIN32
        _putenv_s(key.toStdString().c_str(), value.toStdString().c_str());
#else
        setenv(key.toStdString().c_str(), value.toStdString().c_str(), 1);
#endif
    }

    file.close();
}

#endif // ENVLOADER_H
