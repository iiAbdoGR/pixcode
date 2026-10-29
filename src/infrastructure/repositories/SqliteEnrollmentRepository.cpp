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
    else if (st == "completed") e.status = domain::EnrollmentStatus::Completed;
    else e.status = domain::EnrollmentStatus::Active;

    e.enrolledAtUtc = query.value("enrolled_at_utc").toString().toStdString();
    if (!query.value("cancelled_at_utc").isNull()) {
        e.cancelledAtUtc = query.value("cancelled_at_utc").toString().toStdString();
    }
    if (!query.value("cancellation_reason").isNull()) {
        e.cancellationReason = query.value("cancellation_reason").toString().toStdString();
    }
    if (!query.value("cancelled_by_user_id").isNull()) {
        e.cancelledByUserId = query.value("cancelled_by_user_id").toString().toStdString();
    }
    if (!query.value("restored_at_utc").isNull()) {
        e.restoredAtUtc = query.value("restored_at_utc").toString().toStdString();
    }
    if (!query.value("restoration_reason").isNull()) {
        e.restorationReason = query.value("restoration_reason").toString().toStdString();
    }
    if (!query.value("restored_by_user_id").isNull()) {
        e.restoredByUserId = query.value("restored_by_user_id").toString().toStdString();
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

std::int64_t SqliteEnrollmentRepository::countActiveInGroup(std::int64_t groupId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM enrollments WHERE group_id = :gid AND status = 'active';");
    q.bindValue(":gid", static_cast<qint64>(groupId));
    if (q.exec() && q.next()) {
        return q.value(0).toLongLong();
    }
    return 0;
}

std::vector<domain::Enrollment> SqliteEnrollmentRepository::findAll() {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM enrollments ORDER BY id DESC;");
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
    else if (enrollment.status == domain::EnrollmentStatus::Completed) stStr = "completed";

    if (enrollment.id == 0) {
        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO enrollments (
                student_id, group_id, agreed_fee_piastres, discount_piastres,
                net_fee_piastres, status, enrolled_at_utc,
                cancelled_at_utc, cancellation_reason, cancelled_by_user_id,
                restored_at_utc, restoration_reason, restored_by_user_id,
                notes
            ) VALUES (
                :sid, :gid, :agreed, :disc,
                :net, :st, :enrolled,
                :canc_at, :canc_rsn, :canc_by,
                :rest_at, :rest_rsn, :rest_by,
                :notes
            );
        )");
        q.bindValue(":sid", static_cast<qint64>(enrollment.studentId));
        q.bindValue(":gid", static_cast<qint64>(enrollment.groupId));
        q.bindValue(":agreed", static_cast<qint64>(enrollment.agreedFee.piasters()));
        q.bindValue(":disc", static_cast<qint64>(enrollment.discount.piasters()));
        q.bindValue(":net", static_cast<qint64>(enrollment.netFee.piasters()));
        q.bindValue(":st", stStr);
        q.bindValue(":enrolled", enrollment.enrolledAtUtc.empty() ? now : QString::fromStdString(enrollment.enrolledAtUtc));
        
        q.bindValue(":canc_at", enrollment.cancelledAtUtc ? QString::fromStdString(*enrollment.cancelledAtUtc) : QVariant(QMetaType(QMetaType::QString)));
        q.bindValue(":canc_rsn", enrollment.cancellationReason ? QString::fromStdString(*enrollment.cancellationReason) : QVariant(QMetaType(QMetaType::QString)));
        q.bindValue(":canc_by", enrollment.cancelledByUserId ? QString::fromStdString(*enrollment.cancelledByUserId) : QVariant(QMetaType(QMetaType::QString)));
        
        q.bindValue(":rest_at", enrollment.restoredAtUtc ? QString::fromStdString(*enrollment.restoredAtUtc) : QVariant(QMetaType(QMetaType::QString)));
        q.bindValue(":rest_rsn", enrollment.restorationReason ? QString::fromStdString(*enrollment.restorationReason) : QVariant(QMetaType(QMetaType::QString)));
        q.bindValue(":rest_by", enrollment.restoredByUserId ? QString::fromStdString(*enrollment.restoredByUserId) : QVariant(QMetaType(QMetaType::QString)));

        q.bindValue(":notes", QString::fromStdString(enrollment.notes));
        q.exec();

        domain::Enrollment saved = enrollment;
        saved.id = q.lastInsertId().toLongLong();
        if (saved.enrolledAtUtc.empty()) {
            saved.enrolledAtUtc = now.toStdString();
        }
        return saved;
    } else {
        QSqlQuery q(db);
        q.prepare(R"(
            UPDATE enrollments SET
                agreed_fee_piastres = :agreed,
                discount_piastres = :disc,
                net_fee_piastres = :net,
                status = :st,
                cancelled_at_utc = :canc_at,
                cancellation_reason = :canc_rsn,
                cancelled_by_user_id = :canc_by,
                restored_at_utc = :rest_at,
                restoration_reason = :rest_rsn,
                restored_by_user_id = :rest_by,
                notes = :notes
            WHERE id = :id;
        )");
        q.bindValue(":agreed", static_cast<qint64>(enrollment.agreedFee.piasters()));
        q.bindValue(":disc", static_cast<qint64>(enrollment.discount.piasters()));
        q.bindValue(":net", static_cast<qint64>(enrollment.netFee.piasters()));
        q.bindValue(":st", stStr);

        q.bindValue(":canc_at", enrollment.cancelledAtUtc ? QString::fromStdString(*enrollment.cancelledAtUtc) : QVariant(QMetaType(QMetaType::QString)));
        q.bindValue(":canc_rsn", enrollment.cancellationReason ? QString::fromStdString(*enrollment.cancellationReason) : QVariant(QMetaType(QMetaType::QString)));
        q.bindValue(":canc_by", enrollment.cancelledByUserId ? QString::fromStdString(*enrollment.cancelledByUserId) : QVariant(QMetaType(QMetaType::QString)));

        q.bindValue(":rest_at", enrollment.restoredAtUtc ? QString::fromStdString(*enrollment.restoredAtUtc) : QVariant(QMetaType(QMetaType::QString)));
        q.bindValue(":rest_rsn", enrollment.restorationReason ? QString::fromStdString(*enrollment.restorationReason) : QVariant(QMetaType(QMetaType::QString)));
        q.bindValue(":rest_by", enrollment.restoredByUserId ? QString::fromStdString(*enrollment.restoredByUserId) : QVariant(QMetaType(QMetaType::QString)));

        q.bindValue(":notes", QString::fromStdString(enrollment.notes));
        q.bindValue(":id", static_cast<qint64>(enrollment.id));
        q.exec();

        return enrollment;
    }
}

} // namespace pixcode::infrastructure
