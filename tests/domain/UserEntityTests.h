#pragma once
#include <QtTest/QTest>

class UserEntityTests : public QObject {
    Q_OBJECT

private slots:
    void testLockoutAfterMaxFailedAttempts();
    void testLastSuperAdminDeactivationBlocked();
};
