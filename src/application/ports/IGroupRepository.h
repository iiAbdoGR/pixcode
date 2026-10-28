#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include "domain/entities/Group.h"

namespace pixcode::application {

struct GroupFilter {
    std::string searchText;
    std::int64_t courseId{0};
    std::optional<domain::GroupStatus> status;
    int page{0};
    int pageSize{50};
};

class IGroupRepository {
public:
    virtual ~IGroupRepository() = default;

    [[nodiscard]] virtual std::optional<domain::Group> findById(std::int64_t id) = 0;
    [[nodiscard]] virtual std::vector<domain::Group> findAll(const GroupFilter& filter) = 0;
    [[nodiscard]] virtual int count(const GroupFilter& filter) = 0;
    virtual domain::Group save(const domain::Group& group) = 0;
    [[nodiscard]] virtual int countActiveEnrollments(std::int64_t groupId) = 0;
};

} // namespace pixcode::application

