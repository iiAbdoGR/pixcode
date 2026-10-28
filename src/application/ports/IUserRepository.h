#pragma once

#include <string>
#include <vector>
#include <optional>
#include "domain/entities/User.h"

namespace pixcode::application {

class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    [[nodiscard]] virtual std::optional<domain::User> findById(const std::string& id) = 0;
    [[nodiscard]] virtual std::optional<domain::User> findByUsername(const std::string& username) = 0;
    [[nodiscard]] virtual std::vector<domain::User> findAll() = 0;
    [[nodiscard]] virtual int countActiveSuperAdmins() = 0;
    [[nodiscard]] virtual int countTotalUsers() = 0;

    virtual void save(const domain::User& user) = 0;
    virtual void assignRole(const std::string& userId, const std::string& roleId) = 0;
    virtual void removeRole(const std::string& userId, const std::string& roleId) = 0;
};

} // namespace pixcode::application

