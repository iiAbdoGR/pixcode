#include "InstructorPayrollUseCases.h"

namespace pixcode::application {

static domain::SessionStatus parseSessionStatus(const std::string& s) {
    if (s == "completed") return domain::SessionStatus::Completed;
    if (s == "cancelled") return domain::SessionStatus::Cancelled;
    if (s == "rescheduled") return domain::SessionStatus::Rescheduled;
    return domain::SessionStatus::Scheduled;
}

static std::string formatSessionStatus(domain::SessionStatus s) {
    switch (s) {
        case domain::SessionStatus::Completed: return "completed";
        case domain::SessionStatus::Cancelled: return "cancelled";
        case domain::SessionStatus::Rescheduled: return "rescheduled";
        default: return "scheduled";
    }
}

static std::string formatApprovalStatus(domain::SessionApprovalStatus s) {
    switch (s) {
        case domain::SessionApprovalStatus::Approved: return "approved";
        case domain::SessionApprovalStatus::Rejected: return "rejected";
        default: return "pending";
    }
}

InstructorPayrollUseCases::InstructorPayrollUseCases(
    std::shared_ptr<ISessionRepository> sessionRepo,
    std::shared_ptr<IInstructorPayoutRepository> payoutRepo,
    std::shared_ptr<IInstructorRepository> instructorRepo,
    std::shared_ptr<IGroupRepository> groupRepo,
    std::shared_ptr<ICourseRepository> courseRepo,
    std::shared_ptr<IClock> clock,
    std::shared_ptr<IAuthorizationContext> authContext,
    std::shared_ptr<AuditService> auditService)
    : m_sessionRepo(std::move(sessionRepo)),
      m_payoutRepo(std::move(payoutRepo)),
      m_instructorRepo(std::move(instructorRepo)),
      m_groupRepo(std::move(groupRepo)),
      m_courseRepo(std::move(courseRepo)),
      m_clock(std::move(clock)),
      m_authContext(std::move(authContext)),
      m_auditService(std::move(auditService)) {}

Result<SessionSummaryDto> InstructorPayrollUseCases::createSession(const CreateSessionCommand& cmd) {
    m_authContext->enforcePermission("session.create");

    auto groupOpt = m_groupRepo->findById(cmd.groupId);
    if (!groupOpt.has_value()) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Group not found"));
    }

    std::optional<std::int64_t> instId = cmd.instructorId;
    if (!instId.has_value() || *instId <= 0) {
        instId = groupOpt->instructorId;
    }

    const auto now = m_clock->nowUtcIso();
    domain::Session s;
    s.groupId = cmd.groupId;
    s.instructorId = instId;
    s.sessionDate = cmd.sessionDate;
    s.scheduledStartTime = cmd.scheduledStartTime;
    s.scheduledEndTime = cmd.scheduledEndTime;
    s.actualStartTime = cmd.scheduledStartTime;
    s.actualEndTime = cmd.scheduledEndTime;
    s.plannedDurationMinutes = cmd.plannedDurationMinutes > 0 ? cmd.plannedDurationMinutes : 60;
    s.actualDurationMinutes = s.plannedDurationMinutes;
    s.payableDurationMinutes = s.plannedDurationMinutes;
    s.status = domain::SessionStatus::Scheduled;
    s.approvalStatus = domain::SessionApprovalStatus::Pending;
    s.notes = cmd.notes;
    s.createdAtUtc = now;
    s.updatedAtUtc = now;

    auto saved = m_sessionRepo->save(s);
    return Result<SessionSummaryDto>::ok(buildSessionDto(saved));
}

Result<SessionSummaryDto> InstructorPayrollUseCases::updateSession(const UpdateSessionCommand& cmd) {
    m_authContext->enforcePermission("session.edit");

    auto sOpt = m_sessionRepo->findById(cmd.id);
    if (!sOpt.has_value()) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Session not found"));
    }

    auto s = *sOpt;
    if (cmd.instructorId.has_value()) s.instructorId = cmd.instructorId;
    if (!cmd.sessionDate.empty()) s.sessionDate = cmd.sessionDate;
    if (!cmd.actualStartTime.empty()) s.actualStartTime = cmd.actualStartTime;
    if (!cmd.actualEndTime.empty()) s.actualEndTime = cmd.actualEndTime;
    if (cmd.actualDurationMinutes > 0) s.actualDurationMinutes = cmd.actualDurationMinutes;
    if (cmd.payableDurationMinutes > 0) s.payableDurationMinutes = cmd.payableDurationMinutes;
    if (!cmd.status.empty()) s.status = parseSessionStatus(cmd.status);
    if (!cmd.notes.empty()) s.notes = cmd.notes;
    s.updatedAtUtc = m_clock->nowUtcIso();

    auto saved = m_sessionRepo->save(s);
    return Result<SessionSummaryDto>::ok(buildSessionDto(saved));
}

