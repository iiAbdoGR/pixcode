#include "SubscriptionLifecycleTests.h"
#include <QtCore/QTemporaryDir>
#include "infrastructure/database/DatabaseManager.h"
#include "infrastructure/migrations/MigrationRunner.h"
#include "infrastructure/repositories/SqliteStudentRepository.h"
#include "infrastructure/repositories/SqliteCourseRepository.h"
#include "infrastructure/repositories/SqliteGroupRepository.h"
#include "infrastructure/repositories/SqliteEnrollmentRepository.h"
#include "infrastructure/repositories/SqlitePaymentRepository.h"
#include "infrastructure/repositories/SqliteUserRepository.h"
#include "application/use_cases/EnrollmentUseCases.h"

void SubscriptionLifecycleTests::testCancellationAndAuditFields() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto dbManager = std::make_shared<pixcode::infrastructure::DatabaseManager>(tempDir.filePath("sub_test.db"));
    pixcode::infrastructure::MigrationRunner runner(dbManager);
    QString err;
    QVERIFY(runner.runMigrations(err));

    auto users = std::make_shared<pixcode::infrastructure::SqliteUserRepository>(dbManager);
    pixcode::domain::User adminUser("usr_admin_1", "admin1", "hash", true, false, 0, std::nullopt, "ar", "2026-01-01T00:00:00Z", "2026-01-01T00:00:00Z", std::nullopt);
    users->save(adminUser);

    auto students = std::make_shared<pixcode::infrastructure::SqliteStudentRepository>(dbManager);
    auto courses = std::make_shared<pixcode::infrastructure::SqliteCourseRepository>(dbManager);
    auto groups = std::make_shared<pixcode::infrastructure::SqliteGroupRepository>(dbManager);
    auto enrollments = std::make_shared<pixcode::infrastructure::SqliteEnrollmentRepository>(dbManager);
    auto payments = std::make_shared<pixcode::infrastructure::SqlitePaymentRepository>(dbManager);

    pixcode::domain::Student st;
    st.studentCode = "ST-101";
    st.fullName = "Ahmed Ali";
    auto savedStudent = students->save(st);

    pixcode::domain::Course c;
    c.name = "Robotics";
    c.defaultFee = pixcode::domain::Money(150000);
    auto savedCourse = courses->save(c);

    pixcode::domain::Group g;
    g.courseId = savedCourse.id;
    g.name = "Group A";
    g.capacity = 10;
    g.fee = pixcode::domain::Money(150000);
    g.status = pixcode::domain::GroupStatus::Open;
    auto savedGroup = groups->save(g);

    pixcode::application::EnrollmentUseCases useCases(enrollments, groups, students, payments, courses);

    // 1. Enroll student
    auto enrollRes = useCases.enrollStudent(pixcode::application::EnrollStudentCommand{
        .studentId = savedStudent.id,
        .groupId = savedGroup.id,
        .discountPiastres = 20000,
        .notes = "Early bird"
    });
    QVERIFY(enrollRes.isOk());
    QCOMPARE(enrollRes.value().status, std::string("active"));

    // 2. Cancel subscription
    auto cancelRes = useCases.cancelEnrollment(pixcode::application::CancelSubscriptionCommand{
        .enrollmentId = enrollRes.value().id,
        .reason = "Relocated to another city",
        .cancelledByUserId = "usr_admin_1"
    });
    QVERIFY(cancelRes.isOk());
    QCOMPARE(cancelRes.value().status, std::string("cancelled"));
    QCOMPARE(cancelRes.value().cancellationReason.value_or(""), std::string("Relocated to another city"));
    QCOMPARE(cancelRes.value().cancelledByUserId.value_or(""), std::string("usr_admin_1"));
    QVERIFY(cancelRes.value().cancelledAtUtc.has_value());
}

