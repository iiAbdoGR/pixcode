#include "AuditLogTests.h"
#include <QtCore/QTemporaryDir>
#include "infrastructure/database/DatabaseManager.h"
#include "infrastructure/migrations/MigrationRunner.h"
#include "infrastructure/repositories/SqliteAuditLogRepository.h"

void AuditLogTests::testAuditLogAppendAndQuery() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString dbPath = tempDir.filePath("test_audit.db");
    auto dbManager = std::make_shared<pixcode::infrastructure::DatabaseManager>(dbPath);

    pixcode::infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY(runner.runMigrations(error));

    pixcode::infrastructure::SqliteAuditLogRepository auditRepo(dbManager);

    pixcode::application::RecordAuditCommand cmd{
        .actorUserId = std::nullopt,
        .action = "SYSTEM_TEST",
        .entityType = "System",
        .entityId = "sys_1",
        .summary = "Automated integration audit test",
        .correlationId = "corr-101",
        .changesJson = "{\"status\":\"ok\"}"
    };

    auditRepo.append(cmd, "2026-10-28T12:00:00Z");

    auto logs = auditRepo.findRecent(10, 0);
    QCOMPARE(static_cast<int>(logs.size()), 1);
    QCOMPARE(logs[0].action, std::string("SYSTEM_TEST"));
    QCOMPARE(logs[0].summary, std::string("Automated integration audit test"));
    QCOMPARE(logs[0].actorUsername, std::string("System"));
}
