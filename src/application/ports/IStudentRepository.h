#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include "domain/entities/Student.h"

namespace pixcode::application {

struct StudentFilter {
    std::string searchText;
    std::optional<domain::StudentStatus> status;
    int page{0};
    int pageSize{50};
};

class IStudentRepository {
public:
    virtual ~IStudentRepository() = default;

    [[nodiscard]] virtual std::optional<domain::Student> findById(std::int64_t id) = 0;
    [[nodiscard]] virtual std::optional<domain::Student> findByCode(const std::string& code) = 0;
    [[nodiscard]] virtual std::vector<domain::Student> findAll(const StudentFilter& filter) = 0;
    [[nodiscard]] virtual int count(const StudentFilter& filter) = 0;
    virtual domain::Student save(const domain::Student& student) = 0;
    virtual bool remove(std::int64_t id) = 0;
    [[nodiscard]] virtual std::string nextStudentCode() = 0;
};

} // namespace pixcode::application
