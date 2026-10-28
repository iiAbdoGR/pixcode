#include "User.h"
#include <algorithm>

namespace pixcode::domain {

User::User(std::string id,
           std::string username,
           std::string passwordHash,
           bool isActive,
           bool mustChangePassword,
           int failedLoginCount,
           std::optional<std::string> lockoutUntilUtc,
           std::string preferredLanguage,
           std::string createdAtUtc,
           std::string updatedAtUtc,
           std::optional<std::string> lastLoginAtUtc)
    : m_id(std::move(id)),
      m_username(std::move(username)),
      m_passwordHash(std::move(passwordHash)),
      m_isActive(isActive),
      m_mustChangePassword(mustChangePassword),
      m_failedLoginCount(failedLoginCount),
      m_lockoutUntilUtc(std::move(lockoutUntilUtc)),
      m_preferredLanguage(std::move(preferredLanguage)),
      m_createdAtUtc(std::move(createdAtUtc)),
      m_updatedAtUtc(std::move(updatedAtUtc)),
      m_lastLoginAtUtc(std::move(lastLoginAtUtc)) {}

bool User::isSuperAdmin() const noexcept {
    for (const auto& r : m_roles) {
        if (r.code() == "SuperAdmin") return true;
    }
    return false;
}

bool User::hasPermission(std::string_view permissionCode) const noexcept {
    // SuperAdmin has all permissions by definition
    if (isSuperAdmin()) return true;

    for (const auto& r : m_roles) {
        if (r.hasPermission(permissionCode)) return true;
    }
    return false;
}

bool User::isLockedOut(std::string_view currentUtcTimeIso) const noexcept {
    if (!m_lockoutUntilUtc.has_value() || m_lockoutUntilUtc->empty()) {
        return false;
    }
    // Lexicographical comparison works cleanly for ISO-8601 strings (YYYY-MM-DDTHH:MM:SSZ)
    return currentUtcTimeIso < *m_lockoutUntilUtc;
}

void User::recordSuccessfulLogin(std::string currentUtcTimeIso) {
    m_failedLoginCount = 0;
    m_lockoutUntilUtc = std::nullopt;
    m_lastLoginAtUtc = currentUtcTimeIso;
    m_updatedAtUtc = std::move(currentUtcTimeIso);
}

void User::recordFailedLogin(std::string currentUtcTimeIso, std::string lockoutExpirationUtcIso) {
    m_failedLoginCount += 1;
    m_updatedAtUtc = std::move(currentUtcTimeIso);
    if (m_failedLoginCount >= MaxFailedLoginAttempts) {
        m_lockoutUntilUtc = std::move(lockoutExpirationUtcIso);
    }
}

void User::updatePassword(std::string newPasswordHash, std::string currentUtcTimeIso) {
    m_passwordHash = std::move(newPasswordHash);
    m_mustChangePassword = false;
    m_failedLoginCount = 0;
    m_lockoutUntilUtc = std::nullopt;
    m_updatedAtUtc = std::move(currentUtcTimeIso);
}

void User::setPreferredLanguage(std::string languageCode, std::string currentUtcTimeIso) {
    m_preferredLanguage = std::move(languageCode);
    m_updatedAtUtc = std::move(currentUtcTimeIso);
}

void User::setActive(bool active, std::string currentUtcTimeIso, bool isLastSuperAdmin) {
    if (!active && isLastSuperAdmin && isSuperAdmin()) {
        throw DomainError(ErrorCode::CannotDeleteLastSuperAdmin,
                          "Cannot deactivate the last remaining active SuperAdmin account");
    }
    m_isActive = active;
    m_updatedAtUtc = std::move(currentUtcTimeIso);
}

void User::unlock() {
    m_failedLoginCount = 0;
    m_lockoutUntilUtc = std::nullopt;
}

void User::resetPasswordByAdmin(std::string newPasswordHash, std::string currentUtcTimeIso) {
    m_passwordHash = std::move(newPasswordHash);
    m_mustChangePassword = true;
    m_failedLoginCount = 0;
    m_lockoutUntilUtc = std::nullopt;
    m_updatedAtUtc = std::move(currentUtcTimeIso);
}

} // namespace pixcode::domain

