#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "../value_objects/Money.h"
#include "../errors/DomainError.h"

namespace pixcode::domain {

enum class EnrollmentStatus {
    Active,
    Cancelled,
    Transferred
};

struct Enrollment {
    std::int64_t id{0};
    std::int64_t studentId{0};
    std::int64_t groupId{0};
    Money agreedFee{0};
    Money discount{0};
    Money netFee{0};
    EnrollmentStatus status{EnrollmentStatus::Active};
    std::string enrolledAtUtc;
    std::optional<std::string> cancelledAtUtc;
    std::string notes;

    static std::optional<DomainError> validate(std::int64_t studentId, std::int64_t groupId, Money agreedFee, Money discount) {
        if (studentId <= 0) {
            return DomainError(ErrorCode::ValidationFailed, "Valid student is required for enrollment.");
        }
        if (groupId <= 0) {
            return DomainError(ErrorCode::ValidationFailed, "Valid group is required for enrollment.");
        }
        if (agreedFee.piasters() < 0) {
            return DomainError(ErrorCode::ValidationFailed, "Agreed fee cannot be negative.");
        }
        if (discount.piasters() < 0) {
            return DomainError(ErrorCode::ValidationFailed, "Discount cannot be negative.");
        }
        if (discount.piasters() > agreedFee.piasters()) {
            return DomainError(ErrorCode::ValidationFailed, "Discount cannot exceed agreed fee.");
        }
        return std::nullopt;
    }
};

} // namespace pixcode::domain

