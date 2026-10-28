#include "SqliteGroupRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>

namespace pixcode::infrastructure {

SqliteGroupRepository::SqliteGroupRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

domain::Group SqliteGroupRepository::mapRowToGroup(const QSqlQuery& query) {
    domain::Group g;
    g.id = query.value("id").toLongLong();
    g.courseId = query.value("course_id").toLongLong();
    if (!query.value("instructor_id").isNull()) {
        g.instructorId = query.value("instructor_id").toLongLong();
    }
    g.name = query.value("name").toString().toStdString();
    g.startDate = query.value("start_date").toString().toStdString();
    g.endDate = query.value("end_date").toString().toStdString();
    g.scheduleDescription = query.value("schedule_description").toString().toStdString();
    g.capacity = query.value("capacity").toInt();
    g.fee = domain::Money{query.value("fee_piastres").toLongLong()};

    QString st = query.value("status").toString();
    if (st == "full") g.status = domain::GroupStatus::Full;
    else if (st == "closed") g.status = domain::GroupStatus::Closed;
    else if (st == "cancelled") g.status = domain::GroupStatus::Cancelled;
    else g.status = domain::GroupStatus::Open;

    g.createdAtUtc = query.value("created_at_utc").toString().toStdString();
    g.updatedAtUtc = query.value("updated_at_utc").toString().toStdString();
    return g;
}

std::optional<domain::Group> SqliteGroupRepository::findById(std::int64_t id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM groups WHERE id = :id;");
    q.bindValue(":id", static_cast<qint64>(id));
    if (q.exec() && q.next()) {
        return mapRowToGroup(q);
    }
    return std::nullopt;
}

std::vector<domain::Group> SqliteGroupRepository::findAll(const application::GroupFilter& filter) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT * FROM groups WHERE 1=1";
    QMap<QString, QVariant> bindings;

    if (!filter.searchText.empty()) {
        QString term = QString("%%1%").arg(QString::fromStdString(filter.searchText));
        sql += " AND (name LIKE :s1 OR schedule_description LIKE :s2)";
        bindings[":s1"] = term;
        bindings[":s2"] = term;
    }
    if (filter.courseId > 0) {
        sql += " AND course_id = :cid";
        bindings[":cid"] = static_cast<qint64>(filter.courseId);
    }
    if (filter.status.has_value()) {
        QString stStr = "open";
        switch (*filter.status) {
            case domain::GroupStatus::Full: stStr = "full"; break;
            case domain::GroupStatus::Closed: stStr = "closed"; break;
            case domain::GroupStatus::Cancelled: stStr = "cancelled"; break;
            default: stStr = "open"; break;
        }
        sql += " AND status = :st";
        bindings[":st"] = stStr;
    }

    sql += " ORDER BY id DESC";
    if (filter.pageSize > 0) {
        sql += QString(" LIMIT %1 OFFSET %2").arg(filter.pageSize).arg(filter.page * filter.pageSize);
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) { q.bindValue(it.key(), it.value()); }

    std::vector<domain::Group> list;
    if (q.exec()) {
        while (q.next()) {
            list.push_back(mapRowToGroup(q));
        }
    }
    return list;
}

int SqliteGroupRepository::count(const application::GroupFilter& filter) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT COUNT(*) FROM groups WHERE 1=1";
    QMap<QString, QVariant> bindings;

    if (!filter.searchText.empty()) {
        QString term = QString("%%1%").arg(QString::fromStdString(filter.searchText));
        sql += " AND (name LIKE :s1 OR schedule_description LIKE :s2)";
        bindings[":s1"] = term;
        bindings[":s2"] = term;
    }
    if (filter.courseId > 0) {
        sql += " AND course_id = :cid";
        bindings[":cid"] = static_cast<qint64>(filter.courseId);
    }
    if (filter.status.has_value()) {
        QString stStr = "open";
        switch (*filter.status) {
            case domain::GroupStatus::Full: stStr = "full"; break;
            case domain::GroupStatus::Closed: stStr = "closed"; break;
            case domain::GroupStatus::Cancelled: stStr = "cancelled"; break;
            default: stStr = "open"; break;
        }
        sql += " AND status = :st";
        bindings[":st"] = stStr;
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) { q.bindValue(it.key(), it.value()); }

    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

