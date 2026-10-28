#pragma once

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <string>
#include "domain/errors/DomainError.h"

namespace pixcode::infrastructure {

class SqliteTransaction {
public:
    explicit SqliteTransaction(QSqlDatabase db, bool immediate = true)
        : m_db(std::move(db)), m_committed(false) {
        if (!m_db.isOpen()) {
            throw domain::DomainError(domain::ErrorCode::DatabaseError, "Cannot begin transaction on closed database");
        }

        QSqlQuery query(m_db);
        const QString sql = immediate ? "BEGIN IMMEDIATE TRANSACTION;" : "BEGIN TRANSACTION;";
        if (!query.exec(sql)) {
            throw domain::DomainError(domain::ErrorCode::DatabaseError,
                                      "Failed to begin transaction: " + query.lastError().text().toStdString());
        }
    }

    ~SqliteTransaction() {
        if (!m_committed && m_db.isOpen()) {
            QSqlQuery query(m_db);
            query.exec("ROLLBACK;");
        }
    }

    SqliteTransaction(const SqliteTransaction&) = delete;
    SqliteTransaction& operator=(const SqliteTransaction&) = delete;

    SqliteTransaction(SqliteTransaction&& other) noexcept
        : m_db(std::move(other.m_db)), m_committed(other.m_committed) {
        other.m_committed = true; // Prevents rollback on moved-from
    }

    void commit() {
        if (m_committed) return;

        QSqlQuery query(m_db);
        if (!query.exec("COMMIT;")) {
            throw domain::DomainError(domain::ErrorCode::DatabaseError,
                                      "Failed to commit transaction: " + query.lastError().text().toStdString());
        }
        m_committed = true;
    }

    void rollback() {
        if (m_committed) return;

        QSqlQuery query(m_db);
        query.exec("ROLLBACK;");
        m_committed = true;
    }

private:
    QSqlDatabase m_db;
    bool m_committed;
};

} // namespace pixcode::infrastructure

