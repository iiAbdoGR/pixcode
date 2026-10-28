#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include "domain/entities/Course.h"

namespace pixcode::application {

struct CourseFilter {
    std::string searchText;
    std::optional<domain::CourseStatus> status;
    int page{0};
    int pageSize{50};
};

class ICourseRepository {
public:
    virtual ~ICourseRepository() = default;

    [[nodiscard]] virtual std::optional<domain::Course> findById(std::int64_t id) = 0;
    [[nodiscard]] virtual std::vector<domain::Course> findAll(const CourseFilter& filter) = 0;
    [[nodiscard]] virtual int count(const CourseFilter& filter) = 0;
    virtual domain::Course save(const domain::Course& course) = 0;
};

} // namespace pixcode::application
