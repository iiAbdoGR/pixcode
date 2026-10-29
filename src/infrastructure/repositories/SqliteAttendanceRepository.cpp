#include "SqliteAttendanceRepository.h"
#include "database/SqliteTransaction.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>

namespace pixcode::infrastructure {

SqliteAttendanceRepository::SqliteAttendanceRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

domain::SessionAttendance SqliteAttendanceRepository::mapRowToAttendance(const QSqlQuery& query) {
    domain::SessionAttendance a;
    a.id = query.value("id").toLongLong();
    a.sessionId = query.value("session_id").toLongLong();
    a.studentId = query.value("student_id").toLongLong();

    QString st = query.value("status").toString();
    if (st == "absent") a.status = domain::AttendanceStatus::Absent;
    else if (st == "late") a.status = domain::AttendanceStatus::Late;
    else if (st == "excused") a.status = domain::AttendanceStatus::Excused;
    else a.status = domain::AttendanceStatus::Present;

    a.arrivalTime = query.value("arrival_time").toString().toStdString();
    a.notes = query.value("notes").toString().toStdString();
    if (!query.value("marked_by").isNull()) {
        a.markedBy = query.value("marked_by").toString().toStdString();
    }
    a.markedAtUtc = query.value("marked_at_utc").toString().toStdString();
    a.updatedAtUtc = query.value("updated_at_utc").toString().toStdString();

    return a;
}

std::optional<domain::SessionAttendance> SqliteAttendanceRepository::findById(std::int64_t id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM session_attendance WHERE id = :id;");
    q.bindValue(":id", static_cast<qint64>(id));
    if (q.exec() && q.next()) {
        return mapRowToAttendance(q);
    }
    return std::nullopt;
}

std::optional<domain::SessionAttendance> SqliteAttendanceRepository::findBySessionAndStudent(std::int64_t sessionId, std::int64_t studentId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM session_attendance WHERE session_id = :sid AND student_id = :stuid;");
    q.bindValue(":sid", static_cast<qint64>(sessionId));
    q.bindValue(":stuid", static_cast<qint64>(studentId));
    if (q.exec() && q.next()) {
        return mapRowToAttendance(q);
    }
    return std::nullopt;
}

std::vector<domain::SessionAttendance> SqliteAttendanceRepository::findBySession(std::int64_t sessionId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM session_attendance WHERE session_id = :sid ORDER BY id ASC;");
    q.bindValue(":sid", static_cast<qint64>(sessionId));
    std::vector<domain::SessionAttendance> list;
    if (q.exec()) {
        while (q.next()) {
            list.push_back(mapRowToAttendance(q));
        }
    }
    return list;
}

std::vector<domain::SessionAttendance> SqliteAttendanceRepository::findByStudent(std::int64_t studentId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM session_attendance WHERE student_id = :stuid ORDER BY id DESC;");
    q.bindValue(":stuid", static_cast<qint64>(studentId));
    std::vector<domain::SessionAttendance> list;
    if (q.exec()) {
        while (q.next()) {
            list.push_back(mapRowToAttendance(q));
        }
    }
    return list;
}

domain::SessionAttendance SqliteAttendanceRepository::save(const domain::SessionAttendance& attendance) {
    auto db = m_dbManager->getConnection();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QString stStr = "present";
    switch (attendance.status) {
        case domain::AttendanceStatus::Absent: stStr = "absent"; break;
        case domain::AttendanceStatus::Late: stStr = "late"; break;
        case domain::AttendanceStatus::Excused: stStr = "excused"; break;
        default: stStr = "present"; break;
    }

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO session_attendance (
            session_id, student_id, status, arrival_time, notes, marked_by, marked_at_utc, updated_at_utc
        ) VALUES (
            :sid, :stuid, :st, :arr, :notes, :by, :mat, :uat
        )
        ON CONFLICT(session_id, student_id) DO UPDATE SET
            status = excluded.status,
            arrival_time = excluded.arrival_time,
            notes = excluded.notes,
            marked_by = excluded.marked_by,
            updated_at_utc = excluded.updated_at_utc;
    )");
    q.bindValue(":sid", static_cast<qint64>(attendance.sessionId));
    q.bindValue(":stuid", static_cast<qint64>(attendance.studentId));
    q.bindValue(":st", stStr);
    q.bindValue(":arr", QString::fromStdString(attendance.arrivalTime));
    q.bindValue(":notes", QString::fromStdString(attendance.notes));
    if (attendance.markedBy.has_value()) {
        q.bindValue(":by", QString::fromStdString(*attendance.markedBy));
    } else {
        q.bindValue(":by", QVariant(QMetaType(QMetaType::QString)));
    }
    q.bindValue(":mat", now);
    q.bindValue(":uat", now);
    q.exec();

    domain::SessionAttendance saved = attendance;
    if (saved.id == 0) saved.id = q.lastInsertId().toLongLong();
    saved.markedAtUtc = now.toStdString();
    saved.updatedAtUtc = now.toStdString();
    return saved;
}

bool SqliteAttendanceRepository::saveBatch(const std::vector<domain::SessionAttendance>& items) {
    auto db = m_dbManager->getConnection();
    SqliteTransaction tx(db, true);

    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO session_attendance (
            session_id, student_id, status, arrival_time, notes, marked_by, marked_at_utc, updated_at_utc
        ) VALUES (
            :sid, :stuid, :st, :arr, :notes, :by, :mat, :uat
        )
        ON CONFLICT(session_id, student_id) DO UPDATE SET
            status = excluded.status,
            arrival_time = excluded.arrival_time,
            notes = excluded.notes,
            marked_by = excluded.marked_by,
            updated_at_utc = excluded.updated_at_utc;
    )");

    for (const auto& item : items) {
        QString stStr = "present";
        switch (item.status) {
            case domain::AttendanceStatus::Absent: stStr = "absent"; break;
            case domain::AttendanceStatus::Late: stStr = "late"; break;
            case domain::AttendanceStatus::Excused: stStr = "excused"; break;
            default: stStr = "present"; break;
        }

        q.bindValue(":sid", static_cast<qint64>(item.sessionId));
        q.bindValue(":stuid", static_cast<qint64>(item.studentId));
        q.bindValue(":st", stStr);
        q.bindValue(":arr", QString::fromStdString(item.arrivalTime));
        q.bindValue(":notes", QString::fromStdString(item.notes));
        if (item.markedBy.has_value()) {
            q.bindValue(":by", QString::fromStdString(*item.markedBy));
        } else {
            q.bindValue(":by", QVariant(QMetaType(QMetaType::QString)));
        }
        q.bindValue(":mat", now);
        q.bindValue(":uat", now);

        if (!q.exec()) {
            return false;
        }
    }

    tx.commit();
    return true;
}

int SqliteAttendanceRepository::countStudentStatus(std::int64_t studentId, domain::AttendanceStatus status) {
    auto db = m_dbManager->getConnection();
    QString stStr = "present";
    switch (status) {
        case domain::AttendanceStatus::Absent: stStr = "absent"; break;
        case domain::AttendanceStatus::Late: stStr = "late"; break;
        case domain::AttendanceStatus::Excused: stStr = "excused"; break;
        default: stStr = "present"; break;
    }

    QSqlQuery q(db);
    q.prepare(R"(
        SELECT COUNT(*)
        FROM session_attendance a
        JOIN sessions s ON a.session_id = s.id
        WHERE a.student_id = :stuid AND a.status = :st AND s.status != 'cancelled';
    )");
    q.bindValue(":stuid", static_cast<qint64>(studentId));
    q.bindValue(":st", stStr);

    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

int SqliteAttendanceRepository::countEligibleSessions(std::int64_t studentId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT COUNT(*)
        FROM session_attendance a
        JOIN sessions s ON a.session_id = s.id
        WHERE a.student_id = :stuid AND s.status != 'cancelled';
    )");
    q.bindValue(":stuid", static_cast<qint64>(studentId));
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

int SqliteAttendanceRepository::countTotalRecords() {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM session_attendance;");
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

int SqliteAttendanceRepository::countPresentRecords() {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM session_attendance WHERE status IN ('present', 'late');");
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

} // namespace pixcode::infrastructure

