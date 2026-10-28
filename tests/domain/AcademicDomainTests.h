#pragma once

#include <QtTest/QTest>

class AcademicDomainTests : public QObject {
    Q_OBJECT

private slots:
    void testStudentWorkflow();
    void testCourseAndGroupWorkflow();
    void testEnrollmentAndCapacityEnforcement();
    void testPaymentAndReceiptWorkflow();
};

