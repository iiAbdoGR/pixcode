#pragma once

#include <memory>
#include <string>
#include <optional>
#include "ports/IAuditLogRepository.h"
#include "ports/IClock.h"

namespace pixcode::application {

class AuditService {
public:
    AuditService(std::shared_ptr<IAuditLogRepository> repo, std::shared_ptr<IClock> clock);

    void record(std::optional<std::string> actorUserId,
                std::string action,
                std::string entityType,
                std::string entityId,
                std::string summary,
                std::optional<std::string> correlationId = std::nullopt,
                std::optional<std::string> changesJson = std::nullopt);

private:
    std::shared_ptr<IAuditLogRepository> m_repo;
    std::shared_ptr<IClock> m_clock;
};

} // namespace pixcode::application

