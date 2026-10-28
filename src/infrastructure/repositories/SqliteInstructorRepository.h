#pragma once

#include "application/ports/IInstructorRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqliteInstructorRepository : public application::IInstructorRepository {
public:
    explicit SqliteInstructorRepository(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] std::optional<domain::Instructor> findById(std::int64_t id) override;
    [[nodiscard]] std::vector<domain::Instructor> findAll(const application::InstructorFilter& filter) override;
    domain::Instructor save(const domain::Instructor& instructor) override;

private:
    [[nodiscard]] domain::Instructor mapRowToInstructor(const class QSqlQuery& query);

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure

