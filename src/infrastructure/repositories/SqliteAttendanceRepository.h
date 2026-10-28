#pragma once

#include "application/ports/IAttendanceRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqliteAttendanceRepository : public application::IAttendanceRepository {
public:
    explicit SqliteAttendanceRepository(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] std::optional<domain::SessionAttendance> findById(std::int64_t id) override;
    [[nodiscard]] std::optional<domain::SessionAttendance> findBySessionAndStudent(std::int64_t sessionId, std::int64_t studentId) override;
    [[nodiscard]] std::vector<domain::SessionAttendance> findBySession(std::int64_t sessionId) override;
    [[nodiscard]] std::vector<domain::SessionAttendance> findByStudent(std::int64_t studentId) override;

    domain::SessionAttendance save(const domain::SessionAttendance& attendance) override;
    bool saveBatch(const std::vector<domain::SessionAttendance>& items) override;

    [[nodiscard]] int countStudentStatus(std::int64_t studentId, domain::AttendanceStatus status) override;
    [[nodiscard]] int countEligibleSessions(std::int64_t studentId) override;

private:
    [[nodiscard]] domain::SessionAttendance mapRowToAttendance(const class QSqlQuery& query);

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure
