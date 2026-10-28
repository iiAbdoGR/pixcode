#include "PasswordHasherTests.h"
#include "infrastructure/security/Argon2idPasswordHasher.h"

void PasswordHasherTests::testHashAndVerification() {
    pixcode::infrastructure::Argon2idPasswordHasher hasher;
    const std::string plaintext = "SecureAdmin123!";

    const std::string hash = hasher.hashPassword(plaintext);
    QVERIFY(!hash.empty());
    QVERIFY(hash.rfind("$argon2id$", 0) == 0);

    QVERIFY(hasher.verifyPassword(plaintext, hash));
    QVERIFY(!hasher.verifyPassword("WrongPassword!", hash));
    QVERIFY(!hasher.verifyPassword("", hash));
}

void PasswordHasherTests::testPasswordStrengthValidation() {
    pixcode::infrastructure::Argon2idPasswordHasher hasher;
    std::string reason;

    QVERIFY(!hasher.isPasswordStrong("Short1A", reason));
    QVERIFY(!hasher.isPasswordStrong("alllowercase123", reason));
    QVERIFY(!hasher.isPasswordStrong("NoNumbersPassword", reason));
    QVERIFY(hasher.isPasswordStrong("ValidPassword123", reason));
}
