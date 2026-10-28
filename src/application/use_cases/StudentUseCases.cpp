#include "StudentUseCases.h"

namespace pixcode::application {

StudentUseCases::StudentUseCases(std::shared_ptr<IStudentRepository> repo,
                                 std::shared_ptr<IAuthorizationContext> auth)
    : m_repo(std::move(repo)), m_auth(std::move(auth)) {}

Result<StudentDto> StudentUseCases::createStudent(const CreateStudentCommand& cmd) {
    if (auto err = domain::Student::validate(cmd.fullName, cmd.phone); err.has_value()) {
        return Result<StudentDto>::error(*err);
    }

    auto code = m_repo->nextStudentCode();

    domain::Student s;
    s.studentCode = code;
    s.fullName = cmd.fullName;
    s.phone = cmd.phone;
    s.guardianName = cmd.guardianName;
    s.guardianPhone = cmd.guardianPhone;
    s.address = cmd.address;
    s.notes = cmd.notes;
    s.status = domain::StudentStatus::Active;

    auto saved = m_repo->save(s);
    return Result<StudentDto>::ok(toDto(saved));
}

Result<StudentDto> StudentUseCases::updateStudent(const UpdateStudentCommand& cmd) {
    auto existing = m_repo->findById(cmd.id);
    if (!existing) {
        return Result<StudentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Student not found."));
    }

    if (auto err = domain::Student::validate(cmd.fullName, cmd.phone); err.has_value()) {
        return Result<StudentDto>::error(*err);
    }

    existing->fullName = cmd.fullName;
    existing->phone = cmd.phone;
    existing->guardianName = cmd.guardianName;
    existing->guardianPhone = cmd.guardianPhone;
    existing->address = cmd.address;
    existing->notes = cmd.notes;

    auto saved = m_repo->save(*existing);
    return Result<StudentDto>::ok(toDto(saved));
}

Result<StudentDto> StudentUseCases::archiveStudent(std::int64_t id) {
    auto existing = m_repo->findById(id);
    if (!existing) {
        return Result<StudentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Student not found."));
    }

    existing->status = domain::StudentStatus::Archived;
    auto saved = m_repo->save(*existing);
    return Result<StudentDto>::ok(toDto(saved));
}

Result<StudentDto> StudentUseCases::activateStudent(std::int64_t id) {
    auto existing = m_repo->findById(id);
    if (!existing) {
        return Result<StudentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Student not found."));
    }

    existing->status = domain::StudentStatus::Active;
    auto saved = m_repo->save(*existing);
    return Result<StudentDto>::ok(toDto(saved));
}

Result<StudentDto> StudentUseCases::getStudent(std::int64_t id) {
    auto existing = m_repo->findById(id);
    if (!existing) {
        return Result<StudentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Student not found."));
    }
    return Result<StudentDto>::ok(toDto(*existing));
}

Result<std::vector<StudentSummaryDto>> StudentUseCases::listStudents(const StudentFilter& filter) {
    auto students = m_repo->findAll(filter);
    std::vector<StudentSummaryDto> summaries;
    summaries.reserve(students.size());
    for (const auto& s : students) {
        summaries.push_back(toSummaryDto(s));
    }
    return Result<std::vector<StudentSummaryDto>>::ok(std::move(summaries));
}

Result<int> StudentUseCases::countStudents(const StudentFilter& filter) {
    return Result<int>::ok(m_repo->count(filter));
}

StudentDto StudentUseCases::toDto(const domain::Student& s) {
    return StudentDto{
        .id = s.id,
        .studentCode = s.studentCode,
        .fullName = s.fullName,
        .phone = s.phone,
        .guardianName = s.guardianName,
        .guardianPhone = s.guardianPhone,
        .address = s.address,
        .notes = s.notes,
        .status = (s.status == domain::StudentStatus::Active) ? "active" : "archived",
        .registeredAtUtc = s.registeredAtUtc
    };
}

StudentSummaryDto StudentUseCases::toSummaryDto(const domain::Student& s) {
    return StudentSummaryDto{
        .id = s.id,
        .studentCode = s.studentCode,
        .fullName = s.fullName,
        .phone = s.phone,
        .status = (s.status == domain::StudentStatus::Active) ? "active" : "archived"
    };
}

} // namespace pixcode::application
