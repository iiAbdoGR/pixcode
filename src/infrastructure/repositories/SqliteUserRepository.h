#pragma once

#include "application/ports/IUserRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqliteUserRepository : public application::IUserRepository {
public:
    explicit SqliteUserRepository(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] std::optional<domain::User> findById(const std::string& id) override;
    [[nodiscard]] std::optional<domain::User> findByUsername(const std::string& username) override;
    [[nodiscard]] std::vector<domain::User> findAll() override;
    [[nodiscard]] int countActiveSuperAdmins() override;
    [[nodiscard]] int countTotalUsers() override;

    void save(const domain::User& user) override;
    void assignRole(const std::string& userId, const std::string& roleId) override;
    void removeRole(const std::string& userId, const std::string& roleId) override;

private:
    [[nodiscard]] domain::User mapRowToUser(const class QSqlQuery& query);
    void populateUserRoles(domain::User& user);

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure

