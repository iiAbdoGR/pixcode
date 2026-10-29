#include "AttendanceAndPayrollTests.h"
#include <QtCore/QTemporaryDir>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
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
#include "infrastructure/repositories/SqliteUserRepository.h"
#include "infrastructure/repositories/SqliteAuditLogRepository.h"
#include "infrastructure/system/SystemClock.h"
#include "application/authorization/AuthorizationContext.h"
#include "application/use_cases/InstructorPayrollUseCases.h"
#include "application/use_cases/AuditService.h"

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

void AttendanceAndPayrollTests::testSessionSchedulingTimeValidationAndRescheduling() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString dbPath = tempDir.filePath("test_sched.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY(runner.runMigrations(error));

    auto sessionRepo = std::make_shared<infrastructure::SqliteSessionRepository>(dbManager);
    auto payoutRepo = std::make_shared<infrastructure::SqliteInstructorPayoutRepository>(dbManager);
    auto instRepo = std::make_shared<infrastructure::SqliteInstructorRepository>(dbManager);
    auto groupRepo = std::make_shared<infrastructure::SqliteGroupRepository>(dbManager);
    auto courseRepo = std::make_shared<infrastructure::SqliteCourseRepository>(dbManager);
    auto userRepo = std::make_shared<infrastructure::SqliteUserRepository>(dbManager);
    auto auditRepo = std::make_shared<infrastructure::SqliteAuditLogRepository>(dbManager);
    auto clock = std::make_shared<infrastructure::SystemClock>();
    auto auth = std::make_shared<application::AuthorizationContext>();
    auto auditService = std::make_shared<application::AuditService>(auditRepo, clock);

    // Create admin user with Owner role
    domain::User admin("usr_admin", "admin", "hash", true, false, 0, std::nullopt, "en", "2026-01-01T00:00:00Z", "2026-01-01T00:00:00Z", std::nullopt);
    admin.addRole(domain::Role("r_owner", "Owner", "role.owner", true));
    userRepo->save(admin);
    auth->setCurrentUser(admin);

    // Create course and group
    domain::Course c;
    c.name = "Algorithms";
    c = courseRepo->save(c);

    domain::Group g;
    g.courseId = c.id;
    g.name = "Algo 1";
    g.fee = domain::Money(100000);
    g = groupRepo->save(g);

    application::InstructorPayrollUseCases useCases(sessionRepo, payoutRepo, instRepo, groupRepo, courseRepo, clock, auth, auditService);

    // 1. Invalid reversed time range must be rejected
    auto invalidRes = useCases.createSession(application::CreateSessionCommand{
        .groupId = g.id,
        .instructorId = std::nullopt,
        .sessionDate = "2026-11-01",
        .scheduledStartTime = "17:00",
        .scheduledEndTime = "15:00", // Inverted!
        .plannedDurationMinutes = 60,
        .notes = "Inverted test"
    });
    QVERIFY(!invalidRes.isOk());

    // 2. Valid creation
    auto okRes = useCases.createSession(application::CreateSessionCommand{
        .groupId = g.id,
        .instructorId = std::nullopt,
        .sessionDate = "2026-11-01",
        .scheduledStartTime = "15:00",
        .scheduledEndTime = "17:00",
        .plannedDurationMinutes = 120,
        .notes = "Lesson 1"
    });
    QVERIFY(okRes.isOk());
    QVERIFY(okRes.value().id > 0);
    QCOMPARE(okRes.value().scheduledStartTime, std::string("15:00"));
    QCOMPARE(okRes.value().scheduledEndTime, std::string("17:00"));
    QCOMPARE(okRes.value().plannedDurationMinutes, 120);

    // 3. Reschedule / update session
    auto updateRes = useCases.updateSession(application::UpdateSessionCommand{
        .id = okRes.value().id,
        .instructorId = std::nullopt,
        .sessionDate = "2026-11-02",
        .scheduledStartTime = "18:00",
        .scheduledEndTime = "20:00",
        .plannedDurationMinutes = 120,
        .actualStartTime = "",
        .actualEndTime = "",
        .actualDurationMinutes = 0,
        .payableDurationMinutes = 0,
        .status = "scheduled",
        .notes = "Rescheduled to next day"
    });
    QVERIFY(updateRes.isOk());
    QCOMPARE(updateRes.value().sessionDate, std::string("2026-11-02"));
    QCOMPARE(updateRes.value().scheduledStartTime, std::string("18:00"));
    QCOMPARE(updateRes.value().scheduledEndTime, std::string("20:00"));

    // 4. Persistence check: fetch fresh from database
    auto freshOpt = sessionRepo->findById(okRes.value().id);
    QVERIFY(freshOpt.has_value());
    QCOMPARE(freshOpt->sessionDate, std::string("2026-11-02"));
    QCOMPARE(freshOpt->scheduledStartTime, std::string("18:00"));
    QCOMPARE(freshOpt->scheduledEndTime, std::string("20:00"));
}

