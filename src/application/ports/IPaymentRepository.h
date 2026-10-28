#pragma once

#include <vector>
#include <optional>
#include <cstdint>
#include <string>
#include "domain/entities/Payment.h"

namespace pixcode::application {

class IPaymentRepository {
public:
    virtual ~IPaymentRepository() = default;

    [[nodiscard]] virtual std::optional<domain::Payment> findById(std::int64_t id) = 0;
    [[nodiscard]] virtual std::vector<domain::Payment> findByEnrollment(std::int64_t enrollmentId) = 0;
    virtual domain::Payment save(const domain::Payment& payment) = 0;
    [[nodiscard]] virtual std::string nextReceiptNumber() = 0;
    [[nodiscard]] virtual std::int64_t totalPaid(std::int64_t enrollmentId) = 0;
};

} // namespace pixcode::application
