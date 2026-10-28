#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include "domain/entities/Instructor.h"

namespace pixcode::application {

struct InstructorFilter {
    std::string searchText;
    std::optional<domain::InstructorStatus> status;
};

class IInstructorRepository {
public:
    virtual ~IInstructorRepository() = default;

    [[nodiscard]] virtual std::optional<domain::Instructor> findById(std::int64_t id) = 0;
    [[nodiscard]] virtual std::vector<domain::Instructor> findAll(const InstructorFilter& filter) = 0;
    virtual domain::Instructor save(const domain::Instructor& instructor) = 0;
};

} // namespace pixcode::application
