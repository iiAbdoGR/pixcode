#pragma once

#include "application/ports/IPaymentRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqlitePaymentRepository : public application::IPaymentRepository {
public:
    explicit SqlitePaymentRepository(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] std::optional<domain::Payment> findById(std::int64_t id) override;
    [[nodiscard]] std::vector<domain::Payment> findByEnrollment(std::int64_t enrollmentId) override;
    domain::Payment save(const domain::Payment& payment) override;
    [[nodiscard]] std::string nextReceiptNumber() override;
    [[nodiscard]] std::int64_t totalPaid(std::int64_t enrollmentId) override;

private:
    [[nodiscard]] domain::Payment mapRowToPayment(const class QSqlQuery& query);

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure

