#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include "Role.h"
#include "../errors/DomainError.h"

namespace pixcode::domain {

class User {
public:
    static constexpr int MaxFailedLoginAttempts = 5;
    static constexpr std::chrono::minutes LockoutDuration{15};

    User(std::string id,
         std::string username,
         std::string passwordHash,
         bool isActive,
         bool mustChangePassword,
         int failedLoginCount,
         std::optional<std::string> lockoutUntilUtc,
         std::string preferredLanguage,
         std::string createdAtUtc,
         std::string updatedAtUtc,
         std::optional<std::string> lastLoginAtUtc);

    [[nodiscard]] const std::string& id() const noexcept { return m_id; }
    [[nodiscard]] const std::string& username() const noexcept { return m_username; }
    [[nodiscard]] const std::string& passwordHash() const noexcept { return m_passwordHash; }
    [[nodiscard]] bool isActive() const noexcept { return m_isActive; }
    [[nodiscard]] bool mustChangePassword() const noexcept { return m_mustChangePassword; }
    [[nodiscard]] int failedLoginCount() const noexcept { return m_failedLoginCount; }
    [[nodiscard]] const std::optional<std::string>& lockoutUntilUtc() const noexcept { return m_lockoutUntilUtc; }
    [[nodiscard]] const std::string& preferredLanguage() const noexcept { return m_preferredLanguage; }
    [[nodiscard]] const std::string& createdAtUtc() const noexcept { return m_createdAtUtc; }
    [[nodiscard]] const std::string& updatedAtUtc() const noexcept { return m_updatedAtUtc; }
    [[nodiscard]] const std::optional<std::string>& lastLoginAtUtc() const noexcept { return m_lastLoginAtUtc; }

    [[nodiscard]] const std::vector<Role>& roles() const noexcept { return m_roles; }
    void addRole(Role role) { m_roles.push_back(std::move(role)); }
    void clearRoles() noexcept { m_roles.clear(); }

    [[nodiscard]] bool isSuperAdmin() const noexcept;
    [[nodiscard]] bool hasPermission(std::string_view permissionCode) const noexcept;
    [[nodiscard]] bool isLockedOut(std::string_view currentUtcTimeIso) const noexcept;

    void recordSuccessfulLogin(std::string currentUtcTimeIso);
    void recordFailedLogin(std::string currentUtcTimeIso, std::string lockoutExpirationUtcIso);
    void updatePassword(std::string newPasswordHash, std::string currentUtcTimeIso);
    void setPreferredLanguage(std::string languageCode, std::string currentUtcTimeIso);
    void setActive(bool active, std::string currentUtcTimeIso, bool isLastSuperAdmin);
    void unlock();

private:
    std::string m_id;
    std::string m_username;
    std::string m_passwordHash;
    bool m_isActive;
    bool m_mustChangePassword;
    int m_failedLoginCount;
    std::optional<std::string> m_lockoutUntilUtc;
    std::string m_preferredLanguage;
    std::string m_createdAtUtc;
    std::string m_updatedAtUtc;
    std::optional<std::string> m_lastLoginAtUtc;
    std::vector<Role> m_roles;
};

} // namespace pixcode::domain

