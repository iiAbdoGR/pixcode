#include "AttendanceAndPayrollTests.h"
#include <QtCore/QTemporaryDir>
#include <QtSql/QSqlQuery>
#include "domain/entities/Session.h"
#include "domain/entities/SessionAttendance.h"
#include "domain/entities/InstructorPayout.h"
#include "domain/entities/Instructor.h"
#include "infrastructure/database/DatabaseManager.h"
#include "infrastructure/migrations/MigrationRunner.h"
#include "infrastructure/repositories/SqliteInstructorRepository.h"
#include "infrastructure/repositories/SqliteSessionRepository.h"
#include "infrastructure/repositories/SqliteAttendanceRepository.h"
#include "infrastructure/repositories/SqliteInstructorPayoutRepository.h"
#include "infrastructure/repositories/SqliteCourseRepository.h"
#include "infrastructure/repositories/SqliteGroupRepository.h"
#include "infrastructure/repositories/SqliteStudentRepository.h"

using namespace pixcode;

void AttendanceAndPayrollTests::testSessionEarningsCalculation() {
    domain::Session session;
    session.id = 1;
    session.payableDurationMinutes = 90; // 1.5 hours
    session.hourlyRateSnapshot = domain::Money(20000); // 200.00 EGP

    // (90 * 20000) / 60 = 30000 piastres (300.00 EGP)
    auto earnings = domain::Session::calculateEarnings(session.payableDurationMinutes, session.hourlyRateSnapshot);
    QCOMPARE(earnings.piasters(), 30000);

    // Test 45 minutes at 160 EGP (16000 piastres)
    session.payableDurationMinutes = 45;
    session.hourlyRateSnapshot = domain::Money(16000);
    // (45 * 16000) / 60 = 12000 piastres (120.00 EGP)
    QCOMPARE(domain::Session::calculateEarnings(session.payableDurationMinutes, session.hourlyRateSnapshot).piasters(), 12000);
}

void AttendanceAndPayrollTests::testSessionApprovalStatus() {
    domain::Session session;
    session.status = domain::SessionStatus::Completed;
    session.approvalStatus = domain::SessionApprovalStatus::Pending;
    session.payableDurationMinutes = 120; // 2 hours
    session.hourlyRateSnapshot = domain::Money(25000); // 250.00 EGP

    session.approve("admin-uuid", "2026-10-28T21:00:00Z");

    QCOMPARE(session.approvalStatus, domain::SessionApprovalStatus::Approved);
    QCOMPARE(session.approvedBy.value_or(""), std::string("admin-uuid"));
    QCOMPARE(session.earnedAmount.piasters(), 50000); // 500.00 EGP
}

void AttendanceAndPayrollTests::testInstructorRateHistoryTracking() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString dbPath = tempDir.filePath("test_payroll.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY(runner.runMigrations(error));

    infrastructure::SqliteInstructorRepository instRepo(dbManager);

    domain::Instructor inst;
    inst.fullName = "Ahmed Instructor";
    inst.phone = "01012345678";
    inst.specialization = "C++ & Algorithms";
    inst.hourlyRate = domain::Money(15000); // 150 EGP
    auto saved = instRepo.save(inst);
    QVERIFY(saved.id > 0);

    // Check rate history recorded initial rate
    auto db = dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT hourly_rate_piastres, notes FROM instructor_rate_history WHERE instructor_id = :id;");
    q.bindValue(":id", static_cast<qint64>(saved.id));
    QVERIFY(q.exec());
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toLongLong(), 15000);

    // Now update hourly rate to 200 EGP (20000 piastres)
    QVERIFY(instRepo.updateHourlyRate(saved.id, 20000, "Promoted to Senior Instructor"));

    auto updatedOpt = instRepo.findById(saved.id);
    QVERIFY(updatedOpt.has_value());
    QCOMPARE(updatedOpt->hourlyRate.piastres(), 20000);

    // Verify rate history has 2 records now
    QSqlQuery q2(db);
    q2.prepare("SELECT COUNT(*) FROM instructor_rate_history WHERE instructor_id = :id;");
    q2.bindValue(":id", static_cast<qint64>(saved.id));
    QVERIFY(q2.exec());
    QVERIFY(q2.next());
    QCOMPARE(q2.value(0).toInt(), 2);
}

