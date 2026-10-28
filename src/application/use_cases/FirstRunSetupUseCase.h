#pragma once

#include <memory>
#include "dto/AuthDtos.h"
#include "results/Result.h"
#include "ports/IUserRepository.h"
#include "ports/IRoleRepository.h"
#include "ports/IPasswordHasher.h"
#include "ports/IClock.h"
#include "AuditService.h"

namespace pixcode::application {

class FirstRunSetupUseCase {
public:
    FirstRunSetupUseCase(std::shared_ptr<IUserRepository> userRepo,
                         std::shared_ptr<IRoleRepository> roleRepo,
                         std::shared_ptr<IPasswordHasher> passwordHasher,
                         std::shared_ptr<IClock> clock,
                         std::shared_ptr<AuditService> auditService);

    [[nodiscard]] Result<std::string> execute(const FirstRunSetupCommand& cmd);
    [[nodiscard]] bool isFirstRunRequired();

private:
    std::shared_ptr<IUserRepository> m_userRepo;
    std::shared_ptr<IRoleRepository> m_roleRepo;
    std::shared_ptr<IPasswordHasher> m_passwordHasher;
    std::shared_ptr<IClock> m_clock;
    std::shared_ptr<AuditService> m_auditService;
};

} // namespace pixcode::application

