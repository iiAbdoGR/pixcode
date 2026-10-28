#pragma once

#include "application/ports/IAuditLogRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqliteAuditLogRepository : public application::IAuditLogRepository {
public:
    explicit SqliteAuditLogRepository(std::shared_ptr<DatabaseManager> dbManager);

    void append(const application::RecordAuditCommand& cmd, const std::string& occurredAtUtc) override;
    [[nodiscard]] std::vector<application::AuditLogDto> findRecent(int limit, int offset = 0) override;

private:
    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure

