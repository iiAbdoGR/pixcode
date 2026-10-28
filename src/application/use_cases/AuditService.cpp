#include "AuditService.h"

namespace pixcode::application {

AuditService::AuditService(std::shared_ptr<IAuditLogRepository> repo, std::shared_ptr<IClock> clock)
    : m_repo(std::move(repo)), m_clock(std::move(clock)) {}

void AuditService::record(std::optional<std::string> actorUserId,
                          std::string action,
                          std::string entityType,
                          std::string entityId,
                          std::string summary,
                          std::optional<std::string> correlationId,
                          std::optional<std::string> changesJson) {
    if (!m_repo || !m_clock) return;

    RecordAuditCommand cmd{
        .actorUserId = std::move(actorUserId),
        .action = std::move(action),
        .entityType = std::move(entityType),
        .entityId = std::move(entityId),
        .summary = std::move(summary),
        .correlationId = std::move(correlationId),
        .changesJson = std::move(changesJson)
    };

    m_repo->append(cmd, m_clock->nowUtcIso());
}

} // namespace pixcode::application

