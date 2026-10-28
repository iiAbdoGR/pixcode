#pragma once
#include <QtTest/QTest>

class PasswordHasherTests : public QObject {
    Q_OBJECT

private slots:
    void testHashAndVerification();
    void testPasswordStrengthValidation();
};

