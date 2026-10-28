#pragma once

#include <string>
#include <vector>
#include <optional>

namespace pixcode::application {

struct UserDto {
    std::string id;
    std::string username;
    bool isActive;
    bool mustChangePassword;
    int failedLoginCount;
    std::optional<std::string> lockoutUntilUtc;
    std::string preferredLanguage;
    std::string createdAtUtc;
    std::optional<std::string> lastLoginAtUtc;
    std::vector<std::string> roleCodes;
};

struct CreateUserCommand {
    std::string username;
    std::string initialPassword;
    std::string preferredLanguage;
    std::vector<std::string> roleCodes;
};

} // namespace pixcode::application

