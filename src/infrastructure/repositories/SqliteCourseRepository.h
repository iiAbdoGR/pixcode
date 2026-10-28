#pragma once

#include "application/ports/ICourseRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqliteCourseRepository : public application::ICourseRepository {
public:
    explicit SqliteCourseRepository(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] std::optional<domain::Course> findById(std::int64_t id) override;
    [[nodiscard]] std::vector<domain::Course> findAll(const application::CourseFilter& filter) override;
    [[nodiscard]] int count(const application::CourseFilter& filter) override;
    domain::Course save(const domain::Course& course) override;

private:
    [[nodiscard]] domain::Course mapRowToCourse(const class QSqlQuery& query);

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure

