#pragma once
#include <QtTest/QTest>

class AuditLogTests : public QObject {
    Q_OBJECT

private slots:
    void testAuditLogAppendAndQuery();
};