void SubscriptionLifecycleTests::testRestorationCapacityAndDuplicateCheck() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto dbManager = std::make_shared<pixcode::infrastructure::DatabaseManager>(tempDir.filePath("sub_test2.db"));
    pixcode::infrastructure::MigrationRunner runner(dbManager);
    QString err;
    QVERIFY(runner.runMigrations(err));

    auto users = std::make_shared<pixcode::infrastructure::SqliteUserRepository>(dbManager);
    pixcode::domain::User adminUser("usr_admin_2", "admin2", "hash", true, false, 0, std::nullopt, "ar", "2026-01-01T00:00:00Z", "2026-01-01T00:00:00Z", std::nullopt);
    users->save(adminUser);

    auto students = std::make_shared<pixcode::infrastructure::SqliteStudentRepository>(dbManager);
    auto courses = std::make_shared<pixcode::infrastructure::SqliteCourseRepository>(dbManager);
    auto groups = std::make_shared<pixcode::infrastructure::SqliteGroupRepository>(dbManager);
    auto enrollments = std::make_shared<pixcode::infrastructure::SqliteEnrollmentRepository>(dbManager);
    auto payments = std::make_shared<pixcode::infrastructure::SqlitePaymentRepository>(dbManager);

    pixcode::domain::Student st;
    st.studentCode = "ST-102";
    st.fullName = "Sarah Tarek";
    auto savedStudent = students->save(st);

    pixcode::domain::Course c;
    c.name = "Python 101";
    c.defaultFee = pixcode::domain::Money(100000);
    auto savedCourse = courses->save(c);

    // Create group with capacity = 1
    pixcode::domain::Group g;
    g.courseId = savedCourse.id;
    g.name = "Group Small";
    g.capacity = 1;
    g.fee = pixcode::domain::Money(100000);
    g.status = pixcode::domain::GroupStatus::Open;
    auto savedGroup = groups->save(g);

    pixcode::application::EnrollmentUseCases useCases(enrollments, groups, students, payments, courses);

    auto enrollRes = useCases.enrollStudent(pixcode::application::EnrollStudentCommand{
        .studentId = savedStudent.id,
        .groupId = savedGroup.id,
        .discountPiastres = 0,
        .notes = ""
    });
    QVERIFY(enrollRes.isOk());

    // Cancel Sarah's enrollment
    auto cancelRes = useCases.cancelEnrollment(pixcode::application::CancelSubscriptionCommand{
        .enrollmentId = enrollRes.value().id,
        .reason = "Temporary pause",
        .cancelledByUserId = "usr_admin_2"
    });
    QVERIFY(cancelRes.isOk());

    // Now enroll another student to fill the only seat in the group
    pixcode::domain::Student st2;
    st2.studentCode = "ST-103";
    st2.fullName = "Omar Khaled";
    auto savedStudent2 = students->save(st2);

    auto enrollRes2 = useCases.enrollStudent(pixcode::application::EnrollStudentCommand{
        .studentId = savedStudent2.id,
        .groupId = savedGroup.id,
        .discountPiastres = 0,
        .notes = ""
    });
    QVERIFY(enrollRes2.isOk());

    // Attempting to restore Sarah's subscription must fail because capacity is full!
    auto restoreFail = useCases.restoreSubscription(pixcode::application::RestoreSubscriptionCommand{
        .enrollmentId = enrollRes.value().id,
        .reason = "Wants to return",
        .restoredByUserId = "usr_admin_2"
    });
    QVERIFY(!restoreFail.isOk());
    QVERIFY(restoreFail.error().message().find("capacity") != std::string::npos);

    // Cancel Omar's enrollment to free the seat
    useCases.cancelEnrollment(enrollRes2.value().id, "Omar withdrew");

    // Now restoring Sarah's subscription must succeed!
    auto restoreOk = useCases.restoreSubscription(pixcode::application::RestoreSubscriptionCommand{
        .enrollmentId = enrollRes.value().id,
        .reason = "Seat freed, returning",
        .restoredByUserId = "usr_admin_2"
    });
    QVERIFY(restoreOk.isOk());
    QCOMPARE(restoreOk.value().status, std::string("active"));
    QCOMPARE(restoreOk.value().restorationReason.value_or(""), std::string("Seat freed, returning"));
}

