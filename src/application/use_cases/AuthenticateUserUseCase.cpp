#include "AuthenticateUserUseCase.h"
#include "domain/enums/AuditAction.h"

namespace pixcode::application {

AuthenticateUserUseCase::AuthenticateUserUseCase(
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

Result<LoginResultDto> AuthenticateUserUseCase::execute(const LoginCommand& cmd) {
    if (cmd.username.empty() || cmd.password.empty()) {
        return Result<LoginResultDto>::error(
            domain::DomainError(domain::ErrorCode::InvalidCredentials, "Username and password cannot be empty"));
    }

    const auto now = m_clock->nowUtcIso();
    auto userOpt = m_userRepo->findByUsername(cmd.username);

    if (!userOpt.has_value()) {
        m_auditService->record(std::nullopt,
                               std::string(domain::auditActionToString(domain::AuditAction::UserLoginFailed)),
                               "User", cmd.username, "Login failed: User not found");
        return Result<LoginResultDto>::error(
            domain::DomainError(domain::ErrorCode::InvalidCredentials, "Invalid username or password"));
    }

    auto user = std::move(*userOpt);

    // 1. Check account active
    if (!user.isActive()) {
        m_auditService->record(user.id(),
                               std::string(domain::auditActionToString(domain::AuditAction::UserLoginFailed)),
                               "User", user.id(), "Login rejected: Account is disabled");
        return Result<LoginResultDto>::error(
            domain::DomainError(domain::ErrorCode::AccountDisabled, "Account is disabled. Contact your administrator."));
    }

    // 2. Check lockout
    if (user.isLockedOut(now)) {
        m_auditService->record(user.id(),
                               std::string(domain::auditActionToString(domain::AuditAction::UserLoginFailed)),
                               "User", user.id(), "Login rejected: Account locked out until " + user.lockoutUntilUtc().value_or(""));
        return Result<LoginResultDto>::error(
            domain::DomainError(domain::ErrorCode::AccountLocked,
                                "Account is temporarily locked due to too many failed attempts. Try again later."));
    }

    // 3. Verify password
    const bool passwordValid = m_passwordHasher->verifyPassword(cmd.password, user.passwordHash());

    if (!passwordValid) {
        const auto lockoutExpiry = m_clock->addMinutesUtcIso(15);
        user.recordFailedLogin(now, lockoutExpiry);
        m_userRepo->save(user);

        if (user.isLockedOut(now)) {
            m_auditService->record(user.id(),
                                   std::string(domain::auditActionToString(domain::AuditAction::UserLockedOut)),
                                   "User", user.id(), "Account locked out after 5 consecutive failed login attempts");
            return Result<LoginResultDto>::error(
                domain::DomainError(domain::ErrorCode::AccountLocked, "Account has been locked due to consecutive failed attempts."));
        } else {
            m_auditService->record(user.id(),
                                   std::string(domain::auditActionToString(domain::AuditAction::UserLoginFailed)),
                                   "User", user.id(), "Login failed: Invalid password");
            return Result<LoginResultDto>::error(
                domain::DomainError(domain::ErrorCode::InvalidCredentials, "Invalid username or password"));
        }
    }

    // 4. Successful login
    user.recordSuccessfulLogin(now);
    m_userRepo->save(user);

    // Fetch user roles & permissions
    const auto roles = m_roleRepo->findRolesForUser(user.id());
    std::vector<std::string> roleCodes;
    std::vector<std::string> permissionCodes;

    for (const auto& r : roles) {
        roleCodes.push_back(r.code());
        for (const auto& p : r.permissions()) {
            permissionCodes.push_back(p.code());
        }
    }

    m_auditService->record(user.id(),
                           std::string(domain::auditActionToString(domain::AuditAction::UserLoginSuccess)),
                           "User", user.id(), "User logged in successfully");

    LoginResultDto dto{
        .userId = user.id(),
        .username = user.username(),
        .preferredLanguage = user.preferredLanguage(),
        .mustChangePassword = user.mustChangePassword(),
        .roles = std::move(roleCodes),
        .permissions = std::move(permissionCodes)
    };

    return Result<LoginResultDto>::ok(std::move(dto));
}

} // namespace pixcode::application

