#pragma once
#include <QtTest/QTest>

class MoneyTests : public QObject {
    Q_OBJECT

private slots:
    void testBasicArithmetic();
    void testInstallmentRemainderPolicy();
};

