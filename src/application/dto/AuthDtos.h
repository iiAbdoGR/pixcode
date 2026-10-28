#pragma once

#include <string>
#include <vector>
#include <optional>

namespace pixcode::application {

struct LoginCommand {
    std::string username;
    std::string password;
};

struct LoginResultDto {
    std::string userId;
    std::string username;
    std::string preferredLanguage;
    bool mustChangePassword;
    std::vector<std::string> roles;
    std::vector<std::string> permissions;
};

struct FirstRunSetupCommand {
    std::string username;
    std::string password;
    std::string confirmPassword;
    std::string preferredLanguage;
};

struct ChangePasswordCommand {
    std::string userId;
    std::string currentPassword;
    std::string newPassword;
    std::string confirmNewPassword;
};

} // namespace pixcode::application

