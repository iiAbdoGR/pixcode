#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace pixcode::application {

struct CreateSessionCommand {
    std::int64_t groupId{0};
    std::optional<std::int64_t> instructorId;
    std::string sessionDate;        // YYYY-MM-DD
    std::string scheduledStartTime; // HH:MM
    std::string scheduledEndTime;   // HH:MM
    int plannedDurationMinutes{60};
    std::string notes;
};

struct UpdateSessionCommand {
    std::int64_t id{0};
    std::optional<std::int64_t> instructorId;
    std::string sessionDate;
    std::string scheduledStartTime;
    std::string scheduledEndTime;
    int plannedDurationMinutes{0};
    std::string actualStartTime;
    std::string actualEndTime;
    int actualDurationMinutes{0};
    int payableDurationMinutes{0};
    std::string status; // "scheduled", "completed", "cancelled", "rescheduled"
    std::string notes;
};

struct ApproveSessionCommand {
    std::int64_t sessionId{0};
    int approvedPayableMinutes{60};
    std::string approvedByUserId;
    std::string notes;
};

struct RecordInstructorPayoutCommand {
    std::int64_t instructorId{0};
    std::int64_t amountPiastres{0};
    std::string paymentDateUtc;
    std::string paymentMethod{"cash"}; // "cash", "bank_transfer", "other"
    std::string referenceNumber;
    std::string notes;
    std::optional<std::string> createdByUserId;
    std::optional<std::string> approvedByUserId;
};

struct InstructorPayoutDto {
    std::int64_t id{0};
    std::int64_t instructorId{0};
    std::string instructorName;
    std::int64_t amountPiastres{0};
    std::string paymentDateUtc;
    std::string paymentMethod;
    std::string referenceNumber;
    std::string receiptNumber;
    std::string notes;
    std::string createdAtUtc;
};

struct InstructorPayrollSummaryDto {
    std::int64_t instructorId{0};
    std::string instructorName;
    std::string specialization;
    std::string compensationType{"hourly"};
    std::int64_t currentHourlyRatePiastres{0};
    std::int64_t monthlySalaryPiastres{0};
    int totalScheduledSessions{0};
    int totalCompletedSessions{0};
    int totalApprovedSessions{0};
    int totalApprovedPayableMinutes{0}; // hours = minutes / 60.0
    std::int64_t grossEarnedPiastres{0};
    std::int64_t totalPaidOutPiastres{0};
    std::int64_t outstandingDuesPiastres{0};
    std::string settlementStatus{"unpaid"}; // "unpaid", "partially_paid", "fully_paid"
    int pendingApprovalSessionsCount{0};
    int approvedUnpaidSessionsCount{0};
    int settledSessionsCount{0};
};

struct PayoutAllocationDto {
    std::int64_t id{0};
    std::int64_t payoutId{0};
    std::optional<std::int64_t> sessionId;
    std::optional<std::int64_t> salaryPeriodId;
    std::int64_t allocatedAmountPiastres{0};
    std::string createdAtUtc;
};

struct InstructorSalaryPeriodDto {
    std::int64_t id{0};
    std::int64_t instructorId{0};
    std::string periodMonthUtc;
    std::int64_t salaryAmountPiastres{0};
    std::string status; // "pending", "approved", "paid"
    std::optional<std::string> approvedAtUtc;
    std::optional<std::string> approvedByUserId;
    std::string createdAtUtc;
};

} // namespace pixcode::application

