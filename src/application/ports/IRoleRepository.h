#pragma once

#include <string>
#include <vector>
#include <optional>
#include "domain/entities/Role.h"

namespace pixcode::application {

class IRoleRepository {
public:
    virtual ~IRoleRepository() = default;

    [[nodiscard]] virtual std::optional<domain::Role> findById(const std::string& id) = 0;
    [[nodiscard]] virtual std::optional<domain::Role> findByCode(const std::string& code) = 0;
    [[nodiscard]] virtual std::vector<domain::Role> findAll() = 0;
    [[nodiscard]] virtual std::vector<domain::Role> findRolesForUser(const std::string& userId) = 0;
};

} // namespace pixcode::application

