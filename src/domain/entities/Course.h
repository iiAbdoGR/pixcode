#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "../value_objects/Money.h"
#include "../errors/DomainError.h"

namespace pixcode::domain {

enum class CourseStatus {
    Active,
    Inactive
};

struct Course {
    std::int64_t id{0};
    std::string name;
    std::string description;
    int durationWeeks{0};
    Money defaultFee{0};
    CourseStatus status{CourseStatus::Active};
    std::string createdAtUtc;
    std::string updatedAtUtc;

    static std::optional<DomainError> validate(const std::string& name) {
        if (name.empty()) {
            return DomainError(ErrorCode::ValidationFailed, "Course name cannot be empty.");
        }
        if (name.length() > 200) {
            return DomainError(ErrorCode::ValidationFailed, "Course name is too long.");
        }
        return std::nullopt;
    }
};

} // namespace pixcode::domain

