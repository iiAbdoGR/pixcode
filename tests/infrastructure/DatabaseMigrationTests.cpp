#include "DatabaseMigrationTests.h"
#include <iostream>
#include <QtCore/QTemporaryDir>
#include <QtCore/QDateTime>
#include <QtCore/QCryptographicHash>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "infrastructure/database/DatabaseManager.h"
#include "infrastructure/migrations/MigrationRunner.h"
#include "infrastructure/migrations/MigrationSql.h"

void DatabaseMigrationTests::testPragmasAndMigrations() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString dbPath = tempDir.filePath("test_pixcode.db");
    auto dbManager = std::make_shared<pixcode::infrastructure::DatabaseManager>(dbPath);

    auto db = dbManager->getConnection();
    {
        QSqlQuery pragmaQuery(db);
        QVERIFY(pragmaQuery.exec("PRAGMA foreign_keys;"));
        QVERIFY(pragmaQuery.next());
        QCOMPARE(pragmaQuery.value(0).toInt(), 1);

        QVERIFY(pragmaQuery.exec("PRAGMA journal_mode;"));
        QVERIFY(pragmaQuery.next());
        QCOMPARE(pragmaQuery.value(0).toString().toLower(), "wal");
        pragmaQuery.finish();
    }

    pixcode::infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY2(runner.runMigrations(error), error.toStdString().c_str());
    QCOMPARE(runner.getCurrentVersion(), 6);

    QSqlQuery tableQuery(db);
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM roles;"));
    QVERIFY(tableQuery.next());
    // Consolidated to strictly 2 system roles (Owner and Admin)
    QCOMPARE(tableQuery.value(0).toInt(), 2);

    // Verify V002 academic tables exist
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM students;"));
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM courses;"));
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM groups;"));
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM enrollments;"));
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM payments;"));

    // Verify V003 attendance and payroll tables exist
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM sessions;"));
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM session_attendance;"));
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM instructor_rate_history;"));
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM instructor_payouts;"));

    // Verify V004 partial unique index exists
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM sqlite_master WHERE type='index' AND name='idx_unique_active_enrollment';"));
    QVERIFY(tableQuery.next());
    QCOMPARE(tableQuery.value(0).toInt(), 1);

    // Verify V004 instructor columns exist
    QVERIFY(tableQuery.exec("SELECT compensation_type, base_rate_piastres FROM instructors LIMIT 1;"));

    // Verify V005 contract and salary columns exist
    QVERIFY(tableQuery.exec("SELECT contract_start_date, contract_end_date, monthly_salary_piastres FROM instructors LIMIT 1;"));

    // Verify V006 salary periods and allocations tables exist
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM instructor_salary_periods;"));
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM instructor_payout_allocations;"));

    // Verify Admin has restricted instructor payment execution but NO approval
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM role_permissions WHERE role_id = 'role_admin' AND permission_id = 'p_pay_inst_pay';"));
    QVERIFY(tableQuery.next());
    QCOMPARE(tableQuery.value(0).toInt(), 1);

    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM role_permissions WHERE role_id = 'role_admin' AND permission_id = 'p_pay_calc_appr';"));
    QVERIFY(tableQuery.next());
    QCOMPARE(tableQuery.value(0).toInt(), 0);

    // Idempotency: second run succeeds
    QVERIFY2(runner.runMigrations(error), error.toStdString().c_str());
    QCOMPARE(runner.getCurrentVersion(), 6);
}