domain::Group SqliteGroupRepository::save(const domain::Group& group) {
    auto db = m_dbManager->getConnection();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QString stStr = "open";
    switch (group.status) {
        case domain::GroupStatus::Full: stStr = "full"; break;
        case domain::GroupStatus::Closed: stStr = "closed"; break;
        case domain::GroupStatus::Cancelled: stStr = "cancelled"; break;
        default: stStr = "open"; break;
    }

    if (group.id == 0) {
        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO groups (
                course_id, instructor_id, name, start_date, end_date,
                schedule_description, capacity, fee_piastres, status,
                created_at_utc, updated_at_utc
            ) VALUES (
                :cid, :iid, :name, :sdate, :edate,
                :sched, :cap, :fee, :st, :created, :updated
            );
        )");
        q.bindValue(":cid", static_cast<qint64>(group.courseId));
        if (group.instructorId.has_value() && *group.instructorId > 0) {
            q.bindValue(":iid", static_cast<qint64>(*group.instructorId));
        } else {
            q.bindValue(":iid", QVariant(QMetaType(QMetaType::LongLong)));
        }
        q.bindValue(":name", QString::fromStdString(group.name));
        q.bindValue(":sdate", QString::fromStdString(group.startDate));
        q.bindValue(":edate", QString::fromStdString(group.endDate));
        q.bindValue(":sched", QString::fromStdString(group.scheduleDescription));
        q.bindValue(":cap", group.capacity);
        q.bindValue(":fee", static_cast<qint64>(group.fee.piasters()));
        q.bindValue(":st", stStr);
        q.bindValue(":created", now);
        q.bindValue(":updated", now);
        q.exec();

        domain::Group saved = group;
        saved.id = q.lastInsertId().toLongLong();
        saved.createdAtUtc = now.toStdString();
        saved.updatedAtUtc = now.toStdString();
        return saved;
    } else {
        QSqlQuery q(db);
        q.prepare(R"(
            UPDATE groups SET
                course_id = :cid,
                instructor_id = :iid,
                name = :name,
                start_date = :sdate,
                end_date = :edate,
                schedule_description = :sched,
                capacity = :cap,
                fee_piastres = :fee,
                status = :st,
                updated_at_utc = :updated
            WHERE id = :id;
        )");
        q.bindValue(":cid", static_cast<qint64>(group.courseId));
        if (group.instructorId.has_value() && *group.instructorId > 0) {
            q.bindValue(":iid", static_cast<qint64>(*group.instructorId));
        } else {
            q.bindValue(":iid", QVariant(QMetaType(QMetaType::LongLong)));
        }
        q.bindValue(":name", QString::fromStdString(group.name));
        q.bindValue(":sdate", QString::fromStdString(group.startDate));
        q.bindValue(":edate", QString::fromStdString(group.endDate));
        q.bindValue(":sched", QString::fromStdString(group.scheduleDescription));
        q.bindValue(":cap", group.capacity);
        q.bindValue(":fee", static_cast<qint64>(group.fee.piasters()));
        q.bindValue(":st", stStr);
        q.bindValue(":updated", now);
        q.bindValue(":id", static_cast<qint64>(group.id));
        q.exec();

        domain::Group saved = group;
        saved.updatedAtUtc = now.toStdString();
        return saved;
    }
}

int SqliteGroupRepository::countActiveEnrollments(std::int64_t groupId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM enrollments WHERE group_id = :gid AND status = 'active';");
    q.bindValue(":gid", static_cast<qint64>(groupId));
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

} // namespace pixcode::infrastructure