Result<SessionSummaryDto> InstructorPayrollUseCases::approveSession(const ApproveSessionCommand& cmd) {
    m_authContext->enforcePermission("payroll.calculate_approve");

    auto sOpt = m_sessionRepo->findById(cmd.sessionId);
    if (!sOpt.has_value()) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Session not found"));
    }

    auto s = *sOpt;
    s.payableDurationMinutes = cmd.approvedPayableMinutes;
    s.status = domain::SessionStatus::Completed;
    s.approvalStatus = domain::SessionApprovalStatus::Approved;
    s.approvedBy = cmd.approvedByUserId;
    s.approvedAtUtc = m_clock->nowUtcIso();
    s.updatedAtUtc = s.approvedAtUtc.value();

    if (s.instructorId.has_value() && *s.instructorId > 0) {
        auto instOpt = m_instructorRepo->findById(*s.instructorId);
        if (instOpt.has_value()) {
            s.hourlyRateSnapshot = instOpt->hourlyRate;
            s.earnedAmount = domain::Session::calculateEarnings(s.payableDurationMinutes, s.hourlyRateSnapshot);
        }
    }

    auto saved = m_sessionRepo->save(s);

    m_auditService->record(cmd.approvedByUserId, "SESSION_APPROVED", "Session",
                           std::to_string(cmd.sessionId),
                           "Approved teaching session with earned amount: " + s.earnedAmount.format());

    return Result<SessionSummaryDto>::ok(buildSessionDto(saved));
}

Result<std::vector<SessionSummaryDto>> InstructorPayrollUseCases::listSessions(const SessionFilter& filter) {
    auto sessions = m_sessionRepo->findAll(filter);
    std::vector<SessionSummaryDto> dtos;
    dtos.reserve(sessions.size());
    for (const auto& s : sessions) {
        dtos.push_back(buildSessionDto(s));
    }
    return Result<std::vector<SessionSummaryDto>>::ok(std::move(dtos));
}

Result<SessionSummaryDto> InstructorPayrollUseCases::getSession(std::int64_t sessionId) {
    auto sOpt = m_sessionRepo->findById(sessionId);
    if (!sOpt.has_value()) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Session not found"));
    }
    return Result<SessionSummaryDto>::ok(buildSessionDto(*sOpt));
}

