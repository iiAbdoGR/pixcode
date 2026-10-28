#include "ChangePasswordUseCase.h"
#include "domain/enums/AuditAction.h"

namespace pixcode::application {

ChangePasswordUseCase::ChangePasswordUseCase(
    std::shared_ptr<IUserRepository> userRepo,
    std::shared_ptr<IPasswordHasher> passwordHasher,
    std::shared_ptr<IClock> clock,
    std::shared_ptr<AuditService> auditService)
    : m_userRepo(std::move(userRepo)),
      m_passwordHasher(std::move(passwordHasher)),
      m_clock(std::move(clock)),
      m_auditService(std::move(auditService)) {}

Result<void> ChangePasswordUseCase::execute(const ChangePasswordCommand& cmd) {
    if (cmd.newPassword != cmd.confirmNewPassword) {
        return Result<void>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "New passwords do not match"));
    }

    std::string failureReason;
    if (!m_passwordHasher->isPasswordStrong(cmd.newPassword, failureReason)) {
        return Result<void>::error(
            domain::DomainError(domain::ErrorCode::WeakPassword, failureReason));
    }

    auto userOpt = m_userRepo->findById(cmd.userId);
    if (!userOpt.has_value()) {
        return Result<void>::error(
            domain::DomainError(domain::ErrorCode::UserNotFound, "User not found"));
    }

    auto user = std::move(*userOpt);

    // Verify current password
    if (!m_passwordHasher->verifyPassword(cmd.currentPassword, user.passwordHash())) {
        m_auditService->record(user.id(),
                               std::string(domain::auditActionToString(domain::AuditAction::PasswordChanged)),
                               "User", user.id(), "Password change rejected: Incorrect current password");
        return Result<void>::error(
            domain::DomainError(domain::ErrorCode::InvalidCredentials, "Current password is incorrect"));
    }

    const auto now = m_clock->nowUtcIso();
    const auto newHash = m_passwordHasher->hashPassword(cmd.newPassword);
    user.updatePassword(newHash, now);
    m_userRepo->save(user);

    m_auditService->record(user.id(),
                           std::string(domain::auditActionToString(domain::AuditAction::PasswordChanged)),
                           "User", user.id(), "Password successfully updated");

    return Result<void>::ok();
}

} // namespace pixcode::application

