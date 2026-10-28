#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "../errors/DomainError.h"

namespace pixcode::domain {

enum class StudentStatus {
    Active,
    Archived
};

struct Student {
    std::int64_t id{0};
    std::string studentCode;   // STU-00001
    std::string fullName;
    std::string phone;
    std::string guardianName;
    std::string guardianPhone;
    std::string address;
    std::string notes;
    StudentStatus status{StudentStatus::Active};
    std::string registeredAtUtc;
    std::string updatedAtUtc;

    static std::optional<DomainError> validate(const std::string& fullName, const std::string& phone) {
        if (fullName.empty()) {
            return DomainError(ErrorCode::ValidationFailed, "Student full name cannot be empty.");
        }
        if (fullName.length() > 200) {
            return DomainError(ErrorCode::ValidationFailed, "Student full name is too long.");
        }
        if (phone.length() > 50) {
            return DomainError(ErrorCode::ValidationFailed, "Phone number is too long.");
        }
        return std::nullopt;
    }
};

} // namespace pixcode::domain
