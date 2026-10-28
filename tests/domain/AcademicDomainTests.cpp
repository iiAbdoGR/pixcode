#include "AcademicDomainTests.h"
#include <QtCore/QTemporaryDir>
#include "infrastructure/database/DatabaseManager.h"
#include "infrastructure/migrations/MigrationRunner.h"
#include "infrastructure/repositories/SqliteStudentRepository.h"
#include "infrastructure/repositories/SqliteCourseRepository.h"
#include "infrastructure/repositories/SqliteInstructorRepository.h"
#include "infrastructure/repositories/SqliteGroupRepository.h"
#include "infrastructure/repositories/SqliteEnrollmentRepository.h"
#include "infrastructure/repositories/SqlitePaymentRepository.h"
#include "application/authorization/AuthorizationContext.h"
#include "application/use_cases/StudentUseCases.h"
#include "application/use_cases/CourseUseCases.h"
#include "application/use_cases/GroupUseCases.h"
#include "application/use_cases/EnrollmentUseCases.h"

using namespace pixcode;

void AcademicDomainTests::testStudentWorkflow() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = tempDir.filePath("test_students.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY2(runner.runMigrations(error), error.toStdString().c_str());

    auto studentRepo = std::make_shared<infrastructure::SqliteStudentRepository>(dbManager);
    auto authContext = std::make_shared<application::AuthorizationContext>();
    application::StudentUseCases studentUseCases(studentRepo, authContext);

    // 1. Create Student
    application::CreateStudentCommand cmd{
        .fullName = "Ahmed Ali",
        .phone = "01012345678",
        .guardianName = "Ali Mohamed",
        .guardianPhone = "01098765432",
        .address = "Cairo, Egypt",
        .notes = "Enrolled in morning batch"
    };

    auto res = studentUseCases.createStudent(cmd);
    QVERIFY(res.isOk());
    QCOMPARE(res.value().fullName, "Ahmed Ali");
    QCOMPARE(res.value().studentCode, "STU-00001");
    QCOMPARE(res.value().status, "active");

    // 2. Retrieve Student
    auto getRes = studentUseCases.getStudent(res.value().id);
    QVERIFY(getRes.isOk());
    QCOMPARE(getRes.value().phone, "01012345678");

    // 3. Search Student
    application::StudentFilter filter;
    filter.searchText = "Ahmed";
    auto listRes = studentUseCases.listStudents(filter);
    QVERIFY(listRes.isOk());
    QCOMPARE(listRes.value().size(), 1ULL);

    // 4. Archive Student
    auto archRes = studentUseCases.archiveStudent(res.value().id);
    QVERIFY(archRes.isOk());
    QCOMPARE(archRes.value().status, "archived");

    // 5. Activate Student
    auto actRes = studentUseCases.activateStudent(res.value().id);
    QVERIFY(actRes.isOk());
    QCOMPARE(actRes.value().status, "active");
}

void AcademicDomainTests::testCourseAndGroupWorkflow() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = tempDir.filePath("test_courses.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY2(runner.runMigrations(error), error.toStdString().c_str());

    auto courseRepo = std::make_shared<infrastructure::SqliteCourseRepository>(dbManager);
    auto groupRepo = std::make_shared<infrastructure::SqliteGroupRepository>(dbManager);
    auto instRepo = std::make_shared<infrastructure::SqliteInstructorRepository>(dbManager);

    application::CourseUseCases courseUseCases(courseRepo);
    application::GroupUseCases groupUseCases(groupRepo, courseRepo, instRepo);

    // 1. Create Course
    application::CreateCourseCommand cCmd{
        .name = "Python for Kids",
        .description = "Learn fundamentals of Python",
        .durationWeeks = 8,
        .defaultFeePiastres = 150000 // 1500 EGP
    };

    auto cRes = courseUseCases.createCourse(cCmd);
    QVERIFY(cRes.isOk());
    QCOMPARE(cRes.value().name, "Python for Kids");
    QCOMPARE(cRes.value().defaultFeePiastres, 150000);

    // 2. Create Group with fee snapshot
    application::CreateGroupCommand gCmd{
        .courseId = cRes.value().id,
        .instructorId = std::nullopt,
        .name = "Python Group A - Saturday",
        .startDate = "2026-11-01",
        .endDate = "2026-12-25",
        .scheduleDescription = "Saturday 10:00 AM - 12:00 PM",
        .capacity = 2, // capacity of 2 students
        .feePiastres = 140000 // promotional price 1400 EGP
    };

    auto gRes = groupUseCases.createGroup(gCmd);
    QVERIFY(gRes.isOk());
    QCOMPARE(gRes.value().name, "Python Group A - Saturday");
    QCOMPARE(gRes.value().capacity, 2);
    QCOMPARE(gRes.value().feePiastres, 140000);
}

