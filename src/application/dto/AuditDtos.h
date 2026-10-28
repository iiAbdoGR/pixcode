#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace pixcode::application {

struct AuditLogDto {
    std::int64_t id;
    std::string occurredAtUtc;
    std::optional<std::string> actorUserId;
    std::string actorUsername;
    std::string action;
    std::string entityType;
    std::string entityId;
    std::string summary;
    std::optional<std::string> correlationId;
    std::optional<std::string> changesJson;
};

struct RecordAuditCommand {
    std::optional<std::string> actorUserId;
    std::string action;
    std::string entityType;
    std::string entityId;
    std::string summary;
    std::optional<std::string> correlationId;
    std::optional<std::string> changesJson;
};

} // namespace pixcode::application

