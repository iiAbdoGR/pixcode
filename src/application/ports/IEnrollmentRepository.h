#pragma once

#include <vector>
#include <optional>
#include <cstdint>
#include "domain/entities/Enrollment.h"

namespace pixcode::application {

class IEnrollmentRepository {
public:
    virtual ~IEnrollmentRepository() = default;

    [[nodiscard]] virtual std::optional<domain::Enrollment> findById(std::int64_t id) = 0;
    [[nodiscard]] virtual std::optional<domain::Enrollment> findActive(std::int64_t studentId, std::int64_t groupId) = 0;
    [[nodiscard]] virtual std::vector<domain::Enrollment> findByStudent(std::int64_t studentId) = 0;
    [[nodiscard]] virtual std::vector<domain::Enrollment> findByGroup(std::int64_t groupId) = 0;
    virtual domain::Enrollment save(const domain::Enrollment& enrollment) = 0;
};

} // namespace pixcode::application