void SubscriptionLifecycleTests::testReEnrollmentAllowedAfterCancellation() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto dbManager = std::make_shared<pixcode::infrastructure::DatabaseManager>(tempDir.filePath("sub_test3.db"));
    pixcode::infrastructure::MigrationRunner runner(dbManager);
    QString err;
    QVERIFY(runner.runMigrations(err));

    auto students = std::make_shared<pixcode::infrastructure::SqliteStudentRepository>(dbManager);
    auto courses = std::make_shared<pixcode::infrastructure::SqliteCourseRepository>(dbManager);
    auto groups = std::make_shared<pixcode::infrastructure::SqliteGroupRepository>(dbManager);
    auto enrollments = std::make_shared<pixcode::infrastructure::SqliteEnrollmentRepository>(dbManager);
    auto payments = std::make_shared<pixcode::infrastructure::SqlitePaymentRepository>(dbManager);

    pixcode::domain::Student st;
    st.studentCode = "ST-104";
    st.fullName = "Laila Hassan";
    auto savedStudent = students->save(st);

    pixcode::domain::Course c;
    c.name = "Web Dev";
    c.defaultFee = pixcode::domain::Money(200000);
    auto savedCourse = courses->save(c);

    pixcode::domain::Group g;
    g.courseId = savedCourse.id;
    g.name = "Group Web 1";
    g.capacity = 10;
    g.fee = pixcode::domain::Money(200000);
    g.status = pixcode::domain::GroupStatus::Open;
    auto savedGroup = groups->save(g);

    pixcode::application::EnrollmentUseCases useCases(enrollments, groups, students, payments, courses);

    // 1st enrollment
    auto e1 = useCases.enrollStudent(pixcode::application::EnrollStudentCommand{
        .studentId = savedStudent.id,
        .groupId = savedGroup.id,
        .discountPiastres = 0,
        .notes = "Term 1"
    });
    QVERIFY(e1.isOk());

    // Prevent duplicate active enrollment
    auto duplicateErr = useCases.enrollStudent(pixcode::application::EnrollStudentCommand{
        .studentId = savedStudent.id,
        .groupId = savedGroup.id,
        .discountPiastres = 0,
        .notes = "Duplicate attempt"
    });
    QVERIFY(!duplicateErr.isOk());

    // Cancel 1st enrollment
    useCases.cancelEnrollment(e1.value().id, "Cancelled term 1");

    // Re-enrollment of same student in same group must now succeed cleanly!
    auto e2 = useCases.enrollStudent(pixcode::application::EnrollStudentCommand{
        .studentId = savedStudent.id,
        .groupId = savedGroup.id,
        .discountPiastres = 10000,
        .notes = "Term 2 fresh enrollment"
    });
    QVERIFY(e2.isOk());
    QVERIFY(e2.value().id != e1.value().id);
    QCOMPARE(e2.value().status, std::string("active"));

    // Verify history lists both enrollments
    auto list = useCases.listByStudent(savedStudent.id);
    QVERIFY(list.isOk());
    QCOMPARE(list.value().size(), static_cast<size_t>(2));
}

void SubscriptionLifecycleTests::testPaymentHistoryPreservation() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    auto dbManager = std::make_shared<pixcode::infrastructure::DatabaseManager>(tempDir.filePath("sub_test4.db"));
    pixcode::infrastructure::MigrationRunner runner(dbManager);
    QString err;
    QVERIFY(runner.runMigrations(err));

    auto students = std::make_shared<pixcode::infrastructure::SqliteStudentRepository>(dbManager);
    auto courses = std::make_shared<pixcode::infrastructure::SqliteCourseRepository>(dbManager);
    auto groups = std::make_shared<pixcode::infrastructure::SqliteGroupRepository>(dbManager);
    auto enrollments = std::make_shared<pixcode::infrastructure::SqliteEnrollmentRepository>(dbManager);
    auto payments = std::make_shared<pixcode::infrastructure::SqlitePaymentRepository>(dbManager);

    pixcode::domain::Student st;
    st.studentCode = "ST-105";
    st.fullName = "Hassan Mostafa";
    auto savedStudent = students->save(st);

    pixcode::domain::Course c;
    c.name = "AI Basics";
    c.defaultFee = pixcode::domain::Money(300000);
    auto savedCourse = courses->save(c);

    pixcode::domain::Group g;
    g.courseId = savedCourse.id;
    g.name = "Group AI 1";
    g.capacity = 10;
    g.fee = pixcode::domain::Money(300000);
    g.status = pixcode::domain::GroupStatus::Open;
    auto savedGroup = groups->save(g);

    pixcode::application::EnrollmentUseCases useCases(enrollments, groups, students, payments, courses);

    auto e = useCases.enrollStudent(pixcode::application::EnrollStudentCommand{
        .studentId = savedStudent.id,
        .groupId = savedGroup.id,
        .discountPiastres = 50000,
        .notes = ""
    });
    QVERIFY(e.isOk());

    // Record installment payment
    auto p = useCases.recordPayment(pixcode::application::RecordPaymentCommand{
        .enrollmentId = e.value().id,
        .amountPiastres = 100000,
        .paymentMethod = "cash",
        .notes = "First payment",
        .recordedByUserId = std::nullopt
    });
    QVERIFY(p.isOk());

    // Cancel enrollment
    useCases.cancelEnrollment(e.value().id, "Left course");

    // Historical payment and receipt must be fully preserved
    auto payList = useCases.listPayments(e.value().id);
    QVERIFY(payList.isOk());
    QCOMPARE(payList.value().size(), static_cast<size_t>(1));
    QCOMPARE(payList.value().front().amountPiastres, static_cast<std::int64_t>(100000));
    QVERIFY(!payList.value().front().receiptNumber.empty());
}
