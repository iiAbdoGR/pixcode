#include "MigrationRunner.h"
#include "MigrationSql.h"
#include "database/SqliteTransaction.h"
#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

namespace pixcode::infrastructure {

MigrationRunner::MigrationRunner(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

std::string MigrationRunner::computeSha256(const std::string& content) {
    const QByteArray data(content.data(), static_cast<qsizetype>(content.size()));
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex().toStdString();
}

int MigrationRunner::getCurrentVersion() {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    if (!query.exec("SELECT COALESCE(MAX(version), 0) FROM schema_migrations;")) {
        return 0;
    }
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

std::vector<MigrationDefinition> MigrationRunner::getBuiltinMigrations() {
    std::vector<MigrationDefinition> migrations;

    migrations.push_back(MigrationDefinition{
        .version = 1,
        .name = "V001__initial_schema",
        .sql = std::string(V001_InitialSchemaSql)
    });

    migrations.push_back(MigrationDefinition{
        .version = 2,
        .name = "V002__academic_schema",
        .sql = std::string(V002_AcademicSchemaSql)
    });

    migrations.push_back(MigrationDefinition{
        .version = 3,
        .name = "V003__attendance_and_payroll_schema",
        .sql = std::string(V003_AttendanceAndPayrollSql)
    });

    migrations.push_back(MigrationDefinition{
        .version = 4,
        .name = "V004__business_integrity_and_role_consolidation",
        .sql = std::string(V004_BusinessIntegrityAndRoleConsolidationSql)
    });

    migrations.push_back(MigrationDefinition{
        .version = 5,
        .name = "V005__instructor_contracts_and_admin_approval",
        .sql = std::string(V005_InstructorContractsAndAdminApprovalSql)
    });

    migrations.push_back(MigrationDefinition{
        .version = 6,
        .name = "V006__payroll_allocations_and_role_security",
        .sql = std::string(V006_PayrollAllocationsAndRoleSecuritySql)
    });

    return migrations;
}

bool MigrationRunner::runMigrations(QString& outError) {
    auto db = m_dbManager->getConnection();

    // 1. Ensure migrations ledger exists
    {
        QSqlQuery initQuery(db);
        const QString createLedgerSql =
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            "  version INTEGER PRIMARY KEY,"
            "  name TEXT NOT NULL,"
            "  applied_at_utc TEXT NOT NULL,"
            "  checksum TEXT NOT NULL"
            ");";
        if (!initQuery.exec(createLedgerSql)) {
            outError = "Failed to create schema_migrations table: " + initQuery.lastError().text();
            return false;
        }
    }

    const auto migrations = getBuiltinMigrations();

    for (const auto& migration : migrations) {
        const std::string calculatedChecksum = computeSha256(migration.sql);

        // Check if already applied
        {
            QSqlQuery checkQuery(db);
            checkQuery.prepare("SELECT checksum FROM schema_migrations WHERE version = :version;");
            checkQuery.bindValue(":version", migration.version);

            if (!checkQuery.exec()) {
                outError = "Failed to query schema_migrations: " + checkQuery.lastError().text();
                return false;
            }

            if (checkQuery.next()) {
                const std::string recordedChecksum = checkQuery.value(0).toString().toStdString();
                if (recordedChecksum != calculatedChecksum) {
                    outError = QString("Schema tampering detected in migration version %1! Checksum mismatch.")
                        .arg(migration.version);
                    return false;
                }
                // Already applied cleanly
                continue;
            }
            checkQuery.finish();
        }

        // Perform online consistent backup prior to applying migration if on disk
        if (!m_dbManager->databasePath().isEmpty() && m_dbManager->databasePath() != ":memory:") {
            QString backupErr;
            const QString backupPath = m_dbManager->databasePath() + QString(".pre_v%1_%2.bak")
                .arg(migration.version)
                .arg(QDateTime::currentSecsSinceEpoch());
            if (!m_dbManager->createOnlineBackup(backupPath, backupErr)) {
                outError = QString("Pre-migration backup before applying version %1 failed: %2")
                    .arg(migration.version).arg(backupErr);
                return false;
            }
        }

        // Apply migration atomically
        try {
            // Turn off foreign key enforcement outside the transaction so table rebuilds/drops succeed
            {
                QSqlQuery fkOff(db);
                fkOff.exec("PRAGMA foreign_keys = OFF;");
            }

            struct FkGuard {
                QSqlDatabase d;
                ~FkGuard() {
                    QSqlQuery fkOn(d);
                    fkOn.exec("PRAGMA foreign_keys = ON;");
                }
            } fkGuard{db};

            SqliteTransaction tx(db, true);

            {
                QSqlQuery execQuery(db);
                const QString qsql = QString::fromStdString(migration.sql);

                const auto rawStatements = qsql.split(';', Qt::SkipEmptyParts);
                for (const auto& rawStmt : rawStatements) {
                    QString cleanStmt;
                    for (const auto& line : rawStmt.split('\n')) {
                        QString trimmedLine = line.trimmed();
                        if (!trimmedLine.startsWith("--") && !trimmedLine.isEmpty()) {
                            cleanStmt += trimmedLine + "\n";
                        }
                    }
                    cleanStmt = cleanStmt.trimmed();
                    if (cleanStmt.isEmpty()) continue;

                    if (!execQuery.exec(cleanStmt + ";")) {
                        outError = QString("Migration %1 failed on statement [%2]: %3")
                            .arg(migration.version)
                            .arg(cleanStmt.left(50))
                            .arg(execQuery.lastError().text());
                        tx.rollback();
                        return false;
                    }
                }
                execQuery.finish();
            }

            {
                // Record migration in ledger
                QSqlQuery recordQuery(db);
                recordQuery.prepare(
                    "INSERT INTO schema_migrations (version, name, applied_at_utc, checksum) "
                    "VALUES (:ver, :name, :applied, :checksum);"
                );
                recordQuery.bindValue(":ver", migration.version);
                recordQuery.bindValue(":name", QString::fromStdString(migration.name));
                recordQuery.bindValue(":applied", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                recordQuery.bindValue(":checksum", QString::fromStdString(calculatedChecksum));

                if (!recordQuery.exec()) {
                    outError = "Failed to record migration: " + recordQuery.lastError().text();
                    tx.rollback();
                    return false;
                }
                recordQuery.finish();
            }

            tx.commit();

            // Validate that all foreign key constraints remain intact after migration
            {
                QSqlQuery fkCheck(db);
                if (fkCheck.exec("PRAGMA foreign_key_check;") && fkCheck.next()) {
                    outError = QString("Foreign key constraint violation after migration %1 in table %2")
                        .arg(migration.version)
                        .arg(fkCheck.value(0).toString());
                    return false;
                }
            }
        } catch (const std::exception& e) {
            outError = QString("Exception during migration %1: %2").arg(migration.version).arg(e.what());
            return false;
        }
    }

    return true;
}

} // namespace pixcode::infrastructure
