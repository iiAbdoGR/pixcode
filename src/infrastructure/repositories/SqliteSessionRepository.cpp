#include "SqliteSessionRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>
#include <QtCore/QMap>

namespace pixcode::infrastructure {

SqliteSessionRepository::SqliteSessionRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

domain::Session SqliteSessionRepository::mapRowToSession(const QSqlQuery& query) {
    domain::Session s;
    s.id = query.value("id").toLongLong();
    s.groupId = query.value("group_id").toLongLong();
    if (!query.value("instructor_id").isNull()) {
        s.instructorId = query.value("instructor_id").toLongLong();
    }
    s.sessionDate = query.value("session_date").toString().toStdString();
    s.scheduledStartTime = query.value("scheduled_start_time").toString().toStdString();
    s.scheduledEndTime = query.value("scheduled_end_time").toString().toStdString();
    s.actualStartTime = query.value("actual_start_time").toString().toStdString();
    s.actualEndTime = query.value("actual_end_time").toString().toStdString();
    s.plannedDurationMinutes = query.value("planned_duration_minutes").toInt();
    s.actualDurationMinutes = query.value("actual_duration_minutes").toInt();
    s.payableDurationMinutes = query.value("payable_duration_minutes").toInt();

    QString st = query.value("status").toString();
    if (st == "completed") s.status = domain::SessionStatus::Completed;
    else if (st == "cancelled") s.status = domain::SessionStatus::Cancelled;
    else if (st == "rescheduled") s.status = domain::SessionStatus::Rescheduled;
    else s.status = domain::SessionStatus::Scheduled;

    QString appSt = query.value("approval_status").toString();
    if (appSt == "approved") s.approvalStatus = domain::SessionApprovalStatus::Approved;
    else if (appSt == "rejected") s.approvalStatus = domain::SessionApprovalStatus::Rejected;
    else s.approvalStatus = domain::SessionApprovalStatus::Pending;

    QString settlSt = query.value("settlement_status").toString();
    s.settlementStatus = domain::Session::settlementStatusFromString(settlSt.toStdString());

    if (!query.value("approved_by").isNull()) {
        s.approvedBy = query.value("approved_by").toString().toStdString();
    }
    if (!query.value("approved_at_utc").isNull()) {
        s.approvedAtUtc = query.value("approved_at_utc").toString().toStdString();
    }

    s.hourlyRateSnapshot = domain::Money(query.value("hourly_rate_snapshot_piastres").toLongLong());
    s.earnedAmount = domain::Money(query.value("earned_amount_piastres").toLongLong());
    s.notes = query.value("notes").toString().toStdString();
    s.createdAtUtc = query.value("created_at_utc").toString().toStdString();
    s.updatedAtUtc = query.value("updated_at_utc").toString().toStdString();

    return s;
}

std::optional<domain::Session> SqliteSessionRepository::findById(std::int64_t id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM sessions WHERE id = :id;");
    q.bindValue(":id", static_cast<qint64>(id));
    if (q.exec() && q.next()) {
        return mapRowToSession(q);
    }
    return std::nullopt;
}

std::vector<domain::Session> SqliteSessionRepository::findAll(const application::SessionFilter& filter) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT * FROM sessions WHERE 1=1";
    QMap<QString, QVariant> bindings;

