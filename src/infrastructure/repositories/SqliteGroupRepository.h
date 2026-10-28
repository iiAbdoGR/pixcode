#pragma once

#include "application/ports/IGroupRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqliteGroupRepository : public application::IGroupRepository {
public:
    explicit SqliteGroupRepository(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] std::optional<domain::Group> findById(std::int64_t id) override;
    [[nodiscard]] std::vector<domain::Group> findAll(const application::GroupFilter& filter) override;
    [[nodiscard]] int count(const application::GroupFilter& filter) override;
    domain::Group save(const domain::Group& group) override;
    [[nodiscard]] int countActiveEnrollments(std::int64_t groupId) override;

private:
    [[nodiscard]] domain::Group mapRowToGroup(const class QSqlQuery& query);

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure

