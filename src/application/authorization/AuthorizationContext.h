#pragma once

#include "IAuthorizationContext.h"

namespace pixcode::application {

class AuthorizationContext : public IAuthorizationContext {
public:
    AuthorizationContext() = default;
    explicit AuthorizationContext(domain::User currentUser);

    void setCurrentUser(std::optional<domain::User> user);

    [[nodiscard]] bool isAuthenticated() const noexcept override;
    [[nodiscard]] const std::optional<domain::User>& currentUser() const noexcept override;
    [[nodiscard]] bool hasPermission(std::string_view permissionCode) const noexcept override;
    void enforcePermission(std::string_view permissionCode) const override;

private:
    std::optional<domain::User> m_currentUser;
};

} // namespace pixcode::application

