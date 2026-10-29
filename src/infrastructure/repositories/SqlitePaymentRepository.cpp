#include "SqlitePaymentRepository.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>
#include <QtCore/QDate>

namespace pixcode::infrastructure {

SqlitePaymentRepository::SqlitePaymentRepository(std::shared_ptr<DatabaseManager> dbManager)
    : m_dbManager(std::move(dbManager)) {}

domain::Payment SqlitePaymentRepository::mapRowToPayment(const QSqlQuery& query) {
    domain::Payment p;
    p.id = query.value("id").toLongLong();
    p.enrollmentId = query.value("enrollment_id").toLongLong();
    p.amount = domain::Money{query.value("amount_piastres").toLongLong()};

    QString methodStr = query.value("payment_method").toString();
    if (methodStr == "bank_transfer") p.method = domain::PaymentMethod::BankTransfer;
    else if (methodStr == "other") p.method = domain::PaymentMethod::Other;
    else p.method = domain::PaymentMethod::Cash;

    p.paidAtUtc = query.value("paid_at_utc").toString().toStdString();
    p.receiptNumber = query.value("receipt_number").toString().toStdString();
    p.notes = query.value("notes").toString().toStdString();
    if (!query.value("recorded_by").isNull()) {
        p.recordedBy = query.value("recorded_by").toString().toStdString();
    }
    p.createdAtUtc = query.value("created_at_utc").toString().toStdString();
    return p;
}

std::optional<domain::Payment> SqlitePaymentRepository::findById(std::int64_t id) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM payments WHERE id = :id;");
    q.bindValue(":id", static_cast<qint64>(id));
    if (q.exec() && q.next()) {
        return mapRowToPayment(q);
    }
    return std::nullopt;
}

std::vector<domain::Payment> SqlitePaymentRepository::findByEnrollment(std::int64_t enrollmentId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM payments WHERE enrollment_id = :eid ORDER BY id DESC;");
    q.bindValue(":eid", static_cast<qint64>(enrollmentId));
    std::vector<domain::Payment> list;
    if (q.exec()) {
        while (q.next()) {
            list.push_back(mapRowToPayment(q));
        }
    }
    return list;
}

domain::Payment SqlitePaymentRepository::save(const domain::Payment& payment) {
    auto db = m_dbManager->getConnection();
    QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QString methodStr = "cash";
    if (payment.method == domain::PaymentMethod::BankTransfer) methodStr = "bank_transfer";
    else if (payment.method == domain::PaymentMethod::Other) methodStr = "other";

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO payments (
            enrollment_id, amount_piastres, payment_method,
            paid_at_utc, receipt_number, notes, recorded_by, created_at_utc
        ) VALUES (
            :eid, :amount, :method,
            :paid_at, :receipt, :notes, :rec_by, :created
        );
    )");
    q.bindValue(":eid", static_cast<qint64>(payment.enrollmentId));
    q.bindValue(":amount", static_cast<qint64>(payment.amount.piasters()));
    q.bindValue(":method", methodStr);
    q.bindValue(":paid_at", payment.paidAtUtc.empty() ? now : QString::fromStdString(payment.paidAtUtc));
    q.bindValue(":receipt", QString::fromStdString(payment.receiptNumber));
    q.bindValue(":notes", QString::fromStdString(payment.notes));
    if (payment.recordedBy.has_value()) {
        q.bindValue(":rec_by", QString::fromStdString(*payment.recordedBy));
    } else {
        q.bindValue(":rec_by", QVariant(QMetaType(QMetaType::QString)));
    }
    q.bindValue(":created", now);
    q.exec();

    domain::Payment saved = payment;
    saved.id = q.lastInsertId().toLongLong();
    saved.createdAtUtc = now.toStdString();
    return saved;
}

std::string SqlitePaymentRepository::nextReceiptNumber() {
    auto db = m_dbManager->getConnection();
    int currentYear = QDate::currentDate().year();

    QSqlQuery updateQ(db);
    updateQ.prepare(R"(
        INSERT INTO receipt_counters (year, last_no)
        VALUES (:year, 1)
        ON CONFLICT(year) DO UPDATE SET last_no = receipt_counters.last_no + 1;
    )");
    updateQ.bindValue(":year", currentYear);
    updateQ.exec();

    QSqlQuery selectQ(db);
    selectQ.prepare("SELECT last_no FROM receipt_counters WHERE year = :year;");
    selectQ.bindValue(":year", currentYear);
    if (selectQ.exec() && selectQ.next()) {
        int no = selectQ.value(0).toInt();
        return QString("RCP-%1-%2").arg(currentYear).arg(no, 5, 10, QChar('0')).toStdString();
    }
    return QString("RCP-%1-00001").arg(currentYear).toStdString();
}

std::int64_t SqlitePaymentRepository::totalPaid(std::int64_t enrollmentId) {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT COALESCE(SUM(amount_piastres), 0) FROM payments WHERE enrollment_id = :eid;");
    q.bindValue(":eid", static_cast<qint64>(enrollmentId));
    if (q.exec() && q.next()) {
        return q.value(0).toLongLong();
    }
    return 0;
}

std::int64_t SqlitePaymentRepository::totalAllPayments() {
    auto db = m_dbManager->getConnection();
    QSqlQuery q(db);
    q.prepare("SELECT COALESCE(SUM(amount_piastres), 0) FROM payments;");
    if (q.exec() && q.next()) {
        return q.value(0).toLongLong();
    }
    return 0;
}

} // namespace pixcode::infrastructure