void AttendanceAndPayrollTests::testInstructorCreationAcrossAllCompensationTypes() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString dbPath = tempDir.filePath("test_inst_modes.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY(runner.runMigrations(error));

    auto sessionRepo = std::make_shared<infrastructure::SqliteSessionRepository>(dbManager);
    auto payoutRepo = std::make_shared<infrastructure::SqliteInstructorPayoutRepository>(dbManager);
    auto instRepo = std::make_shared<infrastructure::SqliteInstructorRepository>(dbManager);
    auto groupRepo = std::make_shared<infrastructure::SqliteGroupRepository>(dbManager);
    auto courseRepo = std::make_shared<infrastructure::SqliteCourseRepository>(dbManager);
    auto userRepo = std::make_shared<infrastructure::SqliteUserRepository>(dbManager);
    auto auditRepo = std::make_shared<infrastructure::SqliteAuditLogRepository>(dbManager);
    auto clock = std::make_shared<infrastructure::SystemClock>();
    auto auth = std::make_shared<application::AuthorizationContext>();
    auto auditService = std::make_shared<application::AuditService>(auditRepo, clock);

    // Create admin user with Owner role
    domain::User admin("usr_admin", "admin", "hash", true, false, 0, std::nullopt, "en", "2026-01-01T00:00:00Z", "2026-01-01T00:00:00Z", std::nullopt);
    admin.addRole(domain::Role("r_owner", "Owner", "role.owner", true));
    userRepo->save(admin);
    auth->setCurrentUser(admin);

    application::InstructorPayrollUseCases useCases(sessionRepo, payoutRepo, instRepo, groupRepo, courseRepo, clock, auth, auditService);

    // 1. Hourly rate instructor
    auto hInst = useCases.createInstructor(application::CreateInstructorCommand{
        .fullName = "Karim Hourly",
        .phone = "01099998888",
        .specialization = "Data Science",
        .compensationType = "hourly",
        .baseRatePiastres = 25000, // 250 EGP/hr
        .notes = "Hourly contract"
    });
    QVERIFY(hInst.isOk());
    QVERIFY(hInst.value().id > 0);
    QCOMPARE(hInst.value().compensationType, std::string("hourly"));
    QCOMPARE(hInst.value().baseRatePiastres, static_cast<std::int64_t>(25000));

    // 2. Per-session instructor
    auto sInst = useCases.createInstructor(application::CreateInstructorCommand{
        .fullName = "Mona PerSession",
        .phone = "01122223333",
        .specialization = "Frontend React",
        .compensationType = "per_session",
        .baseRatePiastres = 40000, // 400 EGP/session
        .notes = "Per session agreement"
    });
    QVERIFY(sInst.isOk());
    QVERIFY(sInst.value().id > 0);
    QCOMPARE(sInst.value().compensationType, std::string("per_session"));
    QCOMPARE(sInst.value().baseRatePiastres, static_cast<std::int64_t>(40000));

    // 3. Fixed contract instructor
    auto fInst = useCases.createInstructor(application::CreateInstructorCommand{
        .fullName = "Tamer Fixed",
        .phone = "01255556666",
        .specialization = "Full Stack",
        .compensationType = "fixed_contract",
        .baseRatePiastres = 1500000, // 15,000 EGP/month
        .notes = "Monthly salary"
    });
    QVERIFY(fInst.isOk());
    QVERIFY(fInst.value().id > 0);
    QCOMPARE(fInst.value().compensationType, std::string("fixed_contract"));
    QCOMPARE(fInst.value().baseRatePiastres, static_cast<std::int64_t>(1500000));

    // 4. Verify no user accounts were created in users table for these instructors
    auto allUsers = userRepo->findAll();
    for (const auto& u : allUsers) {
        QVERIFY(u.username() != "Karim Hourly");
        QVERIFY(u.username() != "Mona PerSession");
        QVERIFY(u.username() != "Tamer Fixed");
    }

    // 5. Verify persistence after reloading from repository
    auto list = useCases.listInstructors({});
    QVERIFY(list.isOk());
    QCOMPARE(list.value().size(), static_cast<size_t>(3));
}

