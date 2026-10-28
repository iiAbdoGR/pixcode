#pragma once

#include <string>
#include <cstdint>
#include "domain/entities/Student.h"

namespace pixcode::application {

struct CreateStudentCommand {
    std::string fullName;
    std::string phone;
    std::string guardianName;
    std::string guardianPhone;
    std::string address;
    std::string notes;
};

struct UpdateStudentCommand {
    std::int64_t id{0};
    std::string fullName;
    std::string phone;
    std::string guardianName;
    std::string guardianPhone;
    std::string address;
    std::string notes;
};

struct StudentDto {
    std::int64_t id{0};
    std::string studentCode;
    std::string fullName;
    std::string phone;
    std::string guardianName;
    std::string guardianPhone;
    std::string address;
    std::string notes;
    std::string status; // "active" or "archived"
    std::string registeredAtUtc;
};

struct StudentSummaryDto {
    std::int64_t id{0};
    std::string studentCode;
    std::string fullName;
    std::string phone;
    std::string status;
};

} // namespace pixcode::application