    if (filter.groupId.has_value() && *filter.groupId > 0) {
        sql += " AND group_id = :gid";
        bindings[":gid"] = static_cast<qint64>(*filter.groupId);
    }
    if (filter.instructorId.has_value() && *filter.instructorId > 0) {
        sql += " AND instructor_id = :iid";
        bindings[":iid"] = static_cast<qint64>(*filter.instructorId);
    }
    if (!filter.fromDate.empty()) {
        sql += " AND session_date >= :fdate";
        bindings[":fdate"] = QString::fromStdString(filter.fromDate);
    }
    if (!filter.toDate.empty()) {
        sql += " AND session_date <= :tdate";
        bindings[":tdate"] = QString::fromStdString(filter.toDate);
    }
    if (filter.status.has_value()) {
        QString stStr = "scheduled";
        switch (*filter.status) {
            case domain::SessionStatus::Completed: stStr = "completed"; break;
            case domain::SessionStatus::Cancelled: stStr = "cancelled"; break;
            case domain::SessionStatus::Rescheduled: stStr = "rescheduled"; break;
            default: stStr = "scheduled"; break;
        }
        sql += " AND status = :st";
        bindings[":st"] = stStr;
    }
    if (filter.approvalStatus.has_value()) {
        QString appStr = "pending";
        switch (*filter.approvalStatus) {
            case domain::SessionApprovalStatus::Approved: appStr = "approved"; break;
            case domain::SessionApprovalStatus::Rejected: appStr = "rejected"; break;
            default: appStr = "pending"; break;
        }
        sql += " AND approval_status = :app";
        bindings[":app"] = appStr;
    }

    sql += " ORDER BY session_date DESC, scheduled_start_time DESC";
    if (filter.pageSize > 0) {
        sql += QString(" LIMIT %1 OFFSET %2").arg(filter.pageSize).arg(filter.page * filter.pageSize);
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) {
        q.bindValue(it.key(), it.value());
    }

    std::vector<domain::Session> list;
    if (q.exec()) {
        while (q.next()) {
            list.push_back(mapRowToSession(q));
        }
    }
    return list;
}

int SqliteSessionRepository::count(const application::SessionFilter& filter) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT COUNT(*) FROM sessions WHERE 1=1";
    QMap<QString, QVariant> bindings;

    if (filter.groupId.has_value() && *filter.groupId > 0) {
        sql += " AND group_id = :gid";
        bindings[":gid"] = static_cast<qint64>(*filter.groupId);
    }
    if (filter.instructorId.has_value() && *filter.instructorId > 0) {
        sql += " AND instructor_id = :iid";
        bindings[":iid"] = static_cast<qint64>(*filter.instructorId);
    }
    if (!filter.fromDate.empty()) {
        sql += " AND session_date >= :fdate";
        bindings[":fdate"] = QString::fromStdString(filter.fromDate);
    }
    if (!filter.toDate.empty()) {
        sql += " AND session_date <= :tdate";
        bindings[":tdate"] = QString::fromStdString(filter.toDate);
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) {
        q.bindValue(it.key(), it.value());
    }

    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

