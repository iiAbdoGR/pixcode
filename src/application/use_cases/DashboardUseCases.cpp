#include "DashboardUseCases.h"
#include <algorithm>

namespace pixcode::application {

DashboardUseCases::DashboardUseCases(
    std::shared_ptr<IStudentRepository> studentRepo,
    std::shared_ptr<IGroupRepository> groupRepo,
    std::shared_ptr<ISessionRepository> sessionRepo,
    std::shared_ptr<IAttendanceRepository> attendanceRepo,
    std::shared_ptr<IEnrollmentRepository> enrollmentRepo,
    std::shared_ptr<IPaymentRepository> paymentRepo,
    std::shared_ptr<IInstructorPayoutRepository> payoutRepo,
    std::shared_ptr<IClock> clock,
    std::shared_ptr<IAuthorizationContext> authContext)
    : m_studentRepo(std::move(studentRepo)),
      m_groupRepo(std::move(groupRepo)),
      m_sessionRepo(std::move(sessionRepo)),
      m_attendanceRepo(std::move(attendanceRepo)),
      m_enrollmentRepo(std::move(enrollmentRepo)),
      m_paymentRepo(std::move(paymentRepo)),
      m_payoutRepo(std::move(payoutRepo)),
      m_clock(std::move(clock)),
      m_authContext(std::move(authContext)) {}

Result<DashboardSummaryDto> DashboardUseCases::getDashboardSummary() {
    DashboardSummaryDto summary;

    bool isOwner = false;
    if (m_authContext && m_authContext->currentUser().has_value()) {
        isOwner = m_authContext->currentUser()->isOwner();
    }
    summary.isOwner = isOwner;

    // 1. Operational Metrics (Available for all authorized users)
    StudentFilter sf;
    sf.status = domain::StudentStatus::Active;
    summary.operational.activeStudentsCount = m_studentRepo->count(sf);

    GroupFilter gf;
    gf.status = domain::GroupStatus::Open;
    summary.operational.activeGroupsCount = m_groupRepo->count(gf);

    std::string today = m_clock->nowUtcIso().substr(0, 10);
    SessionFilter sessF;
    sessF.fromDate = today;
    sessF.toDate = today;
    summary.operational.todaySessionsCount = m_sessionRepo->count(sessF);

    int totalAtt = m_attendanceRepo->countTotalRecords();
    int presentAtt = m_attendanceRepo->countPresentRecords();
    if (totalAtt > 0) {
        summary.operational.attendanceRatePercentage = (static_cast<double>(presentAtt) / totalAtt) * 100.0;
    } else {
        summary.operational.attendanceRatePercentage = 0.0;
    }

    // 2. Financial Metrics: ONLY populated if isOwner
    if (isOwner) {
        FinancialMetricsDto fin;
        fin.totalIncomePiastres = m_paymentRepo->totalAllPayments();
        fin.totalPayoutsMadePiastres = m_payoutRepo->totalAllPayouts();
        fin.netCashBalancePiastres = fin.totalIncomePiastres - fin.totalPayoutsMadePiastres;

        std::int64_t totalApprovedEarnings = m_sessionRepo->sumAllApprovedEarnings();
        fin.approvedInstructorPayablesPiastres = std::max<std::int64_t>(0, totalApprovedEarnings - fin.totalPayoutsMadePiastres);

        auto enrollments = m_enrollmentRepo->findAll();
        std::int64_t totalReceivables = 0;
        for (const auto& e : enrollments) {
            if (e.status == domain::EnrollmentStatus::Active) {
                std::int64_t paid = m_paymentRepo->totalPaid(e.id);
                std::int64_t bal = e.netFee.piasters() - paid;
                if (bal > 0) {
                    totalReceivables += bal;
                }
            }
        }
        fin.totalReceivablesPiastres = totalReceivables;

        summary.financial = fin;
    } else {
        summary.financial = std::nullopt;
    }

    return Result<DashboardSummaryDto>::ok(std::move(summary));
}

} // namespace pixcode::application

