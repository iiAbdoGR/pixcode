#pragma once

#include <QtCore/QObject>
#include <QtTest/QTest>

class AttendanceAndPayrollTests : public QObject {
    Q_OBJECT

private slots:
    void testSessionEarningsCalculation();
    void testSessionApprovalStatus();
    void testInstructorRateHistoryTracking();
    void testAttendanceBatchRecordingAndStats();
    void testAttendanceUniqueConstraint();
    void testPayoutReceiptNumberingAndBalance();
    void testSessionSchedulingTimeValidationAndRescheduling();
    void testInstructorCreationAcrossAllCompensationTypes();
    void testDuplicateSessionApprovalBlocked();
    void testPayoutAllocationAndSettlement();
};
