#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "../value_objects/Money.h"
#include "../errors/DomainError.h"

namespace pixcode::domain {

enum class PaymentMethod {
    Cash,
    BankTransfer,
    Other
};

struct Payment {
    std::int64_t id{0};
    std::int64_t enrollmentId{0};
    Money amount{0};
    PaymentMethod method{PaymentMethod::Cash};
    std::string paidAtUtc;
    std::string receiptNumber;
    std::string notes;
    std::optional<std::string> recordedBy;
    std::string createdAtUtc;

    static std::optional<DomainError> validate(std::int64_t enrollmentId, Money amount) {
        if (enrollmentId <= 0) {
            return DomainError(ErrorCode::ValidationFailed, "Valid enrollment is required for payment.");
        }
        if (amount.piasters() <= 0) {
            return DomainError(ErrorCode::ValidationFailed, "Payment amount must be strictly positive.");
        }
        return std::nullopt;
    }
};

} // namespace pixcode::domain
