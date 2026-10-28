#pragma once

#include <string>
#include <string_view>
#include <stdexcept>

namespace pixcode::domain {

enum class ErrorCode {
    InvalidUsername,
    WeakPassword,
    UserNotFound,
    UserAlreadyExists,
    AccountLocked,
    AccountDisabled,
    PasswordChangeRequired,
    InvalidCredentials,
    CannotDeleteLastSuperAdmin,
    PermissionDenied,
    ConcurrencyConflict,
    DatabaseError,
    ValidationFailed,
    BusinessRuleViolation
};

class DomainError : public std::runtime_error {
public:
    DomainError(ErrorCode code, std::string message)
        : std::runtime_error(message), m_code(code), m_message(std::move(message)) {}

    [[nodiscard]] ErrorCode code() const noexcept { return m_code; }
    [[nodiscard]] const std::string& message() const noexcept { return m_message; }

private:
    ErrorCode m_code;
    std::string m_message;
};

} // namespace pixcode::domain

