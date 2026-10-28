#pragma once

#include <string>
#include <vector>
#include "Permission.h"

namespace pixcode::domain {

class Role {
public:
    Role(std::string id, std::string code, std::string displayKey, bool isSystemRole)
        : m_id(std::move(id)), m_code(std::move(code)), m_displayKey(std::move(displayKey)), m_isSystemRole(isSystemRole) {}

    [[nodiscard]] const std::string& id() const noexcept { return m_id; }
    [[nodiscard]] const std::string& code() const noexcept { return m_code; }
    [[nodiscard]] const std::string& displayKey() const noexcept { return m_displayKey; }
    [[nodiscard]] bool isSystemRole() const noexcept { return m_isSystemRole; }

    [[nodiscard]] const std::vector<Permission>& permissions() const noexcept { return m_permissions; }
    void addPermission(Permission permission) { m_permissions.push_back(std::move(permission)); }

    [[nodiscard]] bool hasPermission(std::string_view permissionCode) const noexcept {
        for (const auto& p : m_permissions) {
            if (p.code() == permissionCode) return true;
        }
        return false;
    }

private:
    std::string m_id;
    std::string m_code;
    std::string m_displayKey;
    bool m_isSystemRole;
    std::vector<Permission> m_permissions;
};

} // namespace pixcode::domain

