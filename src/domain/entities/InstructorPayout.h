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

} // namespace pixcode::domain
