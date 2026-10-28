#include "GroupUseCases.h"

namespace pixcode::application {

static domain::GroupStatus statusFromString(const std::string& s) {
    if (s == "full") return domain::GroupStatus::Full;
    if (s == "closed") return domain::GroupStatus::Closed;
    if (s == "cancelled") return domain::GroupStatus::Cancelled;
    return domain::GroupStatus::Open;
}

static std::string statusToString(domain::GroupStatus s) {
    switch (s) {
        case domain::GroupStatus::Full: return "full";
        case domain::GroupStatus::Closed: return "closed";
        case domain::GroupStatus::Cancelled: return "cancelled";
        default: return "open";
    }
}

GroupUseCases::GroupUseCases(std::shared_ptr<IGroupRepository> groups,
                             std::shared_ptr<ICourseRepository> courses,
                             std::shared_ptr<IInstructorRepository> instructors)
    : m_groups(std::move(groups)), m_courses(std::move(courses)), m_instructors(std::move(instructors)) {}

Result<GroupDto> GroupUseCases::createGroup(const CreateGroupCommand& cmd) {
    if (auto err = domain::Group::validate(cmd.name, cmd.courseId); err.has_value()) {
        return Result<GroupDto>::error(*err);
    }

    auto course = m_courses->findById(cmd.courseId);
    if (!course) {
        return Result<GroupDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Course not found."));
    }

    std::int64_t fee = (cmd.feePiastres > 0) ? cmd.feePiastres : course->defaultFee.piastres();

    domain::Group g;
    g.courseId = cmd.courseId;
    g.instructorId = cmd.instructorId;
    g.name = cmd.name;
    g.startDate = cmd.startDate;
    g.endDate = cmd.endDate;
    g.scheduleDescription = cmd.scheduleDescription;
    g.capacity = cmd.capacity;
    g.fee = domain::Money{fee};
    g.status = domain::GroupStatus::Open;

    auto saved = m_groups->save(g);
    return Result<GroupDto>::ok(buildDto(saved));
}

Result<GroupDto> GroupUseCases::updateGroup(const UpdateGroupCommand& cmd) {
    auto existing = m_groups->findById(cmd.id);
    if (!existing) {
        return Result<GroupDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Group not found."));
    }
    if (cmd.name.empty()) {
        return Result<GroupDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Group name cannot be empty."));
    }

    existing->instructorId = cmd.instructorId;
    existing->name = cmd.name;
    existing->startDate = cmd.startDate;
    existing->endDate = cmd.endDate;
    existing->scheduleDescription = cmd.scheduleDescription;
    existing->capacity = cmd.capacity;
    existing->fee = domain::Money{cmd.feePiastres};
    existing->status = statusFromString(cmd.status);

    auto saved = m_groups->save(*existing);
    return Result<GroupDto>::ok(buildDto(saved));
}

Result<GroupDto> GroupUseCases::getGroup(std::int64_t id) {
    auto g = m_groups->findById(id);
    if (!g) {
        return Result<GroupDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Group not found."));
    }
    return Result<GroupDto>::ok(buildDto(*g));
}

Result<std::vector<GroupDto>> GroupUseCases::listGroups(const GroupFilter& filter) {
    auto groups = m_groups->findAll(filter);
    std::vector<GroupDto> result;
    result.reserve(groups.size());
    for (const auto& g : groups) {
        result.push_back(buildDto(g));
    }
    return Result<std::vector<GroupDto>>::ok(std::move(result));
}

Result<int> GroupUseCases::countGroups(const GroupFilter& filter) {
    return Result<int>::ok(m_groups->count(filter));
}

GroupDto GroupUseCases::buildDto(const domain::Group& g) {
    GroupDto dto;
    dto.id = g.id;
    dto.courseId = g.courseId;
    dto.instructorId = g.instructorId;
    dto.name = g.name;
    dto.startDate = g.startDate;
    dto.endDate = g.endDate;
    dto.scheduleDescription = g.scheduleDescription;
    dto.capacity = g.capacity;
    dto.feePiastres = g.fee.piastres();
    dto.status = statusToString(g.status);
    dto.enrolledCount = m_groups->countActiveEnrollments(g.id);
    dto.createdAtUtc = g.createdAtUtc;

    auto course = m_courses->findById(g.courseId);
    if (course) {
        dto.courseName = course->name;
    }
    if (g.instructorId.has_value() && *g.instructorId > 0) {
        auto inst = m_instructors->findById(*g.instructorId);
        if (inst) {
            dto.instructorName = inst->fullName;
        }
    }
    return dto;
}

} // namespace pixcode::application