void DatabaseMigrationTests::testMigrationV004WithExistingDataAndForeignKeys() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString dbPath = tempDir.filePath("test_pixcode_existing_data.db");
    auto dbManager = std::make_shared<pixcode::infrastructure::DatabaseManager>(dbPath);
    auto db = dbManager->getConnection();

    // 1. Manually apply migrations 1, 2, and 3
    QSqlQuery q(db);
    QVERIFY(q.exec(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "  version INTEGER PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  applied_at_utc TEXT NOT NULL,"
        "  checksum TEXT NOT NULL"
        ");"
    ));

    auto runSqlScript = [&](const std::string_view& sqlView) {
        QString qsql = QString::fromUtf8(sqlView.data(), static_cast<qsizetype>(sqlView.size()));
        const auto rawStatements = qsql.split(';', Qt::SkipEmptyParts);
        for (const auto& rawStmt : rawStatements) {
            QString cleanStmt;
            for (const auto& line : rawStmt.split('\n')) {
                QString trimmed = line.trimmed();
                if (!trimmed.startsWith("--") && !trimmed.isEmpty()) {
                    cleanStmt += trimmed + "\n";
                }
            }
            cleanStmt = cleanStmt.trimmed();
            if (cleanStmt.isEmpty()) continue;
            QVERIFY2(q.exec(cleanStmt + ";"), q.lastError().text().toStdString().c_str());
        }
    };

    auto sha256 = [](const std::string_view& sv) {
        return QCryptographicHash::hash(QByteArray(sv.data(), static_cast<qsizetype>(sv.size())), QCryptographicHash::Sha256).toHex().toStdString();
    };

    // Apply V1
    runSqlScript(pixcode::infrastructure::V001_InitialSchemaSql);
    q.prepare("INSERT INTO schema_migrations VALUES (1, 'V001__initial_schema', '2026-01-01T00:00:00Z', :chk);");
    q.bindValue(":chk", QString::fromStdString(sha256(pixcode::infrastructure::V001_InitialSchemaSql)));
    QVERIFY(q.exec());

    // Apply V2
    runSqlScript(pixcode::infrastructure::V002_AcademicSchemaSql);
    q.prepare("INSERT INTO schema_migrations VALUES (2, 'V002__academic_schema', '2026-01-01T00:00:00Z', :chk);");
    q.bindValue(":chk", QString::fromStdString(sha256(pixcode::infrastructure::V002_AcademicSchemaSql)));
    QVERIFY(q.exec());

    // Apply V3
    runSqlScript(pixcode::infrastructure::V003_AttendanceAndPayrollSql);
    q.prepare("INSERT INTO schema_migrations VALUES (3, 'V003__attendance_and_payroll_schema', '2026-01-01T00:00:00Z', :chk);");
    q.bindValue(":chk", QString::fromStdString(sha256(pixcode::infrastructure::V003_AttendanceAndPayrollSql)));
    QVERIFY(q.exec());

    // 2. Insert existing operational data with real foreign keys between payments and enrollments
    QVERIFY(q.exec("INSERT INTO students (id, student_code, full_name, registered_at_utc, updated_at_utc) VALUES (1, 'ST-001', 'Test Student', '2026-01-01', '2026-01-01');"));
    QVERIFY(q.exec("INSERT INTO courses (id, name, created_at_utc, updated_at_utc) VALUES (1, 'Course 1', '2026-01-01', '2026-01-01');"));
    QVERIFY(q.exec("INSERT INTO groups (id, course_id, name, capacity, fee_piastres, status, created_at_utc, updated_at_utc) VALUES (1, 1, 'Group 1', 10, 100000, 'open', '2026-01-01', '2026-01-01');"));
    QVERIFY(q.exec("INSERT INTO enrollments (id, student_id, group_id, agreed_fee_piastres, discount_piastres, net_fee_piastres, status, enrolled_at_utc) VALUES (1, 1, 1, 100000, 0, 100000, 'active', '2026-01-01');"));
    
    // Child record in payments referencing enrollments(id) = 1!
    QVERIFY(q.exec("INSERT INTO payments (id, enrollment_id, amount_piastres, payment_method, paid_at_utc, receipt_number, created_at_utc) VALUES (1, 1, 50000, 'cash', '2026-01-02', 'REC-001', '2026-01-02');"));

    // Verify foreign keys are currently ON
    QVERIFY(q.exec("PRAGMA foreign_keys;"));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);

    q.finish();

    // 3. Now execute MigrationRunner to apply Migration 4 over this existing data
    pixcode::infrastructure::MigrationRunner runner(dbManager);
    QString error;
    bool ok = runner.runMigrations(error);
    if (!ok) {
        std::cout << "\n>>> MIGRATION ERROR: " << error.toStdString() << "\n" << std::endl;
    }
    QVERIFY2(ok, error.toStdString().c_str());
    QCOMPARE(runner.getCurrentVersion(), 6);

    // 4. Verify that data and foreign keys remain 100% intact
    QVERIFY(q.exec("SELECT COUNT(*) FROM payments WHERE enrollment_id = 1;"));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);

    QVERIFY(q.exec("SELECT id, cancellation_reason, restored_at_utc FROM enrollments WHERE id = 1;"));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toInt(), 1);

    // Verify foreign key integrity with PRAGMA foreign_key_check
    QVERIFY(q.exec("PRAGMA foreign_key_check;"));
    QVERIFY(!q.next()); // Zero violations!
}