void AttendanceAndPayrollTests::testDuplicateSessionApprovalBlocked() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString dbPath = tempDir.filePath("test_dup_approval.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY(runner.runMigrations(error));

    auto sessionRepo = std::make_shared<infrastructure::SqliteSessionRepository>(dbManager);
    auto instRepo = std::make_shared<infrastructure::SqliteInstructorRepository>(dbManager);
    auto payoutRepo = std::make_shared<infrastructure::SqliteInstructorPayoutRepository>(dbManager);
    auto courseRepo = std::make_shared<infrastructure::SqliteCourseRepository>(dbManager);
    auto groupRepo = std::make_shared<infrastructure::SqliteGroupRepository>(dbManager);
    auto studentRepo = std::make_shared<infrastructure::SqliteStudentRepository>(dbManager);
    auto clock = std::make_shared<infrastructure::SystemClock>();
    auto auditLogRepo = std::make_shared<infrastructure::SqliteAuditLogRepository>(dbManager);
    auto auditService = std::make_shared<application::AuditService>(auditLogRepo, clock);
    auto authContext = std::make_shared<application::AuthorizationContext>();

    // Set Owner with approval permissions and insert into users table for foreign key constraint
    {
        auto db = dbManager->getConnection();
        QSqlQuery uq(db);
        bool ok = uq.exec("INSERT OR IGNORE INTO users (id, username, password_hash, is_active, preferred_language, created_at_utc, updated_at_utc) "
                          "VALUES ('usr_owner', 'owner', 'hash', 1, 'ar', '2026-01-01', '2026-01-01');");
        QVERIFY2(ok, uq.lastError().text().toStdString().c_str());
        uq.exec("INSERT OR IGNORE INTO user_roles (user_id, role_id) VALUES ('usr_owner', 'role_owner');");
    }
    domain::Role ownerRole("role_owner", "Owner", "role.owner", true);
    domain::User owner("usr_owner", "owner", "hash", true, false, 0, std::nullopt, "ar", "2026-01-01", "2026-01-01", std::nullopt);
    owner.addRole(ownerRole);
    authContext->setCurrentUser(owner);

    application::InstructorPayrollUseCases useCases(
        sessionRepo, payoutRepo, instRepo, groupRepo, courseRepo, clock, authContext, auditService
    );

    // Create instructor and group
    auto inst = useCases.createInstructor(application::CreateInstructorCommand{
        .fullName = "Test Instructor",
        .phone = "01000000001",
        .specialization = "Coding",
        .compensationType = "hourly",
        .baseRatePiastres = 20000,
        .notes = ""
    }).value();

    domain::Course course;
    course.name = "C++ Mastery";
    auto savedCourse = courseRepo->save(course);

    domain::Group group;
    group.courseId = savedCourse.id;
    group.instructorId = inst.id;
    group.name = "Group A";
    group.capacity = 10;
    auto savedGroup = groupRepo->save(group);

    // Schedule a completed session
    domain::Session session;
    session.groupId = savedGroup.id;
    session.instructorId = inst.id;
    session.sessionDate = "2026-10-29";
    session.scheduledStartTime = "10:00";
    session.scheduledEndTime = "11:00";
    session.plannedDurationMinutes = 60;
    session.actualDurationMinutes = 60;
    session.payableDurationMinutes = 60;
    session.status = domain::SessionStatus::Completed;
    session.approvalStatus = domain::SessionApprovalStatus::Pending;
    session.settlementStatus = domain::SessionSettlementStatus::Unpaid;
    auto savedSession = sessionRepo->save(session);

    // 1. First approval MUST succeed
    application::ApproveSessionCommand cmd;
    cmd.sessionId = savedSession.id;
    cmd.approvedPayableMinutes = 60;
    cmd.approvedByUserId = "usr_owner";

    auto res1 = useCases.approveSession(cmd);
    QVERIFY2(res1.isOk(), res1.isOk() ? "" : res1.error().message().c_str());
    QCOMPARE(res1.value().approvalStatus, std::string("approved"));
    QCOMPARE(res1.value().earnedAmountPiastres, static_cast<std::int64_t>(20000));

    // 2. Duplicate approval MUST fail and be blocked with BusinessRuleViolation
    auto res2 = useCases.approveSession(cmd);
    QVERIFY(!res2.isOk());
    QCOMPARE(res2.error().code(), domain::ErrorCode::BusinessRuleViolation);

    // 3. Repeated approval request MUST still be blocked
    auto res3 = useCases.approveSession(cmd);
    QVERIFY(!res3.isOk());
    QCOMPARE(res3.error().code(), domain::ErrorCode::BusinessRuleViolation);
}

