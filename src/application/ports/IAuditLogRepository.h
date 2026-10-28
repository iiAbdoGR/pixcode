#pragma once

#include <vector>
#include <string>
#include "domain/entities/AuditLog.h"
#include "dto/AuditDtos.h"

namespace pixcode::application {

class IAuditLogRepository {
public:
    virtual ~IAuditLogRepository() = default;

    virtual void append(const RecordAuditCommand& cmd, const std::string& occurredAtUtc) = 0;
    [[nodiscard]] virtual std::vector<AuditLogDto> findRecent(int limit, int offset = 0) = 0;
};

} // namespace pixcode::application

