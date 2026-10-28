#include "DatabaseManager.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QThread>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <stdexcept>

namespace pixcode::infrastructure {

DatabaseManager::DatabaseManager(QString databaseFilePath)
    : m_databasePath(std::move(databaseFilePath)) {
    // Ensure parent directory exists
    const QFileInfo fi(m_databasePath);
    if (!fi.dir().exists()) {
        fi.dir().mkpath(".");
    }

    // Initialize the primary connection to ensure pragmas and file creation
    auto db = getConnection();
    if (!db.isOpen()) {
        throw std::runtime_error("Could not open primary SQLite database: " +
                                 db.lastError().text().toStdString());
    }
}

DatabaseManager::~DatabaseManager() {
    // Cleanup any lingering connection for the calling thread
    const QString connName = QString("pixcode_conn_%1")
        .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase::removeDatabase(connName);
    }
}

QSqlDatabase DatabaseManager::getConnection() {
    std::lock_guard<std::mutex> lock(m_mutex);

    const QString connName = QString("pixcode_conn_%1")
        .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));

    if (QSqlDatabase::contains(connName)) {
        auto db = QSqlDatabase::database(connName);
        if (db.isOpen()) {
            return db;
        }
    }

    auto db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(m_databasePath);

    if (!db.open()) {
        throw std::runtime_error("Failed to open SQLite database: " +
                                 db.lastError().text().toStdString());
    }

    applyPragmas(db);
    return db;
}

void DatabaseManager::applyPragmas(QSqlDatabase& db) {
    QSqlQuery query(db);

    // 1. Mandatory foreign key constraint enforcement
    query.exec("PRAGMA foreign_keys = ON;");

    // 2. Write-Ahead Logging for high concurrency and read non-blocking
    query.exec("PRAGMA journal_mode = WAL;");

    // 3. Normal synchronous for ACID safety with WAL performance
    query.exec("PRAGMA synchronous = NORMAL;");

    // 4. 5000 ms busy timeout to queue lock requests
    query.exec("PRAGMA busy_timeout = 5000;");

    // 5. In-memory temporary tables
    query.exec("PRAGMA temp_store = MEMORY;");
}

bool DatabaseManager::checkIntegrity(QString& outDetails) {
    auto db = getConnection();
    QSqlQuery query(db);
    if (!query.exec("PRAGMA quick_check;")) {
        outDetails = query.lastError().text();
        return false;
    }

    if (query.next()) {
        outDetails = query.value(0).toString();
        return outDetails.toLower() == "ok";
    }

    outDetails = "No result from PRAGMA quick_check";
    return false;
}

bool DatabaseManager::createOnlineBackup(const QString& destinationPath, QString& outError) {
    auto db = getConnection();
    QSqlQuery query(db);

    // Escape single quotes in path
    QString sanitizedPath = destinationPath;
    sanitizedPath.replace("'", "''");

    const QString sql = QString("VACUUM INTO '%1';").arg(sanitizedPath);
    if (!query.exec(sql)) {
        outError = query.lastError().text();
        return false;
    }

    return true;
}

} // namespace pixcode::infrastructure