void AcademicDomainTests::testEnrollmentAndCapacityEnforcement() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = tempDir.filePath("test_enrollment.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY2(runner.runMigrations(error), error.toStdString().c_str());

    auto studentRepo = std::make_shared<infrastructure::SqliteStudentRepository>(dbManager);
    auto courseRepo = std::make_shared<infrastructure::SqliteCourseRepository>(dbManager);
    auto instRepo = std::make_shared<infrastructure::SqliteInstructorRepository>(dbManager);
    auto groupRepo = std::make_shared<infrastructure::SqliteGroupRepository>(dbManager);
    auto enrollRepo = std::make_shared<infrastructure::SqliteEnrollmentRepository>(dbManager);
    auto payRepo = std::make_shared<infrastructure::SqlitePaymentRepository>(dbManager);

    auto authContext = std::make_shared<application::AuthorizationContext>();
    application::StudentUseCases studentUseCases(studentRepo, authContext);
    application::CourseUseCases courseUseCases(courseRepo);
    application::GroupUseCases groupUseCases(groupRepo, courseRepo, instRepo);
    application::EnrollmentUseCases enrollUseCases(enrollRepo, groupRepo, studentRepo, payRepo, courseRepo);

    // Setup: 3 students
    auto s1 = studentUseCases.createStudent({.fullName = "Student One", .phone = "0101"}).value();
    auto s2 = studentUseCases.createStudent({.fullName = "Student Two", .phone = "0102"}).value();
    auto s3 = studentUseCases.createStudent({.fullName = "Student Three", .phone = "0103"}).value();

    // Setup: 1 Course, 1 Group with capacity = 2
    auto c = courseUseCases.createCourse({.name = "Robotics", .defaultFeePiastres = 200000}).value();
    auto g = groupUseCases.createGroup({.courseId = c.id, .name = "Robotics A", .capacity = 2, .feePiastres = 200000}).value();

    // 1. Enroll Student One (with 200 EGP discount = 20000 piastres)
    auto e1 = enrollUseCases.enrollStudent({
        .studentId = s1.id,
        .groupId = g.id,
        .discountPiastres = 20000,
        .notes = "Early bird discount"
    });
    QVERIFY(e1.isOk());
    QCOMPARE(e1.value().agreedFeePiastres, 200000);
    QCOMPARE(e1.value().discountPiastres, 20000);
    QCOMPARE(e1.value().netFeePiastres, 180000);
    QCOMPARE(e1.value().balancePiastres, 180000);

    // 2. Prevent Duplicate Enrollment
    auto dup = enrollUseCases.enrollStudent({.studentId = s1.id, .groupId = g.id});
    QVERIFY(dup.isError());

    // 3. Enroll Student Two (reaches capacity = 2)
    auto e2 = enrollUseCases.enrollStudent({.studentId = s2.id, .groupId = g.id});
    QVERIFY(e2.isOk());

    // 4. Capacity Enforcement: Student Three should be rejected
    auto e3 = enrollUseCases.enrollStudent({.studentId = s3.id, .groupId = g.id});
    QVERIFY(e3.isError());
}

void AcademicDomainTests::testPaymentAndReceiptWorkflow() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dbPath = tempDir.filePath("test_payments.db");

    auto dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    infrastructure::MigrationRunner runner(dbManager);
    QString error;
    QVERIFY2(runner.runMigrations(error), error.toStdString().c_str());

    auto studentRepo = std::make_shared<infrastructure::SqliteStudentRepository>(dbManager);
    auto courseRepo = std::make_shared<infrastructure::SqliteCourseRepository>(dbManager);
    auto instRepo = std::make_shared<infrastructure::SqliteInstructorRepository>(dbManager);
    auto groupRepo = std::make_shared<infrastructure::SqliteGroupRepository>(dbManager);
    auto enrollRepo = std::make_shared<infrastructure::SqliteEnrollmentRepository>(dbManager);
    auto payRepo = std::make_shared<infrastructure::SqlitePaymentRepository>(dbManager);

    auto authContext = std::make_shared<application::AuthorizationContext>();
    application::StudentUseCases studentUseCases(studentRepo, authContext);
    application::CourseUseCases courseUseCases(courseRepo);
    application::GroupUseCases groupUseCases(groupRepo, courseRepo, instRepo);
    application::EnrollmentUseCases enrollUseCases(enrollRepo, groupRepo, studentRepo, payRepo, courseRepo);

    auto s = studentUseCases.createStudent({.fullName = "Youssef Tarek", .phone = "011223344"}).value();
    auto c = courseUseCases.createCourse({.name = "Web Dev", .defaultFeePiastres = 300000}).value();
    auto g = groupUseCases.createGroup({.courseId = c.id, .name = "Web Batch 1", .capacity = 10, .feePiastres = 300000}).value();

    auto enrollment = enrollUseCases.enrollStudent({.studentId = s.id, .groupId = g.id, .discountPiastres = 0}).value();
    QCOMPARE(enrollment.netFeePiastres, 300000); // 3000 EGP

    // 1. Partial payment: 1000 EGP (100000 piastres)
    auto p1 = enrollUseCases.recordPayment({
        .enrollmentId = enrollment.id,
        .amountPiastres = 100000,
        .paymentMethod = "cash",
        .notes = "First installment"
    });
    QVERIFY(p1.isOk());
    QVERIFY(QString::fromStdString(p1.value().receiptNumber).contains("RCP-"));

    // Check balance
    auto eAfterP1 = enrollUseCases.getEnrollment(enrollment.id).value();
    QCOMPARE(eAfterP1.paidPiastres, 100000);
    QCOMPARE(eAfterP1.balancePiastres, 200000); // 2000 EGP remaining

    // 2. Reject Overpayment (trying to pay 2500 EGP when balance is 2000 EGP)
    auto overpay = enrollUseCases.recordPayment({
        .enrollmentId = enrollment.id,
        .amountPiastres = 250000,
        .paymentMethod = "cash"
    });
    QVERIFY(overpay.isError());

    // 3. Second payment: 2000 EGP (fully settles balance)
    auto p2 = enrollUseCases.recordPayment({
        .enrollmentId = enrollment.id,
        .amountPiastres = 200000,
        .paymentMethod = "bank_transfer",
        .notes = "Final installment settlement"
    });
    QVERIFY(p2.isOk());

    auto eAfterP2 = enrollUseCases.getEnrollment(enrollment.id).value();
    QCOMPARE(eAfterP2.paidPiastres, 300000);
    QCOMPARE(eAfterP2.balancePiastres, 0); // Fully settled!
}
