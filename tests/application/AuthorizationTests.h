#pragma once
#include <QtTest/QTest>

class AuthorizationTests : public QObject {
    Q_OBJECT

private slots:
    void testSuperAdminInherentPermissions();
    void testScopedRolePermissions();
};
