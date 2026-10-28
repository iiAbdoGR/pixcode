#pragma once

#include <QtCore/QLoggingCategory>
#include <QString>

Q_DECLARE_LOGGING_CATEGORY(logAuth)
Q_DECLARE_LOGGING_CATEGORY(logDb)
Q_DECLARE_LOGGING_CATEGORY(logUi)

namespace pixcode::infrastructure {

class AppLogger {
public:
    static void init(const QString& logDirectoryPath);
    static void shutdown();
};

} // namespace pixcode::infrastructure

