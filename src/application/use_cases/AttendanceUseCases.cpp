#include "AttendanceUseCases.h"

namespace pixcode::application {

static domain::AttendanceStatus parseStatus(const std::string& s) {
    if (s == "absent") return domain::AttendanceStatus::Absent;
    if (s == "late") return domain::AttendanceStatus::Late;
    if (s == "excused") return domain::AttendanceStatus::Excused;
    return domain::AttendanceStatus::Present;
}

static std::string formatStatus(domain::AttendanceStatus s) {
    switch (s) {
        case domain::AttendanceStatus::Absent: return "absent";
        case domain::AttendanceStatus::Late: return "late";
        case domain::AttendanceStatus::Excused: return "excused";
        default: return "present";
    }
}

AttendanceUseCases::AttendanceUseCases(
    std::shared_ptr<IAttendanceRepository> attendanceRepo,
    std::shared_ptr<ISessionRepository> sessionRepo,
    std::shared_ptr<IStudentRepository> studentRepo,
    std::shared_ptr<IGroupRepository> groupRepo,
    std::shared_ptr<ICourseRepository> courseRepo,
    std::shared_ptr<IClock> clock,
    std::shared_ptr<IAuthorizationContext> authContext,
    std::shared_ptr<AuditService> auditService)
    : m_attendanceRepo(std::move(attendanceRepo)),
      m_sessionRepo(std::move(sessionRepo)),
      m_studentRepo(std::move(studentRepo)),
      m_groupRepo(std::move(groupRepo)),
      m_courseRepo(std::move(courseRepo)),
      m_clock(std::move(clock)),
      m_authContext(std::move(authContext)),
      m_auditService(std::move(auditService)) {}

Result<void> AttendanceUseCases::recordSessionAttendance(const RecordSessionAttendanceCommand& cmd) {
    m_authContext->enforcePermission("attendance.student.mark");

    auto sessionOpt = m_sessionRepo->findById(cmd.sessionId);
    if (!sessionOpt.has_value()) {
        return Result<void>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Session not found"));
    }

    const auto now = m_clock->nowUtcIso();
    std::vector<domain::SessionAttendance> records;
    records.reserve(cmd.items.size());

    for (const auto& item : cmd.items) {
        domain::SessionAttendance att;
        att.sessionId = cmd.sessionId;
        att.studentId = item.studentId;
        att.status = parseStatus(item.status);
        att.arrivalTime = item.arrivalTime;
        att.notes = item.notes;
        att.markedBy = cmd.markedByUserId;
        att.markedAtUtc = now;
        att.updatedAtUtc = now;

        records.push_back(std::move(att));
    }

    if (!m_attendanceRepo->saveBatch(records)) {
        return Result<void>::error(
            domain::DomainError(domain::ErrorCode::DatabaseError, "Failed to save attendance batch"));
    }

    // Update session status to completed if it was scheduled
    auto session = *sessionOpt;
    if (session.status == domain::SessionStatus::Scheduled) {
        session.status = domain::SessionStatus::Completed;
        session.updatedAtUtc = now;
        m_sessionRepo->save(session);
    }

    std::string actorId = m_authContext->currentUser().has_value() ? m_authContext->currentUser()->id() : "system";
    m_auditService->record(actorId, "ATTENDANCE_RECORDED", "Session", std::to_string(cmd.sessionId),
                           "Recorded attendance for " + std::to_string(cmd.items.size()) + " students");

    return Result<void>::ok();
}

Result<std::vector<SessionAttendanceDto>> AttendanceUseCases::getSessionAttendance(std::int64_t sessionId) {
    auto records = m_attendanceRepo->findBySession(sessionId);
    std::vector<SessionAttendanceDto> dtos;
    dtos.reserve(records.size());

    for (const auto& r : records) {
        SessionAttendanceDto dto;
        dto.id = r.id;
        dto.sessionId = r.sessionId;
        dto.studentId = r.studentId;
        dto.status = formatStatus(r.status);
        dto.arrivalTime = r.arrivalTime;
        dto.notes = r.notes;
        dto.markedAtUtc = r.markedAtUtc;

        auto studentOpt = m_studentRepo->findById(r.studentId);
        if (studentOpt.has_value()) {
            dto.studentCode = studentOpt->studentCode;
            dto.studentName = studentOpt->fullName;
        }

        dtos.push_back(std::move(dto));
    }

    return Result<std::vector<SessionAttendanceDto>>::ok(std::move(dtos));
}

Result<SessionAttendanceStatsDto> AttendanceUseCases::getSessionStats(std::int64_t sessionId) {
    auto records = m_attendanceRepo->findBySession(sessionId);
    SessionAttendanceStatsDto stats;
    stats.sessionId = sessionId;
    stats.totalStudents = static_cast<int>(records.size());

    for (const auto& r : records) {
        if (r.status == domain::AttendanceStatus::Present) stats.presentCount++;
        else if (r.status == domain::AttendanceStatus::Late) stats.lateCount++;
        else if (r.status == domain::AttendanceStatus::Excused) stats.excusedCount++;
        else if (r.status == domain::AttendanceStatus::Absent) stats.absentCount++;
    }

    int totalValid = stats.presentCount + stats.lateCount + stats.excusedCount + stats.absentCount;
    if (totalValid > 0) {
        stats.attendancePercentage = ((stats.presentCount + stats.lateCount + stats.excusedCount) * 100.0) / totalValid;
    } else {
        stats.attendancePercentage = 100.0;
    }

    return Result<SessionAttendanceStatsDto>::ok(stats);
}

Result<StudentAttendanceStatsDto> AttendanceUseCases::getStudentStats(std::int64_t studentId) {
    StudentAttendanceStatsDto stats;
    stats.studentId = studentId;

    stats.presentCount = m_attendanceRepo->countStudentStatus(studentId, domain::AttendanceStatus::Present);
    stats.lateCount = m_attendanceRepo->countStudentStatus(studentId, domain::AttendanceStatus::Late);
    stats.excusedCount = m_attendanceRepo->countStudentStatus(studentId, domain::AttendanceStatus::Excused);
    stats.absentCount = m_attendanceRepo->countStudentStatus(studentId, domain::AttendanceStatus::Absent);

    int totalEligible = stats.presentCount + stats.lateCount + stats.excusedCount + stats.absentCount;
    stats.totalSessions = totalEligible;

    if (totalEligible > 0) {
        // Attendance percentage: (present + late + excused) / totalEligible * 100
        stats.attendancePercentage = ((stats.presentCount + stats.lateCount + stats.excusedCount) * 100.0) / totalEligible;
    } else {
        stats.attendancePercentage = 100.0;
    }

    return Result<StudentAttendanceStatsDto>::ok(stats);
}

Result<std::vector<SessionAttendanceDto>> AttendanceUseCases::getStudentAttendanceHistory(std::int64_t studentId) {
    auto records = m_attendanceRepo->findByStudent(studentId);
    std::vector<SessionAttendanceDto> dtos;
    dtos.reserve(records.size());

    for (const auto& r : records) {
        SessionAttendanceDto dto;
        dto.id = r.id;
        dto.sessionId = r.sessionId;
        dto.studentId = r.studentId;
        dto.status = formatStatus(r.status);
        dto.arrivalTime = r.arrivalTime;
        dto.notes = r.notes;
        dto.markedAtUtc = r.markedAtUtc;

        auto sessOpt = m_sessionRepo->findById(r.sessionId);
        if (sessOpt.has_value()) {
            dto.sessionDate = sessOpt->sessionDate;
            auto grpOpt = m_groupRepo->findById(sessOpt->groupId);
            if (grpOpt.has_value()) {
                dto.groupName = grpOpt->name;
            }
        }

        dtos.push_back(std::move(dto));
    }

    return Result<std::vector<SessionAttendanceDto>>::ok(std::move(dtos));
}

} // namespace pixcode::application
