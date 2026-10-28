#include "AuthorizationTests.h"
#include "application/authorization/AuthorizationContext.h"
#include "domain/entities/User.h"
#include "domain/entities/Role.h"
#include "domain/entities/Permission.h"
#include "domain/errors/DomainError.h"

void AuthorizationTests::testSuperAdminInherentPermissions() {
    pixcode::domain::User superUser(
        "u1", "owner", "hash", true, false, 0, std::nullopt, "ar",
        "2026-10-28T00:00:00Z", "2026-10-28T00:00:00Z", std::nullopt
    );
    superUser.addRole(pixcode::domain::Role("r_super", "SuperAdmin", "role.superadmin", true));

    pixcode::application::AuthorizationContext ctx(superUser);

    QVERIFY(ctx.isAuthenticated());
    QVERIFY(ctx.hasPermission("finance.payments.reverse"));
    QVERIFY(ctx.hasPermission("payroll.manage"));
    QVERIFY(ctx.hasPermission("system.settings.update"));
}

void AuthorizationTests::testScopedRolePermissions() {
    pixcode::domain::User secretary(
        "u2", "reception", "hash", true, false, 0, std::nullopt, "ar",
        "2026-10-28T00:00:00Z", "2026-10-28T00:00:00Z", std::nullopt
    );

    pixcode::domain::Role secRole("r_sec", "Secretary", "role.secretary", false);
    secRole.addPermission(pixcode::domain::Permission("p1", "students.create", "perm.create", ""));
    secRole.addPermission(pixcode::domain::Permission("p2", "finance.payments.collect", "perm.collect", ""));
    secretary.addRole(secRole);

    pixcode::application::AuthorizationContext ctx(secretary);

    QVERIFY(ctx.hasPermission("students.create"));
    QVERIFY(ctx.hasPermission("finance.payments.collect"));

    QVERIFY(!ctx.hasPermission("finance.payments.reverse"));
    QVERIFY(!ctx.hasPermission("payroll.manage"));

    QVERIFY_EXCEPTION_THROWN(
        ctx.enforcePermission("finance.payments.reverse"),
        pixcode::domain::DomainError
    );
}
