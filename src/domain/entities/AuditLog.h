#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace pixcode::domain {

class AuditLog {
public:
    AuditLog(std::int64_t id,
             std::string occurredAtUtc,
             std::optional<std::string> actorUserId,
             std::string action,
             std::string entityType,
             std::string entityId,
             std::string summary,
             std::optional<std::string> correlationId,
             std::optional<std::string> changesJson)
        : m_id(id),
          m_occurredAtUtc(std::move(occurredAtUtc)),
          m_actorUserId(std::move(actorUserId)),
          m_action(std::move(action)),
          m_entityType(std::move(entityType)),
          m_entityId(std::move(entityId)),
          m_summary(std::move(summary)),
          m_correlationId(std::move(correlationId)),
          m_changesJson(std::move(changesJson)) {}

    [[nodiscard]] std::int64_t id() const noexcept { return m_id; }
    [[nodiscard]] const std::string& occurredAtUtc() const noexcept { return m_occurredAtUtc; }
    [[nodiscard]] const std::optional<std::string>& actorUserId() const noexcept { return m_actorUserId; }
    [[nodiscard]] const std::string& action() const noexcept { return m_action; }
    [[nodiscard]] const std::string& entityType() const noexcept { return m_entityType; }
    [[nodiscard]] const std::string& entityId() const noexcept { return m_entityId; }
    [[nodiscard]] const std::string& summary() const noexcept { return m_summary; }
    [[nodiscard]] const std::optional<std::string>& correlationId() const noexcept { return m_correlationId; }
    [[nodiscard]] const std::optional<std::string>& changesJson() const noexcept { return m_changesJson; }

private:
    std::int64_t m_id;
    std::string m_occurredAtUtc;
    std::optional<std::string> m_actorUserId;
    std::string m_action;
    std::string m_entityType;
    std::string m_entityId;
    std::string m_summary;
    std::optional<std::string> m_correlationId;
    std::optional<std::string> m_changesJson;
};

} // namespace pixcode::domain

