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


void AuthorizationTests::testOwnerRoleHasAllPermissions() {
    pixcode::domain::User owner(
        "u_owner", "my_owner", "hash", true, false, 0, std::nullopt, "ar",
        "2026-10-28T00:00:00Z", "2026-10-28T00:00:00Z", std::nullopt
    );
    owner.addRole(pixcode::domain::Role("r_owner", "Owner", "role.owner", true));

    pixcode::application::AuthorizationContext ctx(owner);
    QVERIFY(ctx.isAuthenticated());
    QVERIFY(ctx.hasPermission("groups.manage"));
    QVERIFY(ctx.hasPermission("attendance.student.mark"));
    QVERIFY(ctx.hasPermission("system.users.manage"));
    QVERIFY(ctx.hasPermission("payroll.pay_instructor"));
}

void AuthorizationTests::testStaffProtectionForLastOwner() {
    pixcode::domain::User owner(
        "u_owner", "my_owner", "hash", true, false, 0, std::nullopt, "ar",
        "2026-10-28T00:00:00Z", "2026-10-28T00:00:00Z", std::nullopt
    );
    owner.addRole(pixcode::domain::Role("r_owner", "Owner", "role.owner", true));

    // Must throw when trying to deactivate last remaining Owner
    QVERIFY_EXCEPTION_THROWN(
        owner.setActive(false, "2026-10-28T00:00:00Z", true),
        pixcode::domain::DomainError
    );
}

void AuthorizationTests::testAdminRoleRestrictedFromFinanceAndApproval() {
    pixcode::domain::User adminUser(
        "u_admin", "admin_ops", "hash", true, false, 0, std::nullopt, "ar",
        "2026-10-29T00:00:00Z", "2026-10-29T00:00:00Z", std::nullopt
    );

    pixcode::domain::Role adminRole("role_admin", "Admin", "role.admin", true);
    adminRole.addPermission(pixcode::domain::Permission("p_stu_read", "students.read", "", ""));
    adminRole.addPermission(pixcode::domain::Permission("p_grp_mng", "groups.manage", "", ""));
    adminRole.addPermission(pixcode::domain::Permission("p_pay_inst", "payroll.pay_instructor", "", ""));
    adminUser.addRole(adminRole);

    pixcode::application::AuthorizationContext ctx(adminUser);

    // Admin can perform operational tasks and execute payments
    QVERIFY(ctx.hasPermission("students.read"));
    QVERIFY(ctx.hasPermission("groups.manage"));
    QVERIFY(ctx.hasPermission("payroll.pay_instructor"));

    // Admin cannot access finance or approve liabilities
    QVERIFY(!ctx.hasPermission("finance.payments.read"));
    QVERIFY(!ctx.hasPermission("finance.payments.collect"));
    QVERIFY(!ctx.hasPermission("payroll.calculate_approve"));

    QVERIFY_EXCEPTION_THROWN(
        ctx.enforcePermission("finance.payments.read"),
        pixcode::domain::DomainError
    );

    QVERIFY_EXCEPTION_THROWN(
        ctx.enforcePermission("payroll.calculate_approve"),
        pixcode::domain::DomainError
    );
}