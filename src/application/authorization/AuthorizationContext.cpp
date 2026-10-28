#include "AuthorizationContext.h"
#include "domain/errors/DomainError.h"

namespace pixcode::application {

AuthorizationContext::AuthorizationContext(domain::User currentUser)
    : m_currentUser(std::move(currentUser)) {}

void AuthorizationContext::setCurrentUser(std::optional<domain::User> user) {
    m_currentUser = std::move(user);
}

bool AuthorizationContext::isAuthenticated() const noexcept {
    return m_currentUser.has_value();
}

const std::optional<domain::User>& AuthorizationContext::currentUser() const noexcept {
    return m_currentUser;
}

bool AuthorizationContext::hasPermission(std::string_view permissionCode) const noexcept {
    if (!m_currentUser.has_value()) {
        return false;
    }
    return m_currentUser->hasPermission(permissionCode);
}

void AuthorizationContext::enforcePermission(std::string_view permissionCode) const {
    if (!hasPermission(permissionCode)) {
        throw domain::DomainError(domain::ErrorCode::PermissionDenied,
                                  "Access denied: missing required permission: " + std::string(permissionCode));
    }
}

} // namespace pixcode::application

