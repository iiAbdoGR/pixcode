#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "../value_objects/Money.h"
#include "../errors/DomainError.h"

namespace pixcode::domain {

enum class GroupStatus {
    Open,
    Full,
    Closed,
    Cancelled
};

struct Group {
    std::int64_t id{0};
    std::int64_t courseId{0};
    std::optional<std::int64_t> instructorId;
    std::string name;
    std::string startDate;
    std::string endDate;
    std::string scheduleDescription;
    int capacity{0};
    Money fee{0}; // fee snapshot for this group
    GroupStatus status{GroupStatus::Open};
    std::string createdAtUtc;
    std::string updatedAtUtc;

    static std::optional<DomainError> validate(const std::string& name, std::int64_t courseId) {
        if (name.empty()) {
            return DomainError(ErrorCode::ValidationFailed, "Group name cannot be empty.");
        }
        if (courseId <= 0) {
            return DomainError(ErrorCode::ValidationFailed, "Valid course selection is required.");
        }
        return std::nullopt;
    }
};

} // namespace pixcode::domain