void AttendanceAndPayrollTests::testPayoutAllocationAndSettlement() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString dbPath = tempDir.filePath("test_payout_alloc.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY(runner.runMigrations(error));

    auto sessionRepo = std::make_shared<infrastructure::SqliteSessionRepository>(dbManager);
    auto instRepo = std::make_shared<infrastructure::SqliteInstructorRepository>(dbManager);
    auto payoutRepo = std::make_shared<infrastructure::SqliteInstructorPayoutRepository>(dbManager);
    auto courseRepo = std::make_shared<infrastructure::SqliteCourseRepository>(dbManager);
    auto groupRepo = std::make_shared<infrastructure::SqliteGroupRepository>(dbManager);
    auto clock = std::make_shared<infrastructure::SystemClock>();
    auto auditLogRepo = std::make_shared<infrastructure::SqliteAuditLogRepository>(dbManager);
    auto auditService = std::make_shared<application::AuditService>(auditLogRepo, clock);
    auto authContext = std::make_shared<application::AuthorizationContext>();

    // Owner role and insert into users table for foreign key constraint
    {
        auto db = dbManager->getConnection();
        QSqlQuery uq(db);
        bool ok = uq.exec("INSERT OR IGNORE INTO users (id, username, password_hash, is_active, preferred_language, created_at_utc, updated_at_utc) "
                          "VALUES ('usr_owner', 'owner', 'hash', 1, 'ar', '2026-01-01', '2026-01-01');");
        QVERIFY2(ok, uq.lastError().text().toStdString().c_str());
        uq.exec("INSERT OR IGNORE INTO user_roles (user_id, role_id) VALUES ('usr_owner', 'role_owner');");
    }
    domain::Role ownerRole("role_owner", "Owner", "role.owner", true);
    domain::User owner("usr_owner", "owner", "hash", true, false, 0, std::nullopt, "ar", "2026-01-01", "2026-01-01", std::nullopt);
    owner.addRole(ownerRole);
    authContext->setCurrentUser(owner);

    application::InstructorPayrollUseCases useCases(
        sessionRepo, payoutRepo, instRepo, groupRepo, courseRepo, clock, authContext, auditService
    );

    // Create instructor
    auto inst = useCases.createInstructor(application::CreateInstructorCommand{
        .fullName = "Ali Instructor",
        .phone = "01099999999",
        .specialization = "Python",
        .compensationType = "hourly",
        .baseRatePiastres = 20000,
        .notes = ""
    }).value();

    // Create session and approve it
    domain::Course course;
    course.name = "Python 101";
    auto sc = courseRepo->save(course);

    domain::Group grp;
    grp.courseId = sc.id;
    grp.instructorId = inst.id;
    grp.name = "Group Py";
    auto sg = groupRepo->save(grp);

    domain::Session sess;
    sess.groupId = sg.id;
    sess.instructorId = inst.id;
    sess.sessionDate = "2026-10-29";
    sess.payableDurationMinutes = 60;
    sess.status = domain::SessionStatus::Completed;
    sess.approvalStatus = domain::SessionApprovalStatus::Pending;
    sess.settlementStatus = domain::SessionSettlementStatus::Unpaid;
    auto savedSess = sessionRepo->save(sess);

    auto apprRes = useCases.approveSession(application::ApproveSessionCommand{
        .sessionId = savedSess.id,
        .approvedPayableMinutes = 60,
        .approvedByUserId = "usr_owner"
    });
    QVERIFY(apprRes.isOk());

    // Check summary before payout
    auto s1 = useCases.getInstructorPayrollSummary(inst.id).value();
    QCOMPARE(s1.grossEarnedPiastres, static_cast<std::int64_t>(20000));
    QCOMPARE(s1.totalPaidOutPiastres, static_cast<std::int64_t>(0));
    QCOMPARE(s1.outstandingDuesPiastres, static_cast<std::int64_t>(20000));
    QCOMPARE(s1.settlementStatus, std::string("unpaid"));

    // Execute full payout of 20000 piastres (200 EGP)
    application::RecordInstructorPayoutCommand pCmd{
        .instructorId = inst.id,
        .amountPiastres = 20000,
        .paymentDateUtc = "2026-10-29T12:00:00Z",
        .paymentMethod = "cash",
        .referenceNumber = "REF-101",
        .notes = "Full settlement",
        .createdByUserId = "usr_owner"
    };
    auto pRes = useCases.recordPayout(pCmd);
    QVERIFY(pRes.isOk());

    // Verify summary after payout
    auto s2 = useCases.getInstructorPayrollSummary(inst.id).value();
    QCOMPARE(s2.grossEarnedPiastres, static_cast<std::int64_t>(20000));
    QCOMPARE(s2.totalPaidOutPiastres, static_cast<std::int64_t>(20000));
    QCOMPARE(s2.outstandingDuesPiastres, static_cast<std::int64_t>(0));
    QCOMPARE(s2.settlementStatus, std::string("fully_paid"));

    // Verify session settlement status updated to fully_paid
    auto reloadedSession = sessionRepo->findById(savedSess.id).value();
    QCOMPARE(reloadedSession.settlementStatus, domain::SessionSettlementStatus::FullyPaid);

    // Verify allocation record exists
    auto allocs = payoutRepo->findAllocationsByPayout(pRes.value().id);
    QCOMPARE(allocs.size(), static_cast<size_t>(1));
    QCOMPARE(allocs[0].sessionId.value_or(0), savedSess.id);
    QCOMPARE(allocs[0].allocatedAmount.piasters(), static_cast<std::int64_t>(20000));

    // Attempting to pay again MUST fail (cannot overpay)
    auto pRes2 = useCases.recordPayout(pCmd);
    QVERIFY(!pRes2.isOk());
    QCOMPARE(pRes2.error().code(), domain::ErrorCode::BusinessRuleViolation);
}
