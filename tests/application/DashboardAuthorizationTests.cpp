#include "DashboardAuthorizationTests.h"
#include <QtCore/QTemporaryDir>
#include "infrastructure/database/DatabaseManager.h"
#include "infrastructure/migrations/MigrationRunner.h"
#include "infrastructure/system/SystemClock.h"
#include "infrastructure/repositories/SqliteStudentRepository.h"
#include "infrastructure/repositories/SqliteCourseRepository.h"
#include "infrastructure/repositories/SqliteGroupRepository.h"
#include "infrastructure/repositories/SqliteEnrollmentRepository.h"
#include "infrastructure/repositories/SqlitePaymentRepository.h"
#include "infrastructure/repositories/SqliteSessionRepository.h"
#include "infrastructure/repositories/SqliteAttendanceRepository.h"
#include "infrastructure/repositories/SqliteInstructorPayoutRepository.h"
#include "infrastructure/repositories/SqliteInstructorRepository.h"
#include "application/authorization/AuthorizationContext.h"
#include "application/use_cases/DashboardUseCases.h"
#include "domain/entities/User.h"
#include "domain/entities/Role.h"

void DashboardAuthorizationTests::testOwnerReceivesFullFinancialSummary() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto dbManager = std::make_shared<pixcode::infrastructure::DatabaseManager>(tempDir.filePath("dash_test.db"));
    pixcode::infrastructure::MigrationRunner runner(dbManager);
    QString err;
    QVERIFY(runner.runMigrations(err));

    auto students = std::make_shared<pixcode::infrastructure::SqliteStudentRepository>(dbManager);
    auto groups = std::make_shared<pixcode::infrastructure::SqliteGroupRepository>(dbManager);
    auto sessions = std::make_shared<pixcode::infrastructure::SqliteSessionRepository>(dbManager);
    auto attendance = std::make_shared<pixcode::infrastructure::SqliteAttendanceRepository>(dbManager);
    auto enrollments = std::make_shared<pixcode::infrastructure::SqliteEnrollmentRepository>(dbManager);
    auto payments = std::make_shared<pixcode::infrastructure::SqlitePaymentRepository>(dbManager);
    auto payouts = std::make_shared<pixcode::infrastructure::SqliteInstructorPayoutRepository>(dbManager);
    auto clock = std::make_shared<pixcode::infrastructure::SystemClock>();

    // Create Owner user context
    pixcode::domain::User owner("usr_owner", "owner_user", "hash", true, false, 0, std::nullopt, "ar", "2026-01-01T00:00:00Z", "2026-01-01T00:00:00Z", std::nullopt);
    pixcode::domain::Role ownerRole("role_owner", "Owner", "role.owner", true);
    owner.addRole(ownerRole);

    auto authContext = std::make_shared<pixcode::application::AuthorizationContext>(owner);

    pixcode::application::DashboardUseCases dash(students, groups, sessions, attendance, enrollments, payments, payouts, clock, authContext);

    auto res = dash.getDashboardSummary();
    QVERIFY(res.isOk());
    QVERIFY(res.value().isOwner);
    QVERIFY(res.value().financial.has_value());
    QCOMPARE(res.value().financial->totalIncomePiastres, static_cast<std::int64_t>(0));
    QCOMPARE(res.value().financial->netCashBalancePiastres, static_cast<std::int64_t>(0));
}

void DashboardAuthorizationTests::testAdminReceivesOperationalOnlyStrictlyOmittingFinancials() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto dbManager = std::make_shared<pixcode::infrastructure::DatabaseManager>(tempDir.filePath("dash_test2.db"));
    pixcode::infrastructure::MigrationRunner runner(dbManager);
    QString err;
    QVERIFY(runner.runMigrations(err));

    auto students = std::make_shared<pixcode::infrastructure::SqliteStudentRepository>(dbManager);
    auto groups = std::make_shared<pixcode::infrastructure::SqliteGroupRepository>(dbManager);
    auto sessions = std::make_shared<pixcode::infrastructure::SqliteSessionRepository>(dbManager);
    auto attendance = std::make_shared<pixcode::infrastructure::SqliteAttendanceRepository>(dbManager);
    auto enrollments = std::make_shared<pixcode::infrastructure::SqliteEnrollmentRepository>(dbManager);
    auto payments = std::make_shared<pixcode::infrastructure::SqlitePaymentRepository>(dbManager);
    auto payouts = std::make_shared<pixcode::infrastructure::SqliteInstructorPayoutRepository>(dbManager);
    auto clock = std::make_shared<pixcode::infrastructure::SystemClock>();

    // Create Admin user context
    pixcode::domain::User admin("usr_admin", "admin_user", "hash", true, false, 0, std::nullopt, "ar", "2026-01-01T00:00:00Z", "2026-01-01T00:00:00Z", std::nullopt);
    pixcode::domain::Role adminRole("role_admin", "Admin", "role.admin", true);
    admin.addRole(adminRole);

    auto authContext = std::make_shared<pixcode::application::AuthorizationContext>(admin);

    pixcode::application::DashboardUseCases dash(students, groups, sessions, attendance, enrollments, payments, payouts, clock, authContext);

    auto res = dash.getDashboardSummary();
    QVERIFY(res.isOk());
    QVERIFY(!res.value().isOwner);
    // CRITICAL: Financials MUST be std::nullopt for Admin
    QVERIFY(!res.value().financial.has_value());
    // Operational data is still available
    QCOMPARE(res.value().operational.activeStudentsCount, static_cast<std::int64_t>(0));
}

void DashboardAuthorizationTests::testInstructorEarningsCompensationModes() {
    using pixcode::domain::CompensationType;
    using pixcode::domain::Money;
    using pixcode::domain::Session;

    // 1. Hourly: 90 mins @ 200 EGP/hr (20000 piastres) -> (90 * 20000) / 60 = 30000 piastres (300 EGP)
    Money hourlyEarnings = Session::calculateEarnings(90, Money(20000), CompensationType::Hourly);
    QCOMPARE(hourlyEarnings.piasters(), static_cast<std::int64_t>(30000));

    // 2. PerSession: 500 EGP per session regardless of duration
    Money perSessionEarnings = Session::calculateEarnings(45, Money(50000), CompensationType::PerSession);
    QCOMPARE(perSessionEarnings.piasters(), static_cast<std::int64_t>(50000));

    // 3. FixedContract: accrues 0 per session
    Money fixedEarnings = Session::calculateEarnings(120, Money(600000), CompensationType::FixedContract);
    QCOMPARE(fixedEarnings.piasters(), static_cast<std::int64_t>(0));

    // 4. Cancelled session approval test
    Session s;
    s.status = pixcode::domain::SessionStatus::Cancelled;
    s.hourlyRateSnapshot = Money(20000);
    s.payableDurationMinutes = 60;
    s.approve("usr_admin", "2026-01-01T00:00:00Z", CompensationType::Hourly);
    // Cancelled session MUST have earnedAmount = 0
    QCOMPARE(s.earnedAmount.piasters(), static_cast<std::int64_t>(0));
}

