#pragma once

#include "application/ports/IRoleRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqliteRoleRepository : public application::IRoleRepository {
public:
    explicit SqliteRoleRepository(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] std::optional<domain::Role> findById(const std::string& id) override;
    [[nodiscard]] std::optional<domain::Role> findByCode(const std::string& code) override;
    [[nodiscard]] std::vector<domain::Role> findAll() override;
    [[nodiscard]] std::vector<domain::Role> findRolesForUser(const std::string& userId) override;

private:
    void populateRolePermissions(domain::Role& role);

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure

