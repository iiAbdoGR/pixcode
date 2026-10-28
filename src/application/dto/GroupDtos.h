#pragma once

#include <string>
#include <cstdint>
#include <optional>

namespace pixcode::application {

struct CreateGroupCommand {
    std::int64_t courseId{0};
    std::optional<std::int64_t> instructorId;
    std::string name;
    std::string startDate;
    std::string endDate;
    std::string scheduleDescription;
    int capacity{0};
    std::int64_t feePiastres{0};
};

struct UpdateGroupCommand {
    std::int64_t id{0};
    std::optional<std::int64_t> instructorId;
    std::string name;
    std::string startDate;
    std::string endDate;
    std::string scheduleDescription;
    int capacity{0};
    std::int64_t feePiastres{0};
    std::string status;
};

struct GroupDto {
    std::int64_t id{0};
    std::int64_t courseId{0};
    std::string courseName;
    std::optional<std::int64_t> instructorId;
    std::string instructorName;
    std::string name;
    std::string startDate;
    std::string endDate;
    std::string scheduleDescription;
    int capacity{0};
    std::int64_t feePiastres{0};
    std::string status;
    int enrolledCount{0};
    std::string createdAtUtc;
};

} // namespace pixcode::application

