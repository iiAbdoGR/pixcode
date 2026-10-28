#include "SqliteEnrollmentRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>

namespace pixcode::infrastructure {

SqliteEnrollmentRepository::SqliteEnrollmentRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

domain::Enrollment SqliteEnrollmentRepository::mapRowToEnrollment(const QSqlQuery& query) {
    domain::Enrollment e;
    e.id = query.value("id").toLongLong();
    e.studentId = query.value("student_id").toLongLong();
    e.groupId = query.value("group_id").toLongLong();
    e.agreedFee = domain::Money{query.value("agreed_fee_piastres").toLongLong()};
    e.discount = domain::Money{query.value("discount_piastres").toLongLong()};
    e.netFee = domain::Money{query.value("net_fee_piastres").toLongLong()};

    QString st = query.value("status").toString();
    if (st == "cancelled") e.status = domain::EnrollmentStatus::Cancelled;
    else if (st == "transferred") e.status = domain::EnrollmentStatus::Transferred;
    else e.status = domain::EnrollmentStatus::Active;

    e.enrolledAtUtc = query.value("enrolled_at_utc").toString().toStdString();
    if (!query.value("cancelled_at_utc").isNull()) {
        e.cancelledAtUtc = query.value("cancelled_at_utc").toString().toStdString();
    }
    e.notes = query.value("notes").toString().toStdString();
    return e;
}

std::optional<domain::Enrollment> SqliteEnrollmentRepository::findById(std::int64_t id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM enrollments WHERE id = :id;");
    q.bindValue(":id", static_cast<qint64>(id));
    if (q.exec() && q.next()) {
        return mapRowToEnrollment(q);
    }
    return std::nullopt;
}

std::optional<domain::Enrollment> SqliteEnrollmentRepository::findActive(std::int64_t studentId, std::int64_t groupId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM enrollments WHERE student_id = :sid AND group_id = :gid AND status = 'active';");
    q.bindValue(":sid", static_cast<qint64>(studentId));
    q.bindValue(":gid", static_cast<qint64>(groupId));
    if (q.exec() && q.next()) {
        return mapRowToEnrollment(q);
    }
    return std::nullopt;
}

std::vector<domain::Enrollment> SqliteEnrollmentRepository::findByStudent(std::int64_t studentId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM enrollments WHERE student_id = :sid ORDER BY id DESC;");
    q.bindValue(":sid", static_cast<qint64>(studentId));
    std::vector<domain::Enrollment> list;
    if (q.exec()) {
        while (q.next()) {
            list.push_back(mapRowToEnrollment(q));
        }
    }
    return list;
}

std::vector<domain::Enrollment> SqliteEnrollmentRepository::findByGroup(std::int64_t groupId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM enrollments WHERE group_id = :gid ORDER BY id DESC;");
    q.bindValue(":gid", static_cast<qint64>(groupId));
    std::vector<domain::Enrollment> list;
    if (q.exec()) {
        while (q.next()) {
            list.push_back(mapRowToEnrollment(q));
        }
    }
    return list;
}

domain::Enrollment SqliteEnrollmentRepository::save(const domain::Enrollment& enrollment) {
    auto db = m_dbManager->getConnection();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QString stStr = "active";
    if (enrollment.status == domain::EnrollmentStatus::Cancelled) stStr = "cancelled";
    else if (enrollment.status == domain::EnrollmentStatus::Transferred) stStr = "transferred";

    if (enrollment.id == 0) {
        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO enrollments (
                student_id, group_id, agreed_fee_piastres, discount_piastres,
                net_fee_piastres, status, enrolled_at_utc, notes
            ) VALUES (
                :sid, :gid, :agreed, :disc,
                :net, :st, :enrolled, :notes
            );
        )");
        q.bindValue(":sid", static_cast<qint64>(enrollment.studentId));
        q.bindValue(":gid", static_cast<qint64>(enrollment.groupId));
        q.bindValue(":agreed", static_cast<qint64>(enrollment.agreedFee.piasters()));
        q.bindValue(":disc", static_cast<qint64>(enrollment.discount.piasters()));
        q.bindValue(":net", static_cast<qint64>(enrollment.netFee.piasters()));
        q.bindValue(":st", stStr);
        q.bindValue(":enrolled", now);
        q.bindValue(":notes", QString::fromStdString(enrollment.notes));
        q.exec();

        domain::Enrollment saved = enrollment;
        saved.id = q.lastInsertId().toLongLong();
        saved.enrolledAtUtc = now.toStdString();
        return saved;
    } else {
        QSqlQuery q(db);
        q.prepare(R"(
            UPDATE enrollments SET
                agreed_fee_piastres = :agreed,
                discount_piastres = :disc,
                net_fee_piastres = :net,
                status = :st,
                cancelled_at_utc = :cancelled,
                notes = :notes
            WHERE id = :id;
        )");
        q.bindValue(":agreed", static_cast<qint64>(enrollment.agreedFee.piasters()));
        q.bindValue(":disc", static_cast<qint64>(enrollment.discount.piasters()));
        q.bindValue(":net", static_cast<qint64>(enrollment.netFee.piasters()));
        q.bindValue(":st", stStr);
        if (enrollment.status == domain::EnrollmentStatus::Cancelled) {
            q.bindValue(":cancelled", now);
        } else {
            q.bindValue(":cancelled", QVariant(QMetaType(QMetaType::QString)));
        }
        q.bindValue(":notes", QString::fromStdString(enrollment.notes));
        q.bindValue(":id", static_cast<qint64>(enrollment.id));
        q.exec();

        domain::Enrollment saved = enrollment;
        if (enrollment.status == domain::EnrollmentStatus::Cancelled) {
            saved.cancelledAtUtc = now.toStdString();
        }
        return saved;
    }
}

} // namespace pixcode::infrastructure

