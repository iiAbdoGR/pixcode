#pragma once

#include "application/ports/ISessionRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqliteSessionRepository : public application::ISessionRepository {
public:
    explicit SqliteSessionRepository(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] std::optional<domain::Session> findById(std::int64_t id) override;
    [[nodiscard]] std::vector<domain::Session> findAll(const application::SessionFilter& filter) override;
    [[nodiscard]] int count(const application::SessionFilter& filter) override;
    domain::Session save(const domain::Session& session) override;
    bool remove(std::int64_t id) override;

    [[nodiscard]] std::int64_t sumApprovedEarnings(std::int64_t instructorId, const std::string& fromDate = "", const std::string& toDate = "") override;
    [[nodiscard]] int sumApprovedMinutes(std::int64_t instructorId, const std::string& fromDate = "", const std::string& toDate = "") override;
    [[nodiscard]] int countLessonsTaught(std::int64_t instructorId, const std::string& fromDate = "", const std::string& toDate = "") override;

private:
    [[nodiscard]] domain::Session mapRowToSession(const class QSqlQuery& query);

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure
