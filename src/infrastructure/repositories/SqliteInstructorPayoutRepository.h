#pragma once

#include "application/ports/IInstructorPayoutRepository.h"
#include "database/DatabaseManager.h"
#include <memory>

namespace pixcode::infrastructure {

class SqliteInstructorPayoutRepository : public application::IInstructorPayoutRepository {
public:
    explicit SqliteInstructorPayoutRepository(std::shared_ptr<DatabaseManager> dbManager);

    [[nodiscard]] std::optional<domain::InstructorPayout> findById(std::int64_t id) override;
    [[nodiscard]] std::vector<domain::InstructorPayout> findByInstructor(std::int64_t instructorId) override;
    domain::InstructorPayout save(const domain::InstructorPayout& payout) override;
    [[nodiscard]] std::string nextReceiptNumber() override;
    [[nodiscard]] std::int64_t totalPaidOut(std::int64_t instructorId) override;

private:
    [[nodiscard]] domain::InstructorPayout mapRowToPayout(const class QSqlQuery& query);

    std::shared_ptr<DatabaseManager> m_dbManager;
};

} // namespace pixcode::infrastructure