domain::Session SqliteSessionRepository::save(const domain::Session& session) {
    auto db = m_dbManager->getConnection();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QString stStr = "scheduled";
    switch (session.status) {
        case domain::SessionStatus::Completed: stStr = "completed"; break;
        case domain::SessionStatus::Cancelled: stStr = "cancelled"; break;
        case domain::SessionStatus::Rescheduled: stStr = "rescheduled"; break;
        default: stStr = "scheduled"; break;
    }

    QString appStr = "pending";
    switch (session.approvalStatus) {
        case domain::SessionApprovalStatus::Approved: appStr = "approved"; break;
        case domain::SessionApprovalStatus::Rejected: appStr = "rejected"; break;
        default: appStr = "pending"; break;
    }

    if (session.id == 0) {
        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO sessions (
                group_id, instructor_id, session_date, scheduled_start_time, scheduled_end_time,
                actual_start_time, actual_end_time, planned_duration_minutes, actual_duration_minutes,
                payable_duration_minutes, status, approval_status, settlement_status, approved_by, approved_at_utc,
                hourly_rate_snapshot_piastres, earned_amount_piastres, notes, created_at_utc, updated_at_utc
            ) VALUES (
                :gid, :iid, :sdate, :sstart, :send,
                :astart, :aend, :pdur, :adur,
                :paydur, :st, :appst, :settlst, :appby, :appat,
                :rate, :earned, :notes, :created, :updated
            );
        )");
        q.bindValue(":gid", static_cast<qint64>(session.groupId));
        if (session.instructorId.has_value() && *session.instructorId > 0) {
            q.bindValue(":iid", static_cast<qint64>(*session.instructorId));
        } else {
            q.bindValue(":iid", QVariant(QMetaType(QMetaType::LongLong)));
        }
        q.bindValue(":sdate", QString::fromStdString(session.sessionDate));
        q.bindValue(":sstart", QString::fromStdString(session.scheduledStartTime));
        q.bindValue(":send", QString::fromStdString(session.scheduledEndTime));
        q.bindValue(":astart", QString::fromStdString(session.actualStartTime));
        q.bindValue(":aend", QString::fromStdString(session.actualEndTime));
        q.bindValue(":pdur", session.plannedDurationMinutes);
        q.bindValue(":adur", session.actualDurationMinutes);
        q.bindValue(":paydur", session.payableDurationMinutes);
        q.bindValue(":st", stStr);
        q.bindValue(":appst", appStr);
        q.bindValue(":settlst", QString::fromStdString(session.settlementStatusToString()));
        if (session.approvedBy.has_value()) {
            q.bindValue(":appby", QString::fromStdString(*session.approvedBy));
        } else {
            q.bindValue(":appby", QVariant(QMetaType(QMetaType::QString)));
        }
        if (session.approvedAtUtc.has_value()) {
            q.bindValue(":appat", QString::fromStdString(*session.approvedAtUtc));
        } else {
            q.bindValue(":appat", QVariant(QMetaType(QMetaType::QString)));
        }
        q.bindValue(":rate", static_cast<qint64>(session.hourlyRateSnapshot.piasters()));
        q.bindValue(":earned", static_cast<qint64>(session.earnedAmount.piasters()));
        q.bindValue(":notes", QString::fromStdString(session.notes));
        q.bindValue(":created", now);
        q.bindValue(":updated", now);
        if (!q.exec()) {
            throw std::runtime_error("Failed to insert session: " + q.lastError().text().toStdString());
        }

        domain::Session saved = session;
        saved.id = q.lastInsertId().toLongLong();
        saved.createdAtUtc = now.toStdString();
        saved.updatedAtUtc = now.toStdString();
        return saved;
    } else {
        QSqlQuery q(db);
        q.prepare(R"(
            UPDATE sessions SET
                group_id = :gid,
                instructor_id = :iid,
                session_date = :sdate,
                scheduled_start_time = :sstart,
                scheduled_end_time = :send,
                actual_start_time = :astart,
                actual_end_time = :aend,
                planned_duration_minutes = :pdur,
                actual_duration_minutes = :adur,
                payable_duration_minutes = :paydur,
                status = :st,
                approval_status = :appst,
                settlement_status = :settlst,
                approved_by = :appby,
                approved_at_utc = :appat,
                hourly_rate_snapshot_piastres = :rate,
                earned_amount_piastres = :earned,
                notes = :notes,
                updated_at_utc = :updated
            WHERE id = :id;
        )");
        q.bindValue(":gid", static_cast<qint64>(session.groupId));
        if (session.instructorId.has_value() && *session.instructorId > 0) {
            q.bindValue(":iid", static_cast<qint64>(*session.instructorId));
        } else {
            q.bindValue(":iid", QVariant(QMetaType(QMetaType::LongLong)));
        }
        q.bindValue(":sdate", QString::fromStdString(session.sessionDate));
        q.bindValue(":sstart", QString::fromStdString(session.scheduledStartTime));
        q.bindValue(":send", QString::fromStdString(session.scheduledEndTime));
        q.bindValue(":astart", QString::fromStdString(session.actualStartTime));
        q.bindValue(":aend", QString::fromStdString(session.actualEndTime));
        q.bindValue(":pdur", session.plannedDurationMinutes);
        q.bindValue(":adur", session.actualDurationMinutes);
        q.bindValue(":paydur", session.payableDurationMinutes);
        q.bindValue(":st", stStr);
        q.bindValue(":appst", appStr);
        q.bindValue(":settlst", QString::fromStdString(session.settlementStatusToString()));
        if (session.approvedBy.has_value()) {
            q.bindValue(":appby", QString::fromStdString(*session.approvedBy));
        } else {
            q.bindValue(":appby", QVariant(QMetaType(QMetaType::QString)));
        }
        if (session.approvedAtUtc.has_value()) {
            q.bindValue(":appat", QString::fromStdString(*session.approvedAtUtc));
        } else {
            q.bindValue(":appat", QVariant(QMetaType(QMetaType::QString)));
        }
        q.bindValue(":rate", static_cast<qint64>(session.hourlyRateSnapshot.piasters()));
        q.bindValue(":earned", static_cast<qint64>(session.earnedAmount.piasters()));
        q.bindValue(":notes", QString::fromStdString(session.notes));
        q.bindValue(":updated", now);
        q.bindValue(":id", static_cast<qint64>(session.id));
        if (!q.exec()) {
            throw std::runtime_error("Failed to update session: " + q.lastError().text().toStdString());
        }

        domain::Session saved = session;
        saved.updatedAtUtc = now.toStdString();
        return saved;
    }
}

