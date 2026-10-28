#include "SqliteAuditLogRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

namespace pixcode::infrastructure {

SqliteAuditLogRepository::SqliteAuditLogRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

void SqliteAuditLogRepository::append(const application::RecordAuditCommand& cmd, const std::string& occurredAtUtc) {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);

    query.prepare(
        "INSERT INTO audit_logs (occurred_at_utc, actor_user_id, action, entity_type, "
        "                        entity_id, summary, correlation_id, changes_json) "
        "VALUES (:occurred, :actor, :action, :type, :id, :summary, :corr, :changes);"
    );

    query.bindValue(":occurred", QString::fromStdString(occurredAtUtc));

    if (cmd.actorUserId.has_value()) {
        query.bindValue(":actor", QString::fromStdString(*cmd.actorUserId));
    } else {
        query.bindValue(":actor", QVariant(QMetaType(QMetaType::QString)));
    }

    query.bindValue(":action", QString::fromStdString(cmd.action));
    query.bindValue(":type", QString::fromStdString(cmd.entityType));
    query.bindValue(":id", QString::fromStdString(cmd.entityId));
    query.bindValue(":summary", QString::fromStdString(cmd.summary));

    if (cmd.correlationId.has_value()) {
        query.bindValue(":corr", QString::fromStdString(*cmd.correlationId));
    } else {
        query.bindValue(":corr", QVariant(QMetaType(QMetaType::QString)));
    }

    if (cmd.changesJson.has_value()) {
        query.bindValue(":changes", QString::fromStdString(*cmd.changesJson));
    } else {
        query.bindValue(":changes", QVariant(QMetaType(QMetaType::QString)));
    }

    query.exec();
}

std::vector<application::AuditLogDto> SqliteAuditLogRepository::findRecent(int limit, int offset) {
    auto db = m_dbManager->getConnection();
    QSqlQuery query(db);
    std::vector<application::AuditLogDto> logs;

    query.prepare(
        "SELECT a.id, a.occurred_at_utc, a.actor_user_id, u.username as actor_username, "
        "       a.action, a.entity_type, a.entity_id, a.summary, a.correlation_id, a.changes_json "
        "FROM audit_logs a "
        "LEFT JOIN users u ON a.actor_user_id = u.id "
        "ORDER BY a.occurred_at_utc DESC "
        "LIMIT :limit OFFSET :offset;"
    );
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);

    if (!query.exec()) {
        return logs;
    }

    while (query.next()) {
        application::AuditLogDto dto;
        dto.id = query.value("id").toLongLong();
        dto.occurredAtUtc = query.value("occurred_at_utc").toString().toStdString();

        if (!query.value("actor_user_id").isNull()) {
            dto.actorUserId = query.value("actor_user_id").toString().toStdString();
        }
        dto.actorUsername = query.value("actor_username").isNull()
            ? "System" : query.value("actor_username").toString().toStdString();

        dto.action = query.value("action").toString().toStdString();
        dto.entityType = query.value("entity_type").toString().toStdString();
        dto.entityId = query.value("entity_id").toString().toStdString();
        dto.summary = query.value("summary").toString().toStdString();

        if (!query.value("correlation_id").isNull()) {
            dto.correlationId = query.value("correlation_id").toString().toStdString();
        }
        if (!query.value("changes_json").isNull()) {
            dto.changesJson = query.value("changes_json").toString().toStdString();
        }

        logs.push_back(std::move(dto));
    }

    return logs;
}

} // namespace pixcode::infrastructure

