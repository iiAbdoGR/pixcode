#include "SqliteStudentRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>

namespace pixcode::infrastructure {

SqliteStudentRepository::SqliteStudentRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

domain::Student SqliteStudentRepository::mapRowToStudent(const QSqlQuery& query) {
    domain::Student s;
    s.id = query.value("id").toLongLong();
    s.studentCode = query.value("student_code").toString().toStdString();
    s.fullName = query.value("full_name").toString().toStdString();
    s.phone = query.value("phone").toString().toStdString();
    s.guardianName = query.value("guardian_name").toString().toStdString();
    s.guardianPhone = query.value("guardian_phone").toString().toStdString();
    s.address = query.value("address").toString().toStdString();
    s.notes = query.value("notes").toString().toStdString();
    s.status = (query.value("status").toString() == "archived")
        ? domain::StudentStatus::Archived
        : domain::StudentStatus::Active;
    s.registeredAtUtc = query.value("registered_at_utc").toString().toStdString();
    s.updatedAtUtc = query.value("updated_at_utc").toString().toStdString();
    return s;
}

std::optional<domain::Student> SqliteStudentRepository::findById(std::int64_t id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM students WHERE id = :id;");
    q.bindValue(":id", static_cast<qint64>(id));
    if (q.exec() && q.next()) {
        return mapRowToStudent(q);
    }
    return std::nullopt;
}

std::optional<domain::Student> SqliteStudentRepository::findByCode(const std::string& code) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM students WHERE student_code = :code;");
    q.bindValue(":code", QString::fromStdString(code));
    if (q.exec() && q.next()) {
        return mapRowToStudent(q);
    }
    return std::nullopt;
}

std::vector<domain::Student> SqliteStudentRepository::findAll(const application::StudentFilter& filter) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT * FROM students WHERE 1=1";
    QMap<QString, QVariant> bindings;

    if (!filter.searchText.empty()) {
        QString term = QString("%%1%").arg(QString::fromStdString(filter.searchText));
        sql += " AND (full_name LIKE :s1 OR phone LIKE :s2 OR student_code LIKE :s3)";
        bindings[":s1"] = term;
        bindings[":s2"] = term;
        bindings[":s3"] = term;
    }
    if (filter.status.has_value()) {
        sql += " AND status = :st";
        bindings[":st"] = (*filter.status == domain::StudentStatus::Active) ? "active" : "archived";
    }

    sql += " ORDER BY id DESC";
    if (filter.pageSize > 0) {
        sql += QString(" LIMIT %1 OFFSET %2").arg(filter.pageSize).arg(filter.page * filter.pageSize);
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) { q.bindValue(it.key(), it.value()); }

    std::vector<domain::Student> list;
    if (q.exec()) {
        while (q.next()) {
            list.push_back(mapRowToStudent(q));
        }
    }
    return list;
}

int SqliteStudentRepository::count(const application::StudentFilter& filter) {
    auto db = m_dbManager->getConnection();
    QString sql = "SELECT COUNT(*) FROM students WHERE 1=1";
    QMap<QString, QVariant> bindings;

    if (!filter.searchText.empty()) {
        QString term = QString("%%1%").arg(QString::fromStdString(filter.searchText));
        sql += " AND (full_name LIKE :s1 OR phone LIKE :s2 OR student_code LIKE :s3)";
        bindings[":s1"] = term;
        bindings[":s2"] = term;
        bindings[":s3"] = term;
    }
    if (filter.status.has_value()) {
        sql += " AND status = :st";
        bindings[":st"] = (*filter.status == domain::StudentStatus::Active) ? "active" : "archived";
    }

    QSqlQuery q(db);
    q.prepare(sql);
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) { q.bindValue(it.key(), it.value()); }

    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

domain::Student SqliteStudentRepository::save(const domain::Student& student) {
    auto db = m_dbManager->getConnection();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    if (student.id == 0) {
        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO students (
                student_code, full_name, phone, guardian_name, guardian_phone,
                address, notes, status, registered_at_utc, updated_at_utc
            ) VALUES (
                :code, :name, :phone, :gname, :gphone,
                :address, :notes, :status, :reg, :upd
            );
        )");
        q.bindValue(":code", QString::fromStdString(student.studentCode));
        q.bindValue(":name", QString::fromStdString(student.fullName));
        q.bindValue(":phone", QString::fromStdString(student.phone));
        q.bindValue(":gname", QString::fromStdString(student.guardianName));
        q.bindValue(":gphone", QString::fromStdString(student.guardianPhone));
        q.bindValue(":address", QString::fromStdString(student.address));
        q.bindValue(":notes", QString::fromStdString(student.notes));
        q.bindValue(":status", (student.status == domain::StudentStatus::Active) ? "active" : "archived");
        q.bindValue(":reg", now);
        q.bindValue(":upd", now);
        q.exec();

        domain::Student saved = student;
        saved.id = q.lastInsertId().toLongLong();
        saved.registeredAtUtc = now.toStdString();
        saved.updatedAtUtc = now.toStdString();
        return saved;
    } else {
        QSqlQuery q(db);
        q.prepare(R"(
            UPDATE students SET
                full_name = :name,
                phone = :phone,
                guardian_name = :gname,
                guardian_phone = :gphone,
                address = :address,
                notes = :notes,
                status = :status,
                updated_at_utc = :upd
            WHERE id = :id;
        )");
        q.bindValue(":name", QString::fromStdString(student.fullName));
        q.bindValue(":phone", QString::fromStdString(student.phone));
        q.bindValue(":gname", QString::fromStdString(student.guardianName));
        q.bindValue(":gphone", QString::fromStdString(student.guardianPhone));
        q.bindValue(":address", QString::fromStdString(student.address));
        q.bindValue(":notes", QString::fromStdString(student.notes));
        q.bindValue(":status", (student.status == domain::StudentStatus::Active) ? "active" : "archived");
        q.bindValue(":upd", now);
        q.bindValue(":id", static_cast<qint64>(student.id));
        q.exec();

        domain::Student saved = student;
        saved.updatedAtUtc = now.toStdString();
        return saved;
    }
}

bool SqliteStudentRepository::remove(std::int64_t id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("DELETE FROM students WHERE id = :id;");
    q.bindValue(":id", static_cast<qint64>(id));
    return q.exec();
}

std::string SqliteStudentRepository::nextStudentCode() {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT COALESCE(MAX(id), 0) + 1 FROM students;");
    if (q.exec() && q.next()) {
        int nextId = q.value(0).toInt();
        return QString("STU-%1").arg(nextId, 5, 10, QChar('0')).toStdString();
    }
    return "STU-00001";
}

} // namespace pixcode::infrastructure
