#pragma once

#include <string_view>
#include <optional>
#include "domain/entities/User.h"

namespace pixcode::application {

class IAuthorizationContext {
public:
    virtual ~IAuthorizationContext() = default;

    [[nodiscard]] virtual bool isAuthenticated() const noexcept = 0;
    [[nodiscard]] virtual const std::optional<domain::User>& currentUser() const noexcept = 0;
    [[nodiscard]] virtual bool hasPermission(std::string_view permissionCode) const noexcept = 0;
    virtual void enforcePermission(std::string_view permissionCode) const = 0;
};

} // namespace pixcode::application

