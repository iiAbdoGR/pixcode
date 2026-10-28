#include "AppLogger.h"
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QMutex>
#include <iostream>

Q_LOGGING_CATEGORY(logAuth, "pixcode.auth")
Q_LOGGING_CATEGORY(logDb, "pixcode.db")
Q_LOGGING_CATEGORY(logUi, "pixcode.ui")

namespace pixcode::infrastructure {

static QFile* s_logFile = nullptr;
static QMutex s_logMutex;

static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QMutexLocker locker(&s_logMutex);

    const QString timeStr = QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ss.zzzZ");
    QString level;
    switch (type) {
        case QtDebugMsg:    level = "DEBUG"; break;
        case QtInfoMsg:     level = "INFO "; break;
        case QtWarningMsg:  level = "WARN "; break;
        case QtCriticalMsg: level = "CRIT "; break;
        case QtFatalMsg:    level = "FATAL"; break;
    }

    const QString category = context.category ? context.category : "default";
    const QString formatted = QString("[%1] [%2] [%3] %4\n")
        .arg(timeStr, level, category, msg);

    // Stdout
    std::cout << formatted.toStdString();

    // File output
    if (s_logFile && s_logFile->isOpen()) {
        s_logFile->write(formatted.toUtf8());
        s_logFile->flush();
    }
}

void AppLogger::init(const QString& logDirectoryPath) {
    QDir dir(logDirectoryPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    const QString logPath = dir.filePath(QString("pixcode_%1.log")
        .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd")));

    s_logFile = new QFile(logPath);
    if (s_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qInstallMessageHandler(messageHandler);
    }
}

void AppLogger::shutdown() {
    qInstallMessageHandler(nullptr);
    if (s_logFile) {
        if (s_logFile->isOpen()) {
            s_logFile->close();
        }
        delete s_logFile;
        s_logFile = nullptr;
    }
}

} // namespace pixcode::infrastructure

