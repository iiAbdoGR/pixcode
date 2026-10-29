#pragma once

#include <QtCore/QObject>
#include <QtTest/QTest>

class DashboardAuthorizationTests : public QObject {
    Q_OBJECT

private slots:
    void testOwnerReceivesFullFinancialSummary();
    void testAdminReceivesOperationalOnlyStrictlyOmittingFinancials();
    void testInstructorEarningsCompensationModes();
};

