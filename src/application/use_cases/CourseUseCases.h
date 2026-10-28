#pragma once

#include <vector>
#include <string>
#include <memory>
#include "application/results/Result.h"
#include "application/dto/CourseDtos.h"
#include "application/ports/ICourseRepository.h"
#include "domain/entities/Course.h"

namespace pixcode::application {

class CourseUseCases {
public:
    explicit CourseUseCases(std::shared_ptr<ICourseRepository> repo);

    Result<CourseDto> createCourse(const CreateCourseCommand& cmd);
    Result<CourseDto> updateCourse(const UpdateCourseCommand& cmd);
    Result<CourseDto> getCourse(std::int64_t id);
    Result<std::vector<CourseDto>> listCourses(const CourseFilter& filter);
    Result<int> countCourses(const CourseFilter& filter);

private:
    std::shared_ptr<ICourseRepository> m_repo;
    static CourseDto toDto(const domain::Course& c);
};

} // namespace pixcode::application
