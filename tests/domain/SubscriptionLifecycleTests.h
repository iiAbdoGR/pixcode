#pragma once

#include <QtCore/QObject>
#include <QtTest/QTest>

class SubscriptionLifecycleTests : public QObject {
    Q_OBJECT

private slots:
    void testCancellationAndAuditFields();
    void testRestorationCapacityAndDuplicateCheck();
    void testReEnrollmentAllowedAfterCancellation();
    void testPaymentHistoryPreservation();
};

