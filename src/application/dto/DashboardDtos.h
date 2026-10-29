#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace pixcode::application {

struct OperationalMetricsDto {
    std::int64_t activeStudentsCount{0};
    std::int64_t activeGroupsCount{0};
    std::int64_t todaySessionsCount{0};
    double attendanceRatePercentage{0.0};
};

struct FinancialMetricsDto {
    std::int64_t totalIncomePiastres{0};
    std::int64_t totalReceivablesPiastres{0}; // Outstanding student fees
    std::int64_t approvedInstructorPayablesPiastres{0}; // Approved unpaid instructor earnings
    std::int64_t totalPayoutsMadePiastres{0};
    std::int64_t netCashBalancePiastres{0}; // Collected income minus payouts
};

struct DashboardSummaryDto {
    OperationalMetricsDto operational;
    std::optional<FinancialMetricsDto> financial; // std::nullopt for Admin, populated for Owner
    bool isOwner{false};
};

} // namespace pixcode::application

