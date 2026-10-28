#pragma once

#include <string>
#include <cstdint>
#include <optional>
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

    void approve(const std::string& adminId, const std::string& nowUtc) {
        approvalStatus = SessionApprovalStatus::Approved;
        approvedBy = adminId;
        approvedAtUtc = nowUtc;
        earnedAmount = calculateEarnings(payableDurationMinutes, hourlyRateSnapshot);
    }

    [[nodiscard]] static Money calculateEarnings(int payableMinutes, Money hourlyRate) noexcept {
        if (payableMinutes <= 0 || hourlyRate.piasters() <= 0) {
            return Money(0);
        }
        // Formula: (payableMinutes * hourlyRate) / 60
        // Calculate strictly with integer math to prevent floating point inaccuracies
        std::int64_t totalPiastres = (static_cast<std::int64_t>(payableMinutes) * hourlyRate.piasters()) / 60;
        return Money(totalPiastres);
    }
};

} // namespace pixcode::domain
