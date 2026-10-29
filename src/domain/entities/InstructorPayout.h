#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "../value_objects/Money.h"
#include "../errors/DomainError.h"

namespace pixcode::domain {

enum class PayoutMethod {
    Cash,
    BankTransfer,
    Other
};

struct InstructorPayout {
    std::int64_t id{0};
    std::int64_t instructorId{0};
    Money amount{0};
    std::string paymentDateUtc;
    PayoutMethod paymentMethod{PayoutMethod::Cash};
    std::string referenceNumber;
    std::string receiptNumber;
    std::string notes;
    std::optional<std::string> createdBy;
    std::optional<std::string> approvedBy;
    std::string createdAtUtc;

    static std::optional<DomainError> validate(std::int64_t instructorId, Money amount) {
        if (instructorId <= 0) {
            return DomainError(ErrorCode::ValidationFailed, "Valid instructor is required for payout.");
        }
        if (amount.piasters() <= 0) {
            return DomainError(ErrorCode::ValidationFailed, "Payout amount must be positive.");
        }
        return std::nullopt;
    }

    [[nodiscard]] std::string paymentMethodToString() const {
        switch (paymentMethod) {
            case PayoutMethod::BankTransfer: return "bank_transfer";
            case PayoutMethod::Other: return "other";
            default: return "cash";
        }
    }
};

struct InstructorPayoutAllocation {
    std::int64_t id{0};
    std::int64_t payoutId{0};
    std::optional<std::int64_t> sessionId;
    std::optional<std::int64_t> salaryPeriodId;
    Money allocatedAmount{0};
    std::string createdAtUtc;
};

struct InstructorSalaryPeriod {
    std::int64_t id{0};
    std::int64_t instructorId{0};
    std::string periodMonthUtc; // YYYY-MM
    Money salaryAmount{0};
    std::string status{"pending"}; // "pending", "approved", "paid"
    std::optional<std::string> approvedAtUtc;
    std::optional<std::string> approvedByUserId;
    std::string createdAtUtc;
};

} // namespace pixcode::domain
