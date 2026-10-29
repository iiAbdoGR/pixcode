#pragma once

#include <vector>
#include <string>
#include <memory>
#include "application/results/Result.h"
#include "application/dto/PayrollDtos.h"
#include "application/dto/AttendanceDtos.h"
#include "application/dto/EnrollmentDtos.h"
#include "application/ports/ISessionRepository.h"
#include "application/ports/IInstructorPayoutRepository.h"
#include "application/ports/IInstructorRepository.h"
#include "application/ports/IGroupRepository.h"
#include "application/ports/ICourseRepository.h"
#include "application/ports/IClock.h"
#include "application/authorization/IAuthorizationContext.h"
#include "application/use_cases/AuditService.h"

namespace pixcode::application {

class InstructorPayrollUseCases {
public:
    InstructorPayrollUseCases(
        std::shared_ptr<ISessionRepository> sessionRepo,
        std::shared_ptr<IInstructorPayoutRepository> payoutRepo,
        std::shared_ptr<IInstructorRepository> instructorRepo,
        std::shared_ptr<IGroupRepository> groupRepo,
        std::shared_ptr<ICourseRepository> courseRepo,
        std::shared_ptr<IClock> clock,
        std::shared_ptr<IAuthorizationContext> authContext,
        std::shared_ptr<AuditService> auditService);

    Result<SessionSummaryDto> createSession(const CreateSessionCommand& cmd);
    Result<SessionSummaryDto> updateSession(const UpdateSessionCommand& cmd);
    Result<SessionSummaryDto> approveSession(const ApproveSessionCommand& cmd);
    Result<std::vector<SessionSummaryDto>> listSessions(const SessionFilter& filter);
    Result<SessionSummaryDto> getSession(std::int64_t sessionId);

    Result<InstructorPayoutDto> recordPayout(const RecordInstructorPayoutCommand& cmd);
    Result<std::vector<InstructorPayoutDto>> listPayouts(std::int64_t instructorId);
    Result<InstructorPayrollSummaryDto> getInstructorPayrollSummary(std::int64_t instructorId);

    // Instructor Profile Management (Decoupled non-login staff)
    Result<InstructorDto> createInstructor(const CreateInstructorCommand& cmd);
    Result<InstructorDto> updateInstructor(const UpdateInstructorCommand& cmd);
    Result<std::vector<InstructorDto>> listInstructors(const InstructorFilter& filter = {});
    Result<InstructorDto> getInstructor(std::int64_t id);

private:
    std::shared_ptr<ISessionRepository> m_sessionRepo;
    std::shared_ptr<IInstructorPayoutRepository> m_payoutRepo;
    std::shared_ptr<IInstructorRepository> m_instructorRepo;
    std::shared_ptr<IGroupRepository> m_groupRepo;
    std::shared_ptr<ICourseRepository> m_courseRepo;
    std::shared_ptr<IClock> m_clock;
    std::shared_ptr<IAuthorizationContext> m_authContext;
    std::shared_ptr<AuditService> m_auditService;

    SessionSummaryDto buildSessionDto(const domain::Session& s);
    static InstructorDto toInstructorDto(const domain::Instructor& inst);
};

} // namespace pixcode::application
