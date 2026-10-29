#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace pixcode::application {

struct RecordStudentAttendanceItem {
    std::int64_t studentId{0};
    std::string status{"present"}; // "present", "absent", "late", "excused"
    std::string arrivalTime;
    std::string notes;
};

struct RecordSessionAttendanceCommand {
    std::int64_t sessionId{0};
    std::vector<RecordStudentAttendanceItem> items;
    std::optional<std::string> markedByUserId;
};

struct SessionAttendanceDto {
    std::int64_t id{0};
    std::int64_t sessionId{0};
    std::int64_t studentId{0};
    std::string studentCode;
    std::string studentName;
    std::string sessionDate;
    std::string groupName;
    std::string status; // "present", "absent", "late", "excused"
    std::string arrivalTime;
    std::string notes;
    std::string markedAtUtc;
};

struct SessionAttendanceStatsDto {
    std::int64_t sessionId{0};
    int totalStudents{0};
    int presentCount{0};
    int lateCount{0};
    int excusedCount{0};
    int absentCount{0};
    double attendancePercentage{0.0};
};

struct StudentAttendanceStatsDto {
    std::int64_t studentId{0};
    int totalSessions{0};
    int presentCount{0};
    int lateCount{0};
    int excusedCount{0};
    int absentCount{0};
    double attendancePercentage{0.0};
};

struct SessionSummaryDto {
    std::int64_t id{0};
    std::int64_t groupId{0};
    std::string groupName;
    std::string courseName;
    std::optional<std::int64_t> instructorId;
    std::string instructorName;
    std::string sessionDate;
    std::string scheduledStartTime;
    std::string scheduledEndTime;
    std::string status;           // "scheduled", "completed", "cancelled", "rescheduled"
    std::string approvalStatus;   // "pending", "approved", "rejected"
    std::string settlementStatus; // "unpaid", "partially_paid", "fully_paid"
    int plannedDurationMinutes{60};
    int actualDurationMinutes{60};
    int payableDurationMinutes{60};
    std::int64_t earnedAmountPiastres{0};
    int totalEnrolledStudents{0};
    int attendedStudentsCount{0};
};

} // namespace pixcode::application
