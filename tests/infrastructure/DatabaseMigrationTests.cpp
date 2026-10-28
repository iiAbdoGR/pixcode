#include "DatabaseMigrationTests.h"
#include <iostream>
#include <QtCore/QTemporaryDir>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "infrastructure/database/DatabaseManager.h"
#include "infrastructure/migrations/MigrationRunner.h"

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
    QCOMPARE(runner.getCurrentVersion(), 3);

    QSqlQuery tableQuery(db);
    QVERIFY(tableQuery.exec("SELECT COUNT(*) FROM roles;"));
    QVERIFY(tableQuery.next());
    QVERIFY(tableQuery.value(0).toInt() >= 5);

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

    // Idempotency: second run succeeds
    QVERIFY2(runner.runMigrations(error), error.toStdString().c_str());
    QCOMPARE(runner.getCurrentVersion(), 3);
}
