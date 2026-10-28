#include "CourseUseCases.h"

namespace pixcode::application {

CourseUseCases::CourseUseCases(std::shared_ptr<ICourseRepository> repo)
    : m_repo(std::move(repo)) {}

Result<CourseDto> CourseUseCases::createCourse(const CreateCourseCommand& cmd) {
    if (auto err = domain::Course::validate(cmd.name); err.has_value()) {
        return Result<CourseDto>::error(*err);
    }
    if (cmd.defaultFeePiastres < 0) {
        return Result<CourseDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Fee cannot be negative."));
    }

    domain::Course c;
    c.name = cmd.name;
    c.description = cmd.description;
    c.durationWeeks = cmd.durationWeeks;
    c.defaultFee = domain::Money{cmd.defaultFeePiastres};
    c.status = domain::CourseStatus::Active;

    auto saved = m_repo->save(c);
    return Result<CourseDto>::ok(toDto(saved));
}

Result<CourseDto> CourseUseCases::updateCourse(const UpdateCourseCommand& cmd) {
    auto existing = m_repo->findById(cmd.id);
    if (!existing) {
        return Result<CourseDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Course not found."));
    }
    if (auto err = domain::Course::validate(cmd.name); err.has_value()) {
        return Result<CourseDto>::error(*err);
    }

    existing->name = cmd.name;
    existing->description = cmd.description;
    existing->durationWeeks = cmd.durationWeeks;
    existing->defaultFee = domain::Money{cmd.defaultFeePiastres};
    existing->status = (cmd.status == "inactive") ? domain::CourseStatus::Inactive : domain::CourseStatus::Active;

    auto saved = m_repo->save(*existing);
    return Result<CourseDto>::ok(toDto(saved));
}

Result<CourseDto> CourseUseCases::getCourse(std::int64_t id) {
    auto existing = m_repo->findById(id);
    if (!existing) {
        return Result<CourseDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Course not found."));
    }
    return Result<CourseDto>::ok(toDto(*existing));
}

Result<std::vector<CourseDto>> CourseUseCases::listCourses(const CourseFilter& filter) {
    auto courses = m_repo->findAll(filter);
    std::vector<CourseDto> result;
    result.reserve(courses.size());
    for (const auto& c : courses) {
        result.push_back(toDto(c));
    }
    return Result<std::vector<CourseDto>>::ok(std::move(result));
}

Result<int> CourseUseCases::countCourses(const CourseFilter& filter) {
    return Result<int>::ok(m_repo->count(filter));
}

CourseDto CourseUseCases::toDto(const domain::Course& c) {
    return CourseDto{
        .id = c.id,
        .name = c.name,
        .description = c.description,
        .durationWeeks = c.durationWeeks,
        .defaultFeePiastres = c.defaultFee.piastres(),
        .status = (c.status == domain::CourseStatus::Active) ? "active" : "inactive",
        .createdAtUtc = c.createdAtUtc
    };
}

} // namespace pixcode::application
