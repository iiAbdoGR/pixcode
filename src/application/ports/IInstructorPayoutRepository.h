#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include "domain/entities/InstructorPayout.h"

namespace pixcode::application {

class IInstructorPayoutRepository {
public:
    virtual ~IInstructorPayoutRepository() = default;

    [[nodiscard]] virtual std::optional<domain::InstructorPayout> findById(std::int64_t id) = 0;
    [[nodiscard]] virtual std::vector<domain::InstructorPayout> findByInstructor(std::int64_t instructorId) = 0;
    virtual domain::InstructorPayout save(const domain::InstructorPayout& payout) = 0;
    [[nodiscard]] virtual std::string nextReceiptNumber() = 0;
    [[nodiscard]] virtual std::int64_t totalPaidOut(std::int64_t instructorId) = 0;
    [[nodiscard]] virtual std::int64_t totalAllPayouts() = 0;
    virtual void saveAllocation(const domain::InstructorPayoutAllocation& alloc) = 0;
    [[nodiscard]] virtual std::vector<domain::InstructorPayoutAllocation> findAllocationsByPayout(std::int64_t payoutId) = 0;
};

} // namespace pixcode::application
