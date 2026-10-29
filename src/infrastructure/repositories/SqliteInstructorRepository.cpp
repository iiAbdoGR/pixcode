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
    inst.fullName = query.value("full_name").toString().toStdString();
    inst.phone = query.value("phone").toString().toStdString();
    inst.specialization = query.value("specialization").toString().toStdString();
    
    QString compStr = query.value("compensation_type").toString();
    inst.compensationType = domain::compensationTypeFromString(compStr.toStdString());

    std::int64_t baseRateVal = query.value("base_rate_piastres").toLongLong();
    std::int64_t hourlyRateVal = query.value("hourly_rate_piastres").toLongLong();
    if (baseRateVal == 0 && hourlyRateVal > 0) {
        baseRateVal = hourlyRateVal;
    }
    inst.baseRate = domain::Money(baseRateVal);
    inst.hourlyRate = inst.baseRate;

    std::int64_t monthlySalaryVal = query.value("monthly_salary_piastres").toLongLong();
    inst.monthlySalary = domain::Money(monthlySalaryVal);

    inst.contractStartDate = query.value("contract_start_date").toString().toStdString();
    inst.contractEndDate = query.value("contract_end_date").toString().toStdString();

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
        sql += " AND (full_name LIKE :s1 OR phone LIKE :s2 OR specialization LIKE :s3)";
        bindings[":s1"] = term;
        bindings[":s2"] = term;
        bindings[":s3"] = term;
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
    const std::int64_t rate = instructor.baseRate.piasters() > 0 ? instructor.baseRate.piasters() : instructor.hourlyRate.piasters();
    const QString compTypeStr = QString::fromStdString(domain::compensationTypeToString(instructor.compensationType));

    if (instructor.id == 0) {
        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO instructors (
                full_name, phone, specialization, compensation_type,
                base_rate_piastres, hourly_rate_piastres, monthly_salary_piastres,
                contract_start_date, contract_end_date, notes, status,
                created_at_utc, updated_at_utc
            ) VALUES (
                :name, :phone, :spec, :comp,
                :brate, :hrate, :monthly_sal,
                :c_start, :c_end, :notes, :st,
                :created, :updated
            );
        )");
        q.bindValue(":name", QString::fromStdString(instructor.fullName));
        q.bindValue(":phone", QString::fromStdString(instructor.phone));
        q.bindValue(":spec", QString::fromStdString(instructor.specialization));
        q.bindValue(":comp", compTypeStr);
        q.bindValue(":brate", static_cast<qint64>(rate));
        q.bindValue(":hrate", static_cast<qint64>(rate));
        q.bindValue(":monthly_sal", static_cast<qint64>(instructor.monthlySalary.piasters()));
        q.bindValue(":c_start", QString::fromStdString(instructor.contractStartDate));
        q.bindValue(":c_end", QString::fromStdString(instructor.contractEndDate));
        q.bindValue(":notes", QString::fromStdString(instructor.notes));
        q.bindValue(":st", (instructor.status == domain::InstructorStatus::Active) ? "active" : "inactive");
        q.bindValue(":created", now);
        q.bindValue(":updated", now);
        if (!q.exec()) {
            throw std::runtime_error("Failed to insert instructor: " + q.lastError().text().toStdString());
        }

        domain::Instructor saved = instructor;
        saved.id = q.lastInsertId().toLongLong();
        saved.baseRate = domain::Money(rate);
        saved.hourlyRate = saved.baseRate;
        saved.createdAtUtc = now.toStdString();
        saved.updatedAtUtc = now.toStdString();

        // Record initial rate history
        if (rate > 0) {
            QSqlQuery rh(db);
            rh.prepare(R"(
                INSERT INTO instructor_rate_history (
                    instructor_id, compensation_type, hourly_rate_piastres, effective_from_utc, created_at_utc, notes
                ) VALUES (
                    :inst_id, :comp, :hrate, :eff, :created, :notes
                );
            )");
            rh.bindValue(":inst_id", static_cast<qint64>(saved.id));
            rh.bindValue(":comp", compTypeStr);
            rh.bindValue(":hrate", static_cast<qint64>(rate));
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
                full_name = :name,
                phone = :phone,
                specialization = :spec,
                compensation_type = :comp,
                base_rate_piastres = :brate,
                hourly_rate_piastres = :hrate,
                monthly_salary_piastres = :monthly_sal,
                contract_start_date = :c_start,
                contract_end_date = :c_end,
                notes = :notes,
                status = :st,
                updated_at_utc = :updated
            WHERE id = :id;
        )");
        q.bindValue(":name", QString::fromStdString(instructor.fullName));
        q.bindValue(":phone", QString::fromStdString(instructor.phone));
        q.bindValue(":spec", QString::fromStdString(instructor.specialization));
        q.bindValue(":comp", compTypeStr);
        q.bindValue(":brate", static_cast<qint64>(rate));
        q.bindValue(":hrate", static_cast<qint64>(rate));
        q.bindValue(":monthly_sal", static_cast<qint64>(instructor.monthlySalary.piasters()));
        q.bindValue(":c_start", QString::fromStdString(instructor.contractStartDate));
        q.bindValue(":c_end", QString::fromStdString(instructor.contractEndDate));
        q.bindValue(":notes", QString::fromStdString(instructor.notes));
        q.bindValue(":st", (instructor.status == domain::InstructorStatus::Active) ? "active" : "inactive");
        q.bindValue(":updated", now);
        q.bindValue(":id", static_cast<qint64>(instructor.id));
        if (!q.exec()) {
            throw std::runtime_error("Failed to update instructor: " + q.lastError().text().toStdString());
        }

        domain::Instructor saved = instructor;
        saved.baseRate = domain::Money(rate);
        saved.hourlyRate = saved.baseRate;
        saved.updatedAtUtc = now.toStdString();
        return saved;
    }
}

bool SqliteInstructorRepository::updateHourlyRate(std::int64_t instructorId, std::int64_t ratePiastres, const std::string& notes) {
    auto db = m_dbManager->getConnection();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QSqlQuery q(db);
    q.prepare("UPDATE instructors SET base_rate_piastres = :brate, hourly_rate_piastres = :hrate, updated_at_utc = :updated WHERE id = :id;");
    q.bindValue(":brate", static_cast<qint64>(ratePiastres));
    q.bindValue(":hrate", static_cast<qint64>(ratePiastres));
    q.bindValue(":updated", now);
    q.bindValue(":id", static_cast<qint64>(instructorId));
    if (!q.exec()) {
        return false;
    }

    QSqlQuery rh(db);
    rh.prepare(R"(
        INSERT INTO instructor_rate_history (
            instructor_id, compensation_type, hourly_rate_piastres, effective_from_utc, created_at_utc, notes
        ) VALUES (
            :inst_id, 'hourly', :hrate, :eff, :created, :notes
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
