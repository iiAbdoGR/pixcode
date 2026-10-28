#pragma once

#include <string_view>

namespace pixcode::domain {

enum class SystemRole {
    SuperAdmin,
    Admin,
    Accountant,
    Secretary,
    Instructor
};

constexpr std::string_view roleToString(SystemRole role) noexcept {
    switch (role) {
        case SystemRole::SuperAdmin: return "SuperAdmin";
        case SystemRole::Admin: return "Admin";
        case SystemRole::Accountant: return "Accountant";
        case SystemRole::Secretary: return "Secretary";
        case SystemRole::Instructor: return "Instructor";
    }
    return "Unknown";
}

} // namespace pixcode::domain

