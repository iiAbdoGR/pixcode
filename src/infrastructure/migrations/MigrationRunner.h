#pragma once

#include <memory>
#include <string>
#include <vector>
#include <QString>
#include "database/DatabaseManager.h"

namespace pixcode::infrastructure {

struct MigrationDefinition {
    int version;
    std::string name;
    std::string sql;
};

class MigrationRunner {
public:
    explicit MigrationRunner(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] bool runMigrations(QString& outError);
    [[nodiscard]] int getCurrentVersion();

private:
    [[nodiscard]] std::string computeSha256(const std::string& content);
    [[nodiscard]] std::vector<MigrationDefinition> getBuiltinMigrations();

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure

