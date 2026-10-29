#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include "Instructor.h"
#include "../value_objects/Money.h"
#include "../errors/DomainError.h"

namespace pixcode::domain {

enum class SessionStatus {
    Scheduled,
    Completed,
    Cancelled,
    Rescheduled
};

enum class SessionApprovalStatus {
    Pending,
    Approved,
    Rejected
};

enum class SessionSettlementStatus {
    Unpaid,
    PartiallyPaid,
    FullyPaid
};

struct Session {
    std::int64_t id{0};
    std::int64_t groupId{0};
    std::optional<std::int64_t> instructorId;
    std::string sessionDate;           // YYYY-MM-DD
    std::string scheduledStartTime;    // HH:MM
    std::string scheduledEndTime;      // HH:MM
    std::string actualStartTime;       // HH:MM
    std::string actualEndTime;         // HH:MM
    int plannedDurationMinutes{60};
    int actualDurationMinutes{60};
    int payableDurationMinutes{60};
    SessionStatus status{SessionStatus::Scheduled};
    SessionApprovalStatus approvalStatus{SessionApprovalStatus::Pending};
    SessionSettlementStatus settlementStatus{SessionSettlementStatus::Unpaid};
    std::optional<std::string> approvedBy;
    std::optional<std::string> approvedAtUtc;
    Money hourlyRateSnapshot{0};
    Money earnedAmount{0};
    std::string notes;
    std::string createdAtUtc;
    std::string updatedAtUtc;

    static std::optional<DomainError> validate(std::int64_t groupId, const std::string& sessionDate) {
        if (groupId <= 0) {
            return DomainError(ErrorCode::ValidationFailed, "Valid group is required for session.");
        }
        if (sessionDate.empty()) {
            return DomainError(ErrorCode::ValidationFailed, "Session date cannot be empty.");
        }
        return std::nullopt;
    }

    [[nodiscard]] std::string statusToString() const {
        switch (status) {
            case SessionStatus::Completed: return "completed";
            case SessionStatus::Cancelled: return "cancelled";
            case SessionStatus::Rescheduled: return "rescheduled";
            default: return "scheduled";
        }
    }

    [[nodiscard]] std::string approvalStatusToString() const {
        switch (approvalStatus) {
            case SessionApprovalStatus::Approved: return "approved";
            case SessionApprovalStatus::Rejected: return "rejected";
            default: return "pending";
        }
    }

    [[nodiscard]] std::string settlementStatusToString() const {
        switch (settlementStatus) {
            case SessionSettlementStatus::FullyPaid: return "fully_paid";
            case SessionSettlementStatus::PartiallyPaid: return "partially_paid";
            default: return "unpaid";
        }
    }

    [[nodiscard]] static SessionSettlementStatus settlementStatusFromString(std::string_view str) noexcept {
        if (str == "fully_paid") return SessionSettlementStatus::FullyPaid;
        if (str == "partially_paid") return SessionSettlementStatus::PartiallyPaid;
        return SessionSettlementStatus::Unpaid;
    }

    void approve(const std::string& adminId, const std::string& nowUtc, CompensationType compType = CompensationType::Hourly) {
        approvalStatus = SessionApprovalStatus::Approved;
        approvedBy = adminId;
        approvedAtUtc = nowUtc;
        if (status == SessionStatus::Completed) {
            earnedAmount = calculateEarnings(payableDurationMinutes, hourlyRateSnapshot, compType);
        } else {
            earnedAmount = Money(0);
        }
    }

    [[nodiscard]] static Money calculateEarnings(int payableMinutes, Money rate, CompensationType compType = CompensationType::Hourly) noexcept {
        if (payableMinutes <= 0 || rate.piasters() <= 0) {
            return Money(0);
        }
        if (compType == CompensationType::PerSession) {
            return rate;
        }
        if (compType == CompensationType::MonthlySalary || compType == CompensationType::FixedContract) {
            // For Monthly Salary or Fixed Contract, teaching sessions track teaching delivery and attendance;
            // they do not generate per-session financial earnings!
            return Money(0);
        }
        // Hourly: (payableMinutes * hourlyRate) / 60
        // Calculate strictly with integer math to prevent floating point inaccuracies
        std::int64_t totalPiastres = (static_cast<std::int64_t>(payableMinutes) * rate.piasters()) / 60;
        return Money(totalPiastres);
    }
};

} // namespace pixcode::domain
