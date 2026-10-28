#pragma once

#include <string_view>

namespace pixcode::domain {

enum class AuditAction {
    UserLoginSuccess,
    UserLoginFailed,
    UserLockedOut,
    UserCreated,
    UserUpdated,
    UserDisabled,
    PasswordChanged,
    RoleAssigned,
    SystemInitialized,
    BackupCreated,
    BackupRestored
};

constexpr std::string_view auditActionToString(AuditAction action) noexcept {
    switch (action) {
        case AuditAction::UserLoginSuccess: return "USER_LOGIN_SUCCESS";
        case AuditAction::UserLoginFailed: return "USER_LOGIN_FAILED";
        case AuditAction::UserLockedOut: return "USER_LOCKED_OUT";
        case AuditAction::UserCreated: return "USER_CREATED";
        case AuditAction::UserUpdated: return "USER_UPDATED";
        case AuditAction::UserDisabled: return "USER_DISABLED";
        case AuditAction::PasswordChanged: return "PASSWORD_CHANGED";
        case AuditAction::RoleAssigned: return "ROLE_ASSIGNED";
        case AuditAction::SystemInitialized: return "SYSTEM_INITIALIZED";
        case AuditAction::BackupCreated: return "BACKUP_CREATED";
        case AuditAction::BackupRestored: return "BACKUP_RESTORED";
    }
    return "UNKNOWN";
}

} // namespace pixcode::domain

