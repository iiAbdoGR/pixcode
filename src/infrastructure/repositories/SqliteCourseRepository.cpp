#include "SqliteCourseRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>

namespace pixcode::infrastructure {

SqliteCourseRepository::SqliteCourseRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

domain::Course SqliteCourseRepository::mapRowToCourse(const QSqlQuery& query) {
    domain::Course c;
    c.id = query.value("id").toLongLong();
    c.name = query.value("name").toString().toStdString();
    c.description = query.value("description").toString().toStdString();
    c.durationWeeks = query.value("duration_weeks").toInt();
    c.defaultFee = domain::Money{query.value("default_fee_piastres").toLongLong()};
    c.status = (query.value("status").toString() == "inactive")
        ? domain::CourseStatus::Inactive
        : domain::CourseStatus::Active;
    c.createdAtUtc = query.value("created_at_utc").toString().toStdString();
    c.updatedAtUtc = query.value("updated_at_utc").toString().toStdString();
    return c;
}

std::optional<domain::Course> SqliteCourseRepository::findById(std::int64_t id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM courses WHERE id = :id;");
    q.bindValue(":id", static_cast<qint64>(id));
    if (q.exec() && q.next()) {
        return mapRowToCourse(q);
    }
    return std::nullopt;
}

std::vector<domain::Course> SqliteCourseRepository::findAll(const application::CourseFilter& filter) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT * FROM courses WHERE 1=1";
    QMap<QString, QVariant> bindings;

    if (!filter.searchText.empty()) {
        QString term = QString("%%1%").arg(QString::fromStdString(filter.searchText));
        sql += " AND (name LIKE :s1 OR description LIKE :s2)";
        bindings[":s1"] = term;
        bindings[":s2"] = term;
    }
    if (filter.status.has_value()) {
        sql += " AND status = :st";
        bindings[":st"] = (*filter.status == domain::CourseStatus::Active) ? "active" : "inactive";
    }

    sql += " ORDER BY id DESC";
    if (filter.pageSize > 0) {
        sql += QString(" LIMIT %1 OFFSET %2").arg(filter.pageSize).arg(filter.page * filter.pageSize);
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) { q.bindValue(it.key(), it.value()); }

    std::vector<domain::Course> list;
    if (q.exec()) {
        while (q.next()) {
            list.push_back(mapRowToCourse(q));
        }
    }
    return list;
}

int SqliteCourseRepository::count(const application::CourseFilter& filter) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT COUNT(*) FROM courses WHERE 1=1";
    QMap<QString, QVariant> bindings;

    if (!filter.searchText.empty()) {
        QString term = QString("%%1%").arg(QString::fromStdString(filter.searchText));
        sql += " AND (name LIKE :s1 OR description LIKE :s2)";
        bindings[":s1"] = term;
        bindings[":s2"] = term;
    }
    if (filter.status.has_value()) {
        sql += " AND status = :st";
        bindings[":st"] = (*filter.status == domain::CourseStatus::Active) ? "active" : "inactive";
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) { q.bindValue(it.key(), it.value()); }

    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

domain::Course SqliteCourseRepository::save(const domain::Course& course) {
    auto db = m_dbManager->getConnection();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    if (course.id == 0) {
        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO courses (
                name, description, duration_weeks, default_fee_piastres,
                status, created_at_utc, updated_at_utc
            ) VALUES (
                :name, :desc, :dur, :fee, :st, :created, :updated
            );
        )");
        q.bindValue(":name", QString::fromStdString(course.name));
        q.bindValue(":desc", QString::fromStdString(course.description));
        q.bindValue(":dur", course.durationWeeks);
        q.bindValue(":fee", static_cast<qint64>(course.defaultFee.piasters()));
        q.bindValue(":st", (course.status == domain::CourseStatus::Active) ? "active" : "inactive");
        q.bindValue(":created", now);
        q.bindValue(":updated", now);
        q.exec();

        domain::Course saved = course;
        saved.id = q.lastInsertId().toLongLong();
        saved.createdAtUtc = now.toStdString();
        saved.updatedAtUtc = now.toStdString();
        return saved;
    } else {
        QSqlQuery q(db);
        q.prepare(R"(
            UPDATE courses SET
                name = :name,
                description = :desc,
                duration_weeks = :dur,
                default_fee_piastres = :fee,
                status = :st,
                updated_at_utc = :updated
            WHERE id = :id;
        )");
        q.bindValue(":name", QString::fromStdString(course.name));
        q.bindValue(":desc", QString::fromStdString(course.description));
        q.bindValue(":dur", course.durationWeeks);
        q.bindValue(":fee", static_cast<qint64>(course.defaultFee.piasters()));
        q.bindValue(":st", (course.status == domain::CourseStatus::Active) ? "active" : "inactive");
        q.bindValue(":updated", now);
        q.bindValue(":id", static_cast<qint64>(course.id));
        q.exec();

        domain::Course saved = course;
        saved.updatedAtUtc = now.toStdString();
        return saved;
    }
}

} // namespace pixcode::infrastructure
