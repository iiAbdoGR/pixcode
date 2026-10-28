#pragma once

#include <string>
#include <cstdint>

namespace pixcode::application {

struct CreateCourseCommand {
    std::string name;
    std::string description;
    int durationWeeks{0};
    std::int64_t defaultFeePiastres{0};
};

struct UpdateCourseCommand {
    std::int64_t id{0};
    std::string name;
    std::string description;
    int durationWeeks{0};
    std::int64_t defaultFeePiastres{0};
    std::string status; // "active" or "inactive"
};

struct CourseDto {
    std::int64_t id{0};
    std::string name;
    std::string description;
    int durationWeeks{0};
    std::int64_t defaultFeePiastres{0};
    std::string status;
    std::string createdAtUtc;
};

} // namespace pixcode::application
