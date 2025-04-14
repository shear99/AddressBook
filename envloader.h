#ifndef ENVLOADER_H
#define ENVLOADER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QProcessEnvironment>

inline void loadEnvFile(const QString& filePath = ".env") {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open .env file.";
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        QStringList parts = line.split('=', Qt::SkipEmptyParts);
        if (parts.size() != 2)
            continue;

        QString key = parts[0].trimmed();
        QString value = parts[1].trimmed();

#ifdef _WIN32
        _putenv_s(key.toStdString().c_str(), value.toStdString().c_str());
#else
        setenv(key.toStdString().c_str(), value.toStdString().c_str(), 1);
#endif
    }

    file.close();
}

#endif // ENVLOADER_H
