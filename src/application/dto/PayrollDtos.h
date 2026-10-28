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
    std::string actualStartTime;
    std::string actualEndTime;
    int actualDurationMinutes{60};
    int payableDurationMinutes{60};
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
    std::int64_t currentHourlyRatePiastres{0};
    int totalScheduledSessions{0};
    int totalCompletedSessions{0};
    int totalApprovedSessions{0};
    int totalApprovedPayableMinutes{0}; // hours = minutes / 60.0
    std::int64_t grossEarnedPiastres{0};
    std::int64_t totalPaidOutPiastres{0};
    std::int64_t outstandingDuesPiastres{0};
};

} // namespace pixcode::application
