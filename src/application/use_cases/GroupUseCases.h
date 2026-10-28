#pragma once

#include <vector>
#include <string>
#include <memory>
#include "application/results/Result.h"
#include "application/dto/GroupDtos.h"
#include "application/ports/IGroupRepository.h"
#include "application/ports/ICourseRepository.h"
#include "application/ports/IInstructorRepository.h"
#include "domain/entities/Group.h"

namespace pixcode::application {

class GroupUseCases {
public:
    GroupUseCases(std::shared_ptr<IGroupRepository> groups,
                  std::shared_ptr<ICourseRepository> courses,
                  std::shared_ptr<IInstructorRepository> instructors);

    Result<GroupDto> createGroup(const CreateGroupCommand& cmd);
    Result<GroupDto> updateGroup(const UpdateGroupCommand& cmd);
    Result<GroupDto> getGroup(std::int64_t id);
    Result<std::vector<GroupDto>> listGroups(const GroupFilter& filter);
    Result<int> countGroups(const GroupFilter& filter);

private:
    std::shared_ptr<IGroupRepository> m_groups;
    std::shared_ptr<ICourseRepository> m_courses;
    std::shared_ptr<IInstructorRepository> m_instructors;

    GroupDto buildDto(const domain::Group& g);
};

} // namespace pixcode::application
