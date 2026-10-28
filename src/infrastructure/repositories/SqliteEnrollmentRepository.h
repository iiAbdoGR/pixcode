#pragma once

#include "application/ports/IEnrollmentRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqliteEnrollmentRepository : public application::IEnrollmentRepository {
public:
    explicit SqliteEnrollmentRepository(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] std::optional<domain::Enrollment> findById(std::int64_t id) override;
    [[nodiscard]] std::optional<domain::Enrollment> findActive(std::int64_t studentId, std::int64_t groupId) override;
    [[nodiscard]] std::vector<domain::Enrollment> findByStudent(std::int64_t studentId) override;
    [[nodiscard]] std::vector<domain::Enrollment> findByGroup(std::int64_t groupId) override;
    domain::Enrollment save(const domain::Enrollment& enrollment) override;

private:
    [[nodiscard]] domain::Enrollment mapRowToEnrollment(const class QSqlQuery& query);

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure

