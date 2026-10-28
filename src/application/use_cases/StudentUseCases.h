#pragma once

#include <vector>
#include <string>
#include <memory>
#include "application/results/Result.h"
#include "application/dto/StudentDtos.h"
#include "application/ports/IStudentRepository.h"
#include "application/authorization/IAuthorizationContext.h"
#include "domain/entities/Student.h"

namespace pixcode::application {

class StudentUseCases {
public:
    StudentUseCases(std::shared_ptr<IStudentRepository> repo,
                    std::shared_ptr<IAuthorizationContext> auth);

    Result<StudentDto> createStudent(const CreateStudentCommand& cmd);
    Result<StudentDto> updateStudent(const UpdateStudentCommand& cmd);
    Result<StudentDto> archiveStudent(std::int64_t id);
    Result<StudentDto> activateStudent(std::int64_t id);
    Result<StudentDto> getStudent(std::int64_t id);
    Result<std::vector<StudentSummaryDto>> listStudents(const StudentFilter& filter);
    Result<int> countStudents(const StudentFilter& filter);

private:
    std::shared_ptr<IStudentRepository> m_repo;
    std::shared_ptr<IAuthorizationContext> m_auth;

    static StudentDto toDto(const domain::Student& s);
    static StudentSummaryDto toSummaryDto(const domain::Student& s);
};

} // namespace pixcode::application