Result<InstructorPayoutDto> InstructorPayrollUseCases::recordPayout(const RecordInstructorPayoutCommand& cmd) {
    m_authContext->enforcePermission("payroll.pay_instructor");

    auto instOpt = m_instructorRepo->findById(cmd.instructorId);
    if (!instOpt.has_value()) {
        return Result<InstructorPayoutDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Instructor not found"));
    }

    if (cmd.amountPiastres <= 0) {
        return Result<InstructorPayoutDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Payout amount must be positive"));
    }

    // Check outstanding dues
    std::int64_t grossEarned = m_sessionRepo->sumApprovedEarnings(cmd.instructorId);
    std::int64_t totalPaid = m_payoutRepo->totalPaidOut(cmd.instructorId);
    std::int64_t outstanding = grossEarned - totalPaid;

    if (cmd.amountPiastres > outstanding) {
        return Result<InstructorPayoutDto>::error(
            domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Payout amount exceeds approved outstanding dues"));
    }

    std::string receiptNo = m_payoutRepo->nextReceiptNumber();
    const auto now = m_clock->nowUtcIso();

    domain::InstructorPayout p;
    p.instructorId = cmd.instructorId;
    p.amount = domain::Money(cmd.amountPiastres);
    p.paymentDateUtc = cmd.paymentDateUtc.empty() ? now : cmd.paymentDateUtc;
    p.paymentMethod = (cmd.paymentMethod == "bank_transfer") ? domain::PayoutMethod::BankTransfer : domain::PayoutMethod::Cash;
    p.referenceNumber = cmd.referenceNumber;
    p.receiptNumber = receiptNo;
    p.notes = cmd.notes;
    p.createdBy = cmd.createdByUserId;
    p.approvedBy = cmd.approvedByUserId;
    p.createdAtUtc = now;

    auto saved = m_payoutRepo->save(p);

    std::string actorId = cmd.createdByUserId.value_or("system");
    m_auditService->record(actorId, "INSTRUCTOR_PAYOUT_RECORDED", "InstructorPayout",
                           std::to_string(saved.id),
                           "Issued payout receipt " + receiptNo + " for instructor: " + instOpt->fullName);

    InstructorPayoutDto dto;
    dto.id = saved.id;
    dto.instructorId = saved.instructorId;
    dto.instructorName = instOpt->fullName;
    dto.amountPiastres = saved.amount.piasters();
    dto.paymentDateUtc = saved.paymentDateUtc;
    dto.paymentMethod = cmd.paymentMethod;
    dto.referenceNumber = saved.referenceNumber;
    dto.receiptNumber = saved.receiptNumber;
    dto.notes = saved.notes;
    dto.createdAtUtc = saved.createdAtUtc;

    return Result<InstructorPayoutDto>::ok(dto);
}

Result<std::vector<InstructorPayoutDto>> InstructorPayrollUseCases::listPayouts(std::int64_t instructorId) {
    auto payouts = m_payoutRepo->findByInstructor(instructorId);
    auto instOpt = m_instructorRepo->findById(instructorId);
    std::string instName = instOpt.has_value() ? instOpt->fullName : "";

    std::vector<InstructorPayoutDto> dtos;
    dtos.reserve(payouts.size());

    for (const auto& p : payouts) {
        InstructorPayoutDto dto;
        dto.id = p.id;
        dto.instructorId = p.instructorId;
        dto.instructorName = instName;
        dto.amountPiastres = p.amount.piasters();
        dto.paymentDateUtc = p.paymentDateUtc;
        dto.paymentMethod = (p.paymentMethod == domain::PayoutMethod::BankTransfer) ? "bank_transfer" : "cash";
        dto.referenceNumber = p.referenceNumber;
        dto.receiptNumber = p.receiptNumber;
        dto.notes = p.notes;
        dto.createdAtUtc = p.createdAtUtc;

        dtos.push_back(std::move(dto));
    }

    return Result<std::vector<InstructorPayoutDto>>::ok(std::move(dtos));
}

Result<InstructorPayrollSummaryDto> InstructorPayrollUseCases::getInstructorPayrollSummary(std::int64_t instructorId) {
    auto instOpt = m_instructorRepo->findById(instructorId);
    if (!instOpt.has_value()) {
        return Result<InstructorPayrollSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Instructor not found"));
    }

    InstructorPayrollSummaryDto summary;
    summary.instructorId = instructorId;
    summary.instructorName = instOpt->fullName;
    summary.specialization = instOpt->specialization;
    summary.currentHourlyRatePiastres = instOpt->hourlyRate.piasters();

    SessionFilter fAll;
    fAll.instructorId = instructorId;
    summary.totalScheduledSessions = m_sessionRepo->count(fAll);

    summary.totalApprovedPayableMinutes = m_sessionRepo->sumApprovedMinutes(instructorId);
    summary.totalCompletedSessions = m_sessionRepo->countLessonsTaught(instructorId);
    summary.totalApprovedSessions = summary.totalCompletedSessions;

    summary.grossEarnedPiastres = m_sessionRepo->sumApprovedEarnings(instructorId);
    summary.totalPaidOutPiastres = m_payoutRepo->totalPaidOut(instructorId);
    summary.outstandingDuesPiastres = summary.grossEarnedPiastres - summary.totalPaidOutPiastres;

    return Result<InstructorPayrollSummaryDto>::ok(summary);
}

SessionSummaryDto InstructorPayrollUseCases::buildSessionDto(const domain::Session& s) {
    SessionSummaryDto dto;
    dto.id = s.id;
    dto.groupId = s.groupId;
    dto.instructorId = s.instructorId;
    dto.sessionDate = s.sessionDate;
    dto.scheduledStartTime = s.scheduledStartTime;
    dto.scheduledEndTime = s.scheduledEndTime;
    dto.status = formatSessionStatus(s.status);
    dto.approvalStatus = formatApprovalStatus(s.approvalStatus);
    dto.plannedDurationMinutes = s.plannedDurationMinutes;
    dto.actualDurationMinutes = s.actualDurationMinutes;
    dto.payableDurationMinutes = s.payableDurationMinutes;
    dto.earnedAmountPiastres = s.earnedAmount.piasters();

    auto gOpt = m_groupRepo->findById(s.groupId);
    if (gOpt.has_value()) {
        dto.groupName = gOpt->name;
        auto cOpt = m_courseRepo->findById(gOpt->courseId);
        if (cOpt.has_value()) {
            dto.courseName = cOpt->name;
        }
        dto.totalEnrolledStudents = m_groupRepo->countActiveEnrollments(s.groupId);
    }

    if (s.instructorId.has_value() && *s.instructorId > 0) {
        auto instOpt = m_instructorRepo->findById(*s.instructorId);
        if (instOpt.has_value()) {
            dto.instructorName = instOpt->fullName;
        }
    }

    return dto;
}

} // namespace pixcode::application
