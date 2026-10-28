#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace pixcode::application {

struct CreateStaffCommand {
    std::string username;
    std::string password;
    std::string fullName;
    std::string phone;
    std::string preferredLanguage{"ar"};
    std::vector<std::string> roleCodes;
    std::optional<std::int64_t> linkedInstructorId;
    std::string specialization;
    std::int64_t hourlyRatePiastres{0};
};

struct UpdateStaffCommand {
    std::string userId;
    std::string fullName;
    std::string phone;
    std::vector<std::string> roleCodes;
    bool isActive{true};
    std::string preferredLanguage{"ar"};
    std::string specialization;
    std::int64_t hourlyRatePiastres{0};
};

struct ResetStaffPasswordCommand {
    std::string userId;
    std::string newPassword;
    bool mustChangePassword{true};
};

struct StaffDto {
    std::string id;
    std::string username;
    bool isActive{true};
    bool mustChangePassword{false};
    int failedLoginCount{0};
    std::optional<std::string> lockoutUntilUtc;
    std::string preferredLanguage{"ar"};
    std::string createdAtUtc;
    std::optional<std::string> lastLoginAtUtc;
    std::vector<std::string> roleCodes;
    std::optional<std::int64_t> linkedInstructorId;
    std::string linkedInstructorName;
};

} // namespace pixcode::application
