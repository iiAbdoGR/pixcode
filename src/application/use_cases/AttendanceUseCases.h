#pragma once

#include <vector>
#include <string>
#include <memory>
#include "application/results/Result.h"
#include "application/dto/AttendanceDtos.h"
#include "application/ports/IAttendanceRepository.h"
#include "application/ports/ISessionRepository.h"
#include "application/ports/IStudentRepository.h"
#include "application/ports/IGroupRepository.h"
#include "application/ports/ICourseRepository.h"
#include "application/ports/IClock.h"
#include "application/authorization/IAuthorizationContext.h"
#include "application/use_cases/AuditService.h"

namespace pixcode::application {

class AttendanceUseCases {
public:
    AttendanceUseCases(std::shared_ptr<IAttendanceRepository> attendanceRepo,
                       std::shared_ptr<ISessionRepository> sessionRepo,
                       std::shared_ptr<IStudentRepository> studentRepo,
                       std::shared_ptr<IGroupRepository> groupRepo,
                       std::shared_ptr<ICourseRepository> courseRepo,
                       std::shared_ptr<IClock> clock,
                       std::shared_ptr<IAuthorizationContext> authContext,
                       std::shared_ptr<AuditService> auditService);

    Result<void> recordSessionAttendance(const RecordSessionAttendanceCommand& cmd);
    Result<std::vector<SessionAttendanceDto>> getSessionAttendance(std::int64_t sessionId);
    Result<SessionAttendanceStatsDto> getSessionStats(std::int64_t sessionId);
    Result<StudentAttendanceStatsDto> getStudentStats(std::int64_t studentId);
    Result<std::vector<SessionAttendanceDto>> getStudentAttendanceHistory(std::int64_t studentId);

private:
    std::shared_ptr<IAttendanceRepository> m_attendanceRepo;
    std::shared_ptr<ISessionRepository> m_sessionRepo;
    std::shared_ptr<IStudentRepository> m_studentRepo;
    std::shared_ptr<IGroupRepository> m_groupRepo;
    std::shared_ptr<ICourseRepository> m_courseRepo;
    std::shared_ptr<IClock> m_clock;
    std::shared_ptr<IAuthorizationContext> m_authContext;
    std::shared_ptr<AuditService> m_auditService;
};

} // namespace pixcode::application