void AttendanceAndPayrollTests::testAttendanceBatchRecordingAndStats() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString dbPath = tempDir.filePath("test_att.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY(runner.runMigrations(error));

    infrastructure::SqliteCourseRepository courseRepo(dbManager);
    infrastructure::SqliteGroupRepository groupRepo(dbManager);
    infrastructure::SqliteStudentRepository studentRepo(dbManager);
    infrastructure::SqliteSessionRepository sessionRepo(dbManager);
    infrastructure::SqliteAttendanceRepository attRepo(dbManager);

    domain::Course course;
    course.name = "Robotics 101";
    course = courseRepo.save(course);

    domain::Group group;
    group.courseId = course.id;
    group.name = "RoboGroup-A";
    group = groupRepo.save(group);

    // Create 3 students
    domain::Student s1; s1.fullName = "Student One"; s1.studentCode = "ST001"; s1 = studentRepo.save(s1);
    domain::Student s2; s2.fullName = "Student Two"; s2.studentCode = "ST002"; s2 = studentRepo.save(s2);
    domain::Student s3; s3.fullName = "Student Three"; s3.studentCode = "ST003"; s3 = studentRepo.save(s3);

    // Create a session
    domain::Session session;
    session.groupId = group.id;
    session.sessionDate = "2026-10-28";
    session.status = domain::SessionStatus::Completed;
    session = sessionRepo.save(session);
    QVERIFY(session.id > 0);

    // Save batch attendance: s1 present, s2 late, s3 absent
    std::vector<domain::SessionAttendance> batch;
    domain::SessionAttendance a1; a1.sessionId = session.id; a1.studentId = s1.id; a1.status = domain::AttendanceStatus::Present; a1.markedAtUtc = "2026-10-28T10:00:00Z";
    domain::SessionAttendance a2; a2.sessionId = session.id; a2.studentId = s2.id; a2.status = domain::AttendanceStatus::Late; a2.markedAtUtc = "2026-10-28T10:00:00Z";
    domain::SessionAttendance a3; a3.sessionId = session.id; a3.studentId = s3.id; a3.status = domain::AttendanceStatus::Absent; a3.markedAtUtc = "2026-10-28T10:00:00Z";

    batch.push_back(a1);
    batch.push_back(a2);
    batch.push_back(a3);

    QVERIFY(attRepo.saveBatch(batch));

    // Verify records returned by findBySession
    auto list = attRepo.findBySession(session.id);
    QCOMPARE(list.size(), 3ULL);

    // Verify individual status counts
    QCOMPARE(attRepo.countStudentStatus(s1.id, domain::AttendanceStatus::Present), 1);
    QCOMPARE(attRepo.countStudentStatus(s2.id, domain::AttendanceStatus::Late), 1);
    QCOMPARE(attRepo.countStudentStatus(s3.id, domain::AttendanceStatus::Absent), 1);
}

void AttendanceAndPayrollTests::testAttendanceUniqueConstraint() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString dbPath = tempDir.filePath("test_att_dup.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY(runner.runMigrations(error));

    infrastructure::SqliteCourseRepository courseRepo(dbManager);
    infrastructure::SqliteGroupRepository groupRepo(dbManager);
    infrastructure::SqliteStudentRepository studentRepo(dbManager);
    infrastructure::SqliteSessionRepository sessionRepo(dbManager);
    infrastructure::SqliteAttendanceRepository attRepo(dbManager);

    domain::Course course; course.name = "Web Dev"; course = courseRepo.save(course);
    domain::Group group; group.courseId = course.id; group.name = "Web-1"; group = groupRepo.save(group);
    domain::Student s1; s1.fullName = "Ali"; s1.studentCode = "ST101"; s1 = studentRepo.save(s1);

    domain::Session session; session.groupId = group.id; session.sessionDate = "2026-10-28"; session = sessionRepo.save(session);

    // Save initial attendance
    domain::SessionAttendance a1; a1.sessionId = session.id; a1.studentId = s1.id; a1.status = domain::AttendanceStatus::Present; a1.markedAtUtc = "2026-10-28T10:00:00Z";
    QVERIFY(attRepo.saveBatch({a1}));

    // Update existing attendance record via saveBatch (INSERT OR REPLACE)
    domain::SessionAttendance a1Updated; a1Updated.sessionId = session.id; a1Updated.studentId = s1.id; a1Updated.status = domain::AttendanceStatus::Late; a1Updated.markedAtUtc = "2026-10-28T10:15:00Z";
    QVERIFY(attRepo.saveBatch({a1Updated}));

    auto list = attRepo.findBySession(session.id);
    QCOMPARE(list.size(), 1ULL);
    QCOMPARE(list[0].status, domain::AttendanceStatus::Late);
}

void AttendanceAndPayrollTests::testPayoutReceiptNumberingAndBalance() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString dbPath = tempDir.filePath("test_payout_seq.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY(runner.runMigrations(error));

    infrastructure::SqliteInstructorRepository instRepo(dbManager);
    infrastructure::SqliteInstructorPayoutRepository payoutRepo(dbManager);

    domain::Instructor inst;
    inst.fullName = "Mahmoud Hassan";
    inst.hourlyRate = domain::Money(20000);
    inst = instRepo.save(inst);

    // Generate first receipt number
    std::string rec1 = payoutRepo.nextReceiptNumber();
    QVERIFY(rec1.rfind("PAY-", 0) == 0);

    // Record payout
    domain::InstructorPayout p1;
    p1.instructorId = inst.id;
    p1.receiptNumber = rec1;
    p1.amount = domain::Money(50000); // 500.00 EGP
    p1.paymentMethod = domain::PayoutMethod::Cash;
    p1.paymentDateUtc = "2026-10-28T12:00:00Z";
    p1.createdAtUtc = "2026-10-28T12:00:00Z";
    auto savedP1 = payoutRepo.save(p1);
    QVERIFY(savedP1.id > 0);

    // Second receipt number
    std::string rec2 = payoutRepo.nextReceiptNumber();
    QVERIFY(rec2.rfind("PAY-", 0) == 0);
    QVERIFY(rec1 != rec2);

    // Check total paid
    std::int64_t totalPaid = payoutRepo.totalPaidOut(inst.id);
    QCOMPARE(totalPaid, 50000);
}
