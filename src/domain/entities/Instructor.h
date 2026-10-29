#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <optional>
#include "../value_objects/Money.h"
#include "../errors/DomainError.h"

namespace pixcode::domain {

enum class InstructorStatus {
    Active,
    Inactive
};

enum class CompensationType {
    Hourly,
    PerSession,
    MonthlySalary,
    FixedContract
};

[[nodiscard]] inline std::string compensationTypeToString(CompensationType type) {
    switch (type) {
        case CompensationType::PerSession: return "per_session";
        case CompensationType::MonthlySalary: return "monthly_salary";
        case CompensationType::FixedContract: return "fixed_contract";
        default: return "hourly";
    }
}

[[nodiscard]] inline CompensationType compensationTypeFromString(std::string_view str) {
    if (str == "per_session") return CompensationType::PerSession;
    if (str == "monthly_salary" || str == "monthly_fixed") return CompensationType::MonthlySalary;
    if (str == "fixed_contract") return CompensationType::FixedContract;
    return CompensationType::Hourly;
}

struct Instructor {
    std::int64_t id{0};
    std::string fullName;
    std::string phone;
    std::string specialization;
    CompensationType compensationType{CompensationType::Hourly};
    Money baseRate{0};
    Money hourlyRate{0}; // Preserved alongside baseRate
    Money monthlySalary{0};
    std::string contractStartDate;
    std::string contractEndDate;
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