bool SqliteSessionRepository::remove(std::int64_t id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("DELETE FROM sessions WHERE id = :id;");
    q.bindValue(":id", static_cast<qint64>(id));
    return q.exec();
}

std::int64_t SqliteSessionRepository::sumApprovedEarnings(std::int64_t instructorId, const std::string& fromDate, const std::string& toDate) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT COALESCE(SUM(earned_amount_piastres), 0) FROM sessions WHERE instructor_id = :iid AND approval_status = 'approved'";
    QMap<QString, QVariant> bindings;
    bindings[":iid"] = static_cast<qint64>(instructorId);

    if (!fromDate.empty()) {
        sql += " AND session_date >= :fdate";
        bindings[":fdate"] = QString::fromStdString(fromDate);
    }
    if (!toDate.empty()) {
        sql += " AND session_date <= :tdate";
        bindings[":tdate"] = QString::fromStdString(toDate);
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) {
        q.bindValue(it.key(), it.value());
    }

    if (q.exec() && q.next()) {
        return q.value(0).toLongLong();
    }
    return 0;
}

int SqliteSessionRepository::sumApprovedMinutes(std::int64_t instructorId, const std::string& fromDate, const std::string& toDate) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT COALESCE(SUM(payable_duration_minutes), 0) FROM sessions WHERE instructor_id = :iid AND approval_status = 'approved'";
    QMap<QString, QVariant> bindings;
    bindings[":iid"] = static_cast<qint64>(instructorId);

    if (!fromDate.empty()) {
        sql += " AND session_date >= :fdate";
        bindings[":fdate"] = QString::fromStdString(fromDate);
    }
    if (!toDate.empty()) {
        sql += " AND session_date <= :tdate";
        bindings[":tdate"] = QString::fromStdString(toDate);
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) {
        q.bindValue(it.key(), it.value());
    }

    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

int SqliteSessionRepository::countLessonsTaught(std::int64_t instructorId, const std::string& fromDate, const std::string& toDate) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT COUNT(*) FROM sessions WHERE instructor_id = :iid AND approval_status = 'approved'";
    QMap<QString, QVariant> bindings;
    bindings[":iid"] = static_cast<qint64>(instructorId);

    if (!fromDate.empty()) {
        sql += " AND session_date >= :fdate";
        bindings[":fdate"] = QString::fromStdString(fromDate);
    }
    if (!toDate.empty()) {
        sql += " AND session_date <= :tdate";
        bindings[":tdate"] = QString::fromStdString(toDate);
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) {
        q.bindValue(it.key(), it.value());
    }

    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

std::int64_t SqliteSessionRepository::sumAllApprovedEarnings() {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT COALESCE(SUM(earned_amount_piastres), 0) FROM sessions WHERE approval_status = 'approved' AND status = 'completed';");
    if (q.exec() && q.next()) {
        return q.value(0).toLongLong();
    }
    return 0;
}

} // namespace pixcode::infrastructure

