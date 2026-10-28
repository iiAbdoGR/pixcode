#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "../value_objects/Money.h"
#include "../errors/DomainError.h"

namespace pixcode::domain {

enum class InstructorStatus {
    Active,
    Inactive
};

struct Instructor {
    std::int64_t id{0};
    std::optional<std::string> userId;  // user ID UUID if linked to a system user
    std::string fullName;
    std::string phone;
    std::string specialization;
    Money hourlyRate{0};
    std::string notes;
    InstructorStatus status{InstructorStatus::Active};
    std::string createdAtUtc;
    std::string updatedAtUtc;

    static std::optional<DomainError> validate(const std::string& fullName) {
        if (fullName.empty()) {
            return DomainError(ErrorCode::ValidationFailed, "Instructor name cannot be empty.");
        }
        return std::nullopt;
    }
};

} // namespace pixcode::domain
