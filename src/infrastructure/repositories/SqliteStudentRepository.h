#pragma once

#include "application/ports/IStudentRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqliteStudentRepository : public application::IStudentRepository {
public:
    explicit SqliteStudentRepository(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] std::optional<domain::Student> findById(std::int64_t id) override;
    [[nodiscard]] std::optional<domain::Student> findByCode(const std::string& code) override;
    [[nodiscard]] std::vector<domain::Student> findAll(const application::StudentFilter& filter) override;
    [[nodiscard]] int count(const application::StudentFilter& filter) override;
    domain::Student save(const domain::Student& student) override;
    bool remove(std::int64_t id) override;
    [[nodiscard]] std::string nextStudentCode() override;

private:
    [[nodiscard]] domain::Student mapRowToStudent(const class QSqlQuery& query);

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure

