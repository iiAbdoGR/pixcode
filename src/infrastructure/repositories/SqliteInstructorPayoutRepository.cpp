#include "SqliteInstructorPayoutRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>
#include <QtCore/QDate>

namespace pixcode::infrastructure {

SqliteInstructorPayoutRepository::SqliteInstructorPayoutRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

domain::InstructorPayout SqliteInstructorPayoutRepository::mapRowToPayout(const QSqlQuery& query) {
    domain::InstructorPayout p;
    p.id = query.value("id").toLongLong();
    p.instructorId = query.value("instructor_id").toLongLong();
    p.amount = domain::Money(query.value("amount_piastres").toLongLong());
    p.paymentDateUtc = query.value("payment_date_utc").toString().toStdString();

    QString mStr = query.value("payment_method").toString();
    if (mStr == "bank_transfer") p.paymentMethod = domain::PayoutMethod::BankTransfer;
    else if (mStr == "other") p.paymentMethod = domain::PayoutMethod::Other;
    else p.paymentMethod = domain::PayoutMethod::Cash;

    p.referenceNumber = query.value("reference_number").toString().toStdString();
    p.receiptNumber = query.value("receipt_number").toString().toStdString();
    p.notes = query.value("notes").toString().toStdString();
    if (!query.value("created_by").isNull()) {
        p.createdBy = query.value("created_by").toString().toStdString();
    }
    if (!query.value("approved_by").isNull()) {
        p.approvedBy = query.value("approved_by").toString().toStdString();
    }
    p.createdAtUtc = query.value("created_at_utc").toString().toStdString();

    return p;
}

std::optional<domain::InstructorPayout> SqliteInstructorPayoutRepository::findById(std::int64_t id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM instructor_payouts WHERE id = :id;");
    q.bindValue(":id", static_cast<qint64>(id));
    if (q.exec() && q.next()) {
        return mapRowToPayout(q);
    }
    return std::nullopt;
}

std::vector<domain::InstructorPayout> SqliteInstructorPayoutRepository::findByInstructor(std::int64_t instructorId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM instructor_payouts WHERE instructor_id = :iid ORDER BY id DESC;");
    q.bindValue(":iid", static_cast<qint64>(instructorId));
    std::vector<domain::InstructorPayout> list;
    if (q.exec()) {
        while (q.next()) {
            list.push_back(mapRowToPayout(q));
        }
    }
    return list;
}

domain::InstructorPayout SqliteInstructorPayoutRepository::save(const domain::InstructorPayout& payout) {
    auto db = m_dbManager->getConnection();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QString methodStr = "cash";
    if (payout.paymentMethod == domain::PayoutMethod::BankTransfer) methodStr = "bank_transfer";
    else if (payout.paymentMethod == domain::PayoutMethod::Other) methodStr = "other";

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO instructor_payouts (
            instructor_id, amount_piastres, payment_date_utc, payment_method,
            reference_number, receipt_number, notes, created_by, approved_by, created_at_utc
        ) VALUES (
            :iid, :amount, :pdate, :method,
            :ref, :rcp, :notes, :cby, :aby, :created
        );
    )");
    q.bindValue(":iid", static_cast<qint64>(payout.instructorId));
    q.bindValue(":amount", static_cast<qint64>(payout.amount.piasters()));
    q.bindValue(":pdate", payout.paymentDateUtc.empty() ? now : QString::fromStdString(payout.paymentDateUtc));
    q.bindValue(":method", methodStr);
    q.bindValue(":ref", QString::fromStdString(payout.referenceNumber));
    q.bindValue(":rcp", QString::fromStdString(payout.receiptNumber));
    q.bindValue(":notes", QString::fromStdString(payout.notes));
    if (payout.createdBy.has_value()) {
        q.bindValue(":cby", QString::fromStdString(*payout.createdBy));
    } else {
        q.bindValue(":cby", QVariant(QMetaType(QMetaType::QString)));
    }
    if (payout.approvedBy.has_value()) {
        q.bindValue(":aby", QString::fromStdString(*payout.approvedBy));
    } else {
        q.bindValue(":aby", QVariant(QMetaType(QMetaType::QString)));
    }
    q.bindValue(":created", now);
    q.exec();

    domain::InstructorPayout saved = payout;
    saved.id = q.lastInsertId().toLongLong();
    saved.createdAtUtc = now.toStdString();
    return saved;
}

std::string SqliteInstructorPayoutRepository::nextReceiptNumber() {
    auto db = m_dbManager->getConnection();
    int currentYear = QDate::currentDate().year();

    QSqlQuery updateQ(db);
    updateQ.prepare(R"(
        INSERT INTO payout_counters (year, last_no)
        VALUES (:year, 1)
        ON CONFLICT(year) DO UPDATE SET last_no = payout_counters.last_no + 1;
    )");
    updateQ.bindValue(":year", currentYear);
    updateQ.exec();

    QSqlQuery selectQ(db);
    selectQ.prepare("SELECT last_no FROM payout_counters WHERE year = :year;");
    selectQ.bindValue(":year", currentYear);
    if (selectQ.exec() && selectQ.next()) {
        int no = selectQ.value(0).toInt();
        return QString("PAY-%1-%2").arg(currentYear).arg(no, 5, 10, QChar('0')).toStdString();
    }
    return QString("PAY-%1-00001").arg(currentYear).toStdString();
}

std::int64_t SqliteInstructorPayoutRepository::totalPaidOut(std::int64_t instructorId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT COALESCE(SUM(amount_piastres), 0) FROM instructor_payouts WHERE instructor_id = :iid;");
    q.bindValue(":iid", static_cast<qint64>(instructorId));
    if (q.exec() && q.next()) {
        return q.value(0).toLongLong();
    }
    return 0;
}

std::int64_t SqliteInstructorPayoutRepository::totalAllPayouts() {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT COALESCE(SUM(amount_piastres), 0) FROM instructor_payouts;");
    if (q.exec() && q.next()) {
        return q.value(0).toLongLong();
    }
    return 0;
}

void SqliteInstructorPayoutRepository::saveAllocation(const domain::InstructorPayoutAllocation& alloc) {
    auto db = m_dbManager->getConnection();
    QString now = alloc.createdAtUtc.empty()
        ? QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
        : QString::fromStdString(alloc.createdAtUtc);

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO instructor_payout_allocations (
            payout_id, session_id, salary_period_id, allocated_amount_piastres, created_at_utc
        ) VALUES (
            :pid, :sid, :spid, :amount, :created
        );
    )");
    q.bindValue(":pid", static_cast<qint64>(alloc.payoutId));
    if (alloc.sessionId.has_value() && *alloc.sessionId > 0) {
        q.bindValue(":sid", static_cast<qint64>(*alloc.sessionId));
    } else {
        q.bindValue(":sid", QVariant(QMetaType(QMetaType::LongLong)));
    }
    if (alloc.salaryPeriodId.has_value() && *alloc.salaryPeriodId > 0) {
        q.bindValue(":spid", static_cast<qint64>(*alloc.salaryPeriodId));
    } else {
        q.bindValue(":spid", QVariant(QMetaType(QMetaType::LongLong)));
    }
    q.bindValue(":amount", static_cast<qint64>(alloc.allocatedAmount.piasters()));
    q.bindValue(":created", now);
    q.exec();
}

std::vector<domain::InstructorPayoutAllocation> SqliteInstructorPayoutRepository::findAllocationsByPayout(std::int64_t payoutId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM instructor_payout_allocations WHERE payout_id = :pid ORDER BY id ASC;");
    q.bindValue(":pid", static_cast<qint64>(payoutId));

    std::vector<domain::InstructorPayoutAllocation> list;
    if (q.exec()) {
        while (q.next()) {
            domain::InstructorPayoutAllocation a;
            a.id = q.value("id").toLongLong();
            a.payoutId = q.value("payout_id").toLongLong();
            if (!q.value("session_id").isNull()) {
                a.sessionId = q.value("session_id").toLongLong();
            }
            if (!q.value("salary_period_id").isNull()) {
                a.salaryPeriodId = q.value("salary_period_id").toLongLong();
            }
            a.allocatedAmount = domain::Money(q.value("allocated_amount_piastres").toLongLong());
            a.createdAtUtc = q.value("created_at_utc").toString().toStdString();
            list.push_back(a);
        }
    }
    return list;
}

} // namespace pixcode::infrastructure

