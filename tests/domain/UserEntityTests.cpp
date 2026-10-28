#include "UserEntityTests.h"
#include "domain/entities/User.h"
#include "domain/errors/DomainError.h"

void UserEntityTests::testLockoutAfterMaxFailedAttempts() {
    pixcode::domain::User user(
        "u1", "admin", "hash", true, false, 0, std::nullopt, "ar",
        "2026-10-28T00:00:00Z", "2026-10-28T00:00:00Z", std::nullopt
    );

    const std::string now = "2026-10-28T12:00:00Z";
    const std::string expiry = "2026-10-28T12:15:00Z";

    QVERIFY(!user.isLockedOut(now));

    for (int i = 0; i < 4; ++i) {
        user.recordFailedLogin(now, expiry);
        QVERIFY(!user.isLockedOut(now));
    }

    user.recordFailedLogin(now, expiry);
    QCOMPARE(user.failedLoginCount(), 5);
    QVERIFY(user.isLockedOut(now));

    const std::string afterExpiry = "2026-10-28T12:16:00Z";
    QVERIFY(!user.isLockedOut(afterExpiry));

    user.recordSuccessfulLogin(afterExpiry);
    QCOMPARE(user.failedLoginCount(), 0);
    QVERIFY(!user.isLockedOut(now));
}

void UserEntityTests::testLastSuperAdminDeactivationBlocked() {
    pixcode::domain::User superAdmin(
        "u_super", "superadmin", "hash", true, false, 0, std::nullopt, "ar",
        "2026-10-28T00:00:00Z", "2026-10-28T00:00:00Z", std::nullopt
    );
    superAdmin.addRole(pixcode::domain::Role("r1", "SuperAdmin", "role.superadmin", true));

    const std::string now = "2026-10-28T12:00:00Z";
    const bool isLastSuperAdmin = true;

    QVERIFY_EXCEPTION_THROWN(
        superAdmin.setActive(false, now, isLastSuperAdmin),
        pixcode::domain::DomainError
    );

    superAdmin.setActive(false, now, false);
    QVERIFY(!superAdmin.isActive());
}
