#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include "domain/entities/SessionAttendance.h"

namespace pixcode::application {

class IAttendanceRepository {
public:
    virtual ~IAttendanceRepository() = default;

    [[nodiscard]] virtual std::optional<domain::SessionAttendance> findById(std::int64_t id) = 0;
    [[nodiscard]] virtual std::optional<domain::SessionAttendance> findBySessionAndStudent(std::int64_t sessionId, std::int64_t studentId) = 0;
    [[nodiscard]] virtual std::vector<domain::SessionAttendance> findBySession(std::int64_t sessionId) = 0;
    [[nodiscard]] virtual std::vector<domain::SessionAttendance> findByStudent(std::int64_t studentId) = 0;

    virtual domain::SessionAttendance save(const domain::SessionAttendance& attendance) = 0;
    // Atomic batch save in a single transaction
    virtual bool saveBatch(const std::vector<domain::SessionAttendance>& items) = 0;

    // Attendance stats
    [[nodiscard]] virtual int countStudentStatus(std::int64_t studentId, domain::AttendanceStatus status) = 0;
    [[nodiscard]] virtual int countEligibleSessions(std::int64_t studentId) = 0;
};

} // namespace pixcode::application
