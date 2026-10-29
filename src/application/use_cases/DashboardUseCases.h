#pragma once

#include <memory>
#include <string>
#include "application/results/Result.h"
#include "application/dto/DashboardDtos.h"
#include "application/ports/IStudentRepository.h"
#include "application/ports/IGroupRepository.h"
#include "application/ports/ISessionRepository.h"
#include "application/ports/IAttendanceRepository.h"
#include "application/ports/IEnrollmentRepository.h"
#include "application/ports/IPaymentRepository.h"
#include "application/ports/IInstructorPayoutRepository.h"
#include "application/ports/IClock.h"
#include "application/authorization/IAuthorizationContext.h"

namespace pixcode::application {

class DashboardUseCases {
public:
    DashboardUseCases(
        std::shared_ptr<IStudentRepository> studentRepo,
        std::shared_ptr<IGroupRepository> groupRepo,
        std::shared_ptr<ISessionRepository> sessionRepo,
        std::shared_ptr<IAttendanceRepository> attendanceRepo,
        std::shared_ptr<IEnrollmentRepository> enrollmentRepo,
        std::shared_ptr<IPaymentRepository> paymentRepo,
        std::shared_ptr<IInstructorPayoutRepository> payoutRepo,
        std::shared_ptr<IClock> clock,
        std::shared_ptr<IAuthorizationContext> authContext);

    Result<DashboardSummaryDto> getDashboardSummary();

private:
    std::shared_ptr<IStudentRepository> m_studentRepo;
    std::shared_ptr<IGroupRepository> m_groupRepo;
    std::shared_ptr<ISessionRepository> m_sessionRepo;
    std::shared_ptr<IAttendanceRepository> m_attendanceRepo;
    std::shared_ptr<IEnrollmentRepository> m_enrollmentRepo;
    std::shared_ptr<IPaymentRepository> m_paymentRepo;
    std::shared_ptr<IInstructorPayoutRepository> m_payoutRepo;
    std::shared_ptr<IClock> m_clock;
    std::shared_ptr<IAuthorizationContext> m_authContext;
};

} // namespace pixcode::application

