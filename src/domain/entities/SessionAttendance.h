#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "../errors/DomainError.h"

namespace pixcode::domain {

enum class AttendanceStatus {
    Present,
    Absent,
    Late,
    Excused
};

struct SessionAttendance {
    std::int64_t id{0};
    std::int64_t sessionId{0};
    std::int64_t studentId{0};
    AttendanceStatus status{AttendanceStatus::Present};
    std::string arrivalTime;
    std::string notes;
    std::optional<std::string> markedBy;
    std::string markedAtUtc;
    std::string updatedAtUtc;

    static std::optional<DomainError> validate(std::int64_t sessionId, std::int64_t studentId) {
        if (sessionId <= 0) {
            return DomainError(ErrorCode::ValidationFailed, "Valid session is required for attendance.");
        }
        if (studentId <= 0) {
            return DomainError(ErrorCode::ValidationFailed, "Valid student is required for attendance.");
        }
        return std::nullopt;
    }
};

} // namespace pixcode::domain

