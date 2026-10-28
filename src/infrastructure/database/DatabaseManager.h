#pragma once

#include <QtSql/QSqlDatabase>
#include <QString>
#include <memory>
#include <mutex>
#include <string>

namespace pixcode::infrastructure {

class DatabaseManager {
public:
    explicit DatabaseManager(QString databaseFilePath);
    ~DatabaseManager();

    [[nodiscard]] const QString& databasePath() const noexcept { return m_databasePath; }

    // Obtains a thread-local QSqlDatabase connection configured with WAL and Foreign Keys
    [[nodiscard]] QSqlDatabase getConnection();

    // Verifies integrity using PRAGMA quick_check;
    [[nodiscard]] bool checkIntegrity(QString& outDetails);

    // Creates an online consistent backup using VACUUM INTO
    [[nodiscard]] bool createOnlineBackup(const QString& destinationPath, QString& outError);

private:
    void applyPragmas(QSqlDatabase& db);

    QString m_databasePath;
    mutable std::mutex m_mutex;
};

} // namespace pixcode::infrastructure

