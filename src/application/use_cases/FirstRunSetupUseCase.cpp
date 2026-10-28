#include "FirstRunSetupUseCase.h"
#include "domain/enums/AuditAction.h"
#include <regex>

namespace pixcode::application {

FirstRunSetupUseCase::FirstRunSetupUseCase(
    std::shared_ptr<IUserRepository> userRepo,
    std::shared_ptr<IRoleRepository> roleRepo,
    std::shared_ptr<IPasswordHasher> passwordHasher,
    std::shared_ptr<IClock> clock,
    std::shared_ptr<AuditService> auditService)
    : m_userRepo(std::move(userRepo)),
      m_roleRepo(std::move(roleRepo)),
      m_passwordHasher(std::move(passwordHasher)),
      m_clock(std::move(clock)),
      m_auditService(std::move(auditService)) {}

bool FirstRunSetupUseCase::isFirstRunRequired() {
    return m_userRepo->countTotalUsers() == 0;
}

Result<std::string> FirstRunSetupUseCase::execute(const FirstRunSetupCommand& cmd) {
    if (!isFirstRunRequired()) {
        return Result<std::string>::error(
            domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "First-run setup has already been completed"));
    }

    if (cmd.username.length() < 3) {
        return Result<std::string>::error(
            domain::DomainError(domain::ErrorCode::InvalidUsername, "Username must be at least 3 characters long"));
    }

    if (cmd.password != cmd.confirmPassword) {
        return Result<std::string>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Passwords do not match"));
    }

    std::string failureReason;
    if (!m_passwordHasher->isPasswordStrong(cmd.password, failureReason)) {
        return Result<std::string>::error(
            domain::DomainError(domain::ErrorCode::WeakPassword, failureReason));
    }

    const auto superAdminRoleOpt = m_roleRepo->findByCode("SuperAdmin");
    if (!superAdminRoleOpt.has_value()) {
        return Result<std::string>::error(
            domain::DomainError(domain::ErrorCode::DatabaseError, "System role SuperAdmin does not exist in database"));
    }

    const auto now = m_clock->nowUtcIso();
    const auto passwordHash = m_passwordHasher->hashPassword(cmd.password);
    const std::string newUserId = "user_superadmin_01"; // Or generated UUID

    domain::User superAdmin(
        newUserId,
        cmd.username,
        passwordHash,
        true,   // isActive
        false,  // mustChangePassword
        0,      // failedLoginCount
        std::nullopt,
        cmd.preferredLanguage.empty() ? "ar" : cmd.preferredLanguage,
        now,
        now,
        std::nullopt
    );

    m_userRepo->save(superAdmin);
    m_userRepo->assignRole(newUserId, superAdminRoleOpt->id());

    m_auditService->record(newUserId,
                           std::string(domain::auditActionToString(domain::AuditAction::SystemInitialized)),
                           "System", newUserId, "Initial SuperAdmin created and system initialized");

    return Result<std::string>::ok(newUserId);
}

} // namespace pixcode::application

