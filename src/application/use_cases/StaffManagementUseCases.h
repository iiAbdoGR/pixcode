#pragma once

#include <vector>
#include <string>
#include <memory>
#include "application/results/Result.h"
#include "application/dto/StaffDtos.h"
#include "application/ports/IUserRepository.h"
#include "application/ports/IRoleRepository.h"
#include "application/ports/IInstructorRepository.h"
#include "application/ports/IPasswordHasher.h"
#include "application/ports/IClock.h"
#include "application/authorization/IAuthorizationContext.h"
#include "application/use_cases/AuditService.h"

namespace pixcode::application {

class StaffManagementUseCases {
public:
    StaffManagementUseCases(std::shared_ptr<IUserRepository> userRepo,
                            std::shared_ptr<IRoleRepository> roleRepo,
                            std::shared_ptr<IInstructorRepository> instructorRepo,
                            std::shared_ptr<IPasswordHasher> hasher,
                            std::shared_ptr<IClock> clock,
                            std::shared_ptr<IAuthorizationContext> authContext,
                            std::shared_ptr<AuditService> auditService);

    Result<StaffDto> createStaff(const CreateStaffCommand& cmd);
    Result<StaffDto> updateStaff(const UpdateStaffCommand& cmd);
    Result<void> resetPassword(const ResetStaffPasswordCommand& cmd);
    Result<std::vector<StaffDto>> listStaff();
    Result<StaffDto> getStaff(const std::string& userId);

private:
    std::shared_ptr<IUserRepository> m_userRepo;
    std::shared_ptr<IRoleRepository> m_roleRepo;
    std::shared_ptr<IInstructorRepository> m_instructorRepo;
    std::shared_ptr<IPasswordHasher> m_hasher;
    std::shared_ptr<IClock> m_clock;
    std::shared_ptr<IAuthorizationContext> m_authContext;
    std::shared_ptr<AuditService> m_auditService;

    StaffDto buildStaffDto(const domain::User& user);
};

} // namespace pixcode::application
