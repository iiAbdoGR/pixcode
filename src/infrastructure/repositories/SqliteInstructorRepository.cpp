#include "SqliteInstructorRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>

namespace pixcode::infrastructure {

SqliteInstructorRepository::SqliteInstructorRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

domain::Instructor SqliteInstructorRepository::mapRowToInstructor(const QSqlQuery& query) {
    domain::Instructor inst;
    inst.id = query.value("id").toLongLong();
    if (!query.value("user_id").isNull()) {
        inst.userId = query.value("user_id").toString().toStdString();
    }
    inst.fullName = query.value("full_name").toString().toStdString();
    inst.phone = query.value("phone").toString().toStdString();
    inst.specialization = query.value("specialization").toString().toStdString();
    inst.hourlyRate = domain::Money(query.value("hourly_rate_piastres").toLongLong());
    inst.notes = query.value("notes").toString().toStdString();
    inst.status = (query.value("status").toString() == "inactive")
        ? domain::InstructorStatus::Inactive
        : domain::InstructorStatus::Active;
    inst.createdAtUtc = query.value("created_at_utc").toString().toStdString();
    inst.updatedAtUtc = query.value("updated_at_utc").toString().toStdString();
    return inst;
}

std::optional<domain::Instructor> SqliteInstructorRepository::findById(std::int64_t id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM instructors WHERE id = :id;");
    q.bindValue(":id", static_cast<qint64>(id));
    if (q.exec() && q.next()) {
        return mapRowToInstructor(q);
    }
    return std::nullopt;
}

std::optional<domain::Instructor> SqliteInstructorRepository::findByUserId(const std::string& userId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM instructors WHERE user_id = :uid;");
    q.bindValue(":uid", QString::fromStdString(userId));
    if (q.exec() && q.next()) {
        return mapRowToInstructor(q);
    }
    return std::nullopt;
}

std::vector<domain::Instructor> SqliteInstructorRepository::findAll(const application::InstructorFilter& filter) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT * FROM instructors WHERE 1=1";
    QMap<QString, QVariant> bindings;

    if (!filter.searchText.empty()) {
        QString term = QString("%%1%").arg(QString::fromStdString(filter.searchText));
        sql += " AND (full_name LIKE :s1 OR phone LIKE :s2)";
        bindings[":s1"] = term;
        bindings[":s2"] = term;
    }
    if (filter.status.has_value()) {
        sql += " AND status = :st";
        bindings[":st"] = (*filter.status == domain::InstructorStatus::Active) ? "active" : "inactive";
    }

    sql += " ORDER BY full_name ASC";

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) { q.bindValue(it.key(), it.value()); }

    std::vector<domain::Instructor> list;
    if (q.exec()) {
        while (q.next()) {
            list.push_back(mapRowToInstructor(q));
        }
    }
    return list;
}

domain::Instructor SqliteInstructorRepository::save(const domain::Instructor& instructor) {
    auto db = m_dbManager->getConnection();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    if (instructor.id == 0) {
        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO instructors (
                user_id, full_name, phone, specialization, hourly_rate_piastres, notes, status, created_at_utc, updated_at_utc
            ) VALUES (
                :uid, :name, :phone, :spec, :hrate, :notes, :st, :created, :updated
            );
        )");
        if (instructor.userId.has_value()) {
            q.bindValue(":uid", QString::fromStdString(*instructor.userId));
        } else {
            q.bindValue(":uid", QVariant(QMetaType(QMetaType::QString)));
        }
        q.bindValue(":name", QString::fromStdString(instructor.fullName));
        q.bindValue(":phone", QString::fromStdString(instructor.phone));
        q.bindValue(":spec", QString::fromStdString(instructor.specialization));
        q.bindValue(":hrate", static_cast<qint64>(instructor.hourlyRate.piastres()));
        q.bindValue(":notes", QString::fromStdString(instructor.notes));
        q.bindValue(":st", (instructor.status == domain::InstructorStatus::Active) ? "active" : "inactive");
        q.bindValue(":created", now);
        q.bindValue(":updated", now);
        q.exec();

        domain::Instructor saved = instructor;
        saved.id = q.lastInsertId().toLongLong();
        saved.createdAtUtc = now.toStdString();
        saved.updatedAtUtc = now.toStdString();

        // Record initial rate history if rate > 0
        if (instructor.hourlyRate.piastres() > 0) {
            QSqlQuery rh(db);
            rh.prepare(R"(
                INSERT INTO instructor_rate_history (
                    instructor_id, hourly_rate_piastres, effective_from_utc, created_at_utc, notes
                ) VALUES (
                    :inst_id, :hrate, :eff, :created, :notes
                );
            )");
            rh.bindValue(":inst_id", static_cast<qint64>(saved.id));
            rh.bindValue(":hrate", static_cast<qint64>(instructor.hourlyRate.piastres()));
            rh.bindValue(":eff", now);
            rh.bindValue(":created", now);
            rh.bindValue(":notes", QStringLiteral("Initial rate"));
            rh.exec();
        }

        return saved;
    } else {
        QSqlQuery q(db);
        q.prepare(R"(
            UPDATE instructors SET
                user_id = :uid,
                full_name = :name,
                phone = :phone,
                specialization = :spec,
                hourly_rate_piastres = :hrate,
                notes = :notes,
                status = :st,
                updated_at_utc = :updated
            WHERE id = :id;
        )");
        if (instructor.userId.has_value()) {
            q.bindValue(":uid", QString::fromStdString(*instructor.userId));
        } else {
            q.bindValue(":uid", QVariant(QMetaType(QMetaType::QString)));
        }
        q.bindValue(":name", QString::fromStdString(instructor.fullName));
        q.bindValue(":phone", QString::fromStdString(instructor.phone));
        q.bindValue(":spec", QString::fromStdString(instructor.specialization));
        q.bindValue(":hrate", static_cast<qint64>(instructor.hourlyRate.piastres()));
        q.bindValue(":notes", QString::fromStdString(instructor.notes));
        q.bindValue(":st", (instructor.status == domain::InstructorStatus::Active) ? "active" : "inactive");
        q.bindValue(":updated", now);
        q.bindValue(":id", static_cast<qint64>(instructor.id));
        q.exec();

        domain::Instructor saved = instructor;
        saved.updatedAtUtc = now.toStdString();
        return saved;
    }
}

bool SqliteInstructorRepository::updateHourlyRate(std::int64_t instructorId, std::int64_t ratePiastres, const std::string& notes) {
    auto db = m_dbManager->getConnection();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QSqlQuery q(db);
    q.prepare("UPDATE instructors SET hourly_rate_piastres = :hrate, updated_at_utc = :updated WHERE id = :id;");
    q.bindValue(":hrate", static_cast<qint64>(ratePiastres));
    q.bindValue(":updated", now);
    q.bindValue(":id", static_cast<qint64>(instructorId));
    if (!q.exec()) {
        return false;
    }

    QSqlQuery rh(db);
    rh.prepare(R"(
        INSERT INTO instructor_rate_history (
            instructor_id, hourly_rate_piastres, effective_from_utc, created_at_utc, notes
        ) VALUES (
            :inst_id, :hrate, :eff, :created, :notes
        );
    )");
    rh.bindValue(":inst_id", static_cast<qint64>(instructorId));
    rh.bindValue(":hrate", static_cast<qint64>(ratePiastres));
    rh.bindValue(":eff", now);
    rh.bindValue(":created", now);
    rh.bindValue(":notes", QString::fromStdString(notes));
    return rh.exec();
}

} // namespace pixcode::infrastructure
