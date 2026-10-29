#include "InstructorPayrollUseCases.h"
#include <sstream>
#include <iomanip>

namespace pixcode::application {

namespace {

domain::SessionStatus parseSessionStatus(const std::string& str) {
    if (str == "completed") return domain::SessionStatus::Completed;
    if (str == "cancelled") return domain::SessionStatus::Cancelled;
    if (str == "rescheduled") return domain::SessionStatus::Rescheduled;
    return domain::SessionStatus::Scheduled;
}

std::string formatSessionStatus(domain::SessionStatus st) {
    switch (st) {
        case domain::SessionStatus::Completed: return "completed";
        case domain::SessionStatus::Cancelled: return "cancelled";
        case domain::SessionStatus::Rescheduled: return "rescheduled";
        default: return "scheduled";
    }
}

std::string formatApprovalStatus(domain::SessionApprovalStatus st) {
    switch (st) {
        case domain::SessionApprovalStatus::Approved: return "approved";
        case domain::SessionApprovalStatus::Rejected: return "rejected";
        default: return "pending";
    }
}

} // namespace

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
    m_authContext->enforcePermission("groups.manage");

    if (cmd.groupId <= 0) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Group ID is required"));
    }
    if (cmd.sessionDate.empty()) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Session date is required"));
    }
    if (!cmd.scheduledStartTime.empty() && !cmd.scheduledEndTime.empty() && cmd.scheduledStartTime >= cmd.scheduledEndTime) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Scheduled end time must be after start time"));
    }

    try {
        domain::Session s;
        s.groupId = cmd.groupId;
        s.instructorId = cmd.instructorId;
        s.sessionDate = cmd.sessionDate;
        s.scheduledStartTime = cmd.scheduledStartTime;
        s.scheduledEndTime = cmd.scheduledEndTime;
        s.plannedDurationMinutes = (cmd.plannedDurationMinutes > 0) ? cmd.plannedDurationMinutes : 60;
        s.actualDurationMinutes = s.plannedDurationMinutes;
        s.payableDurationMinutes = s.plannedDurationMinutes;
        s.status = domain::SessionStatus::Scheduled;
        s.approvalStatus = domain::SessionApprovalStatus::Pending;
        s.notes = cmd.notes;

        const auto now = m_clock->nowUtcIso();
        s.createdAtUtc = now;
        s.updatedAtUtc = now;

        // Snapshot current instructor rate if assigned
        if (s.instructorId.has_value() && *s.instructorId > 0) {
            auto instOpt = m_instructorRepo->findById(*s.instructorId);
            if (instOpt.has_value()) {
                s.hourlyRateSnapshot = instOpt->baseRate.piasters() > 0 ? instOpt->baseRate : instOpt->hourlyRate;
            }
        }

        auto saved = m_sessionRepo->save(s);
        return Result<SessionSummaryDto>::ok(buildSessionDto(saved));
    } catch (const std::exception& e) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::DatabaseError, e.what()));
    }
}

Result<SessionSummaryDto> InstructorPayrollUseCases::updateSession(const UpdateSessionCommand& cmd) {
    m_authContext->enforcePermission("groups.manage");

    auto sOpt = m_sessionRepo->findById(cmd.id);
    if (!sOpt.has_value()) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Session not found"));
    }

    try {
        auto s = *sOpt;
        if (cmd.instructorId.has_value()) {
            s.instructorId = cmd.instructorId;
            if (*cmd.instructorId > 0) {
                auto instOpt = m_instructorRepo->findById(*cmd.instructorId);
                if (instOpt.has_value()) {
                    s.hourlyRateSnapshot = instOpt->baseRate.piasters() > 0 ? instOpt->baseRate : instOpt->hourlyRate;
                }
            }
        }
        if (!cmd.sessionDate.empty()) s.sessionDate = cmd.sessionDate;
        if (!cmd.scheduledStartTime.empty()) s.scheduledStartTime = cmd.scheduledStartTime;
        if (!cmd.scheduledEndTime.empty()) s.scheduledEndTime = cmd.scheduledEndTime;
        if (cmd.plannedDurationMinutes > 0) s.plannedDurationMinutes = cmd.plannedDurationMinutes;
        if (!s.scheduledStartTime.empty() && !s.scheduledEndTime.empty() && s.scheduledStartTime >= s.scheduledEndTime) {
            return Result<SessionSummaryDto>::error(
                domain::DomainError(domain::ErrorCode::ValidationFailed, "Scheduled end time must be after start time"));
        }
        if (!cmd.actualStartTime.empty()) s.actualStartTime = cmd.actualStartTime;
        if (!cmd.actualEndTime.empty()) s.actualEndTime = cmd.actualEndTime;
        if (cmd.actualDurationMinutes > 0) s.actualDurationMinutes = cmd.actualDurationMinutes;
        if (cmd.payableDurationMinutes > 0) s.payableDurationMinutes = cmd.payableDurationMinutes;
        if (!cmd.status.empty()) s.status = parseSessionStatus(cmd.status);
        if (!cmd.notes.empty()) s.notes = cmd.notes;
        s.updatedAtUtc = m_clock->nowUtcIso();

        auto saved = m_sessionRepo->save(s);
        return Result<SessionSummaryDto>::ok(buildSessionDto(saved));
    } catch (const std::exception& e) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::DatabaseError, e.what()));
    }
}

Result<SessionSummaryDto> InstructorPayrollUseCases::approveSession(const ApproveSessionCommand& cmd) {
    if (!m_authContext->hasPermission("payroll.calculate_approve")) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::PermissionDenied,
                                "غير مصرح: اعتماد الحصص واحتساب المستحقات من صلاحية المالك فقط."));
    }

    auto sOpt = m_sessionRepo->findById(cmd.sessionId);
    if (!sOpt.has_value()) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "الحصة غير موجودة."));
    }

    if (sOpt->approvalStatus == domain::SessionApprovalStatus::Approved) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "الحصة معتمدة بالفعل ولا يمكن اعتمادها مرتين."));
    }

    try {
        auto s = *sOpt;
        s.payableDurationMinutes = cmd.approvedPayableMinutes;
        s.status = domain::SessionStatus::Completed;
        s.approvalStatus = domain::SessionApprovalStatus::Approved;
        s.settlementStatus = domain::SessionSettlementStatus::Unpaid;
        s.approvedBy = cmd.approvedByUserId;
        s.approvedAtUtc = m_clock->nowUtcIso();
        s.updatedAtUtc = s.approvedAtUtc.value();

        if (s.instructorId.has_value() && *s.instructorId > 0) {
            auto instOpt = m_instructorRepo->findById(*s.instructorId);
            if (instOpt.has_value()) {
                s.hourlyRateSnapshot = instOpt->baseRate.piasters() > 0 ? instOpt->baseRate : instOpt->hourlyRate;
                s.earnedAmount = domain::Session::calculateEarnings(s.payableDurationMinutes, s.hourlyRateSnapshot, instOpt->compensationType);
            }
        }

        auto saved = m_sessionRepo->save(s);

        m_auditService->record(cmd.approvedByUserId, "SESSION_APPROVED", "Session",
                               std::to_string(cmd.sessionId),
                               "Approved teaching session with earned amount: " + s.earnedAmount.format());

        return Result<SessionSummaryDto>::ok(buildSessionDto(saved));
    } catch (const std::exception& e) {
        return Result<SessionSummaryDto>::error(
            domain::DomainError(domain::ErrorCode::DatabaseError, e.what()));
    }
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

    if (cmd.instructorId <= 0) {
        return Result<InstructorPayoutDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Valid instructor is required"));
    }
    if (cmd.amountPiastres <= 0) {
        return Result<InstructorPayoutDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Payout amount must be greater than zero"));
    }

    auto instOpt = m_instructorRepo->findById(cmd.instructorId);
    if (!instOpt.has_value()) {
        return Result<InstructorPayoutDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Instructor not found"));
    }

    // Check outstanding balance to prevent overpaying
    std::int64_t approvedEarnings = m_sessionRepo->sumApprovedEarnings(cmd.instructorId);
    std::int64_t alreadyPaidOut = m_payoutRepo->totalPaidOut(cmd.instructorId);
    std::int64_t outstanding = approvedEarnings - alreadyPaidOut;

    if (cmd.amountPiastres > outstanding) {
        return Result<InstructorPayoutDto>::error(
            domain::DomainError(domain::ErrorCode::BusinessRuleViolation,
                                "Payout amount exceeds approved outstanding dues"));
    }

    domain::InstructorPayout payout;
    payout.instructorId = cmd.instructorId;
    payout.amount = domain::Money(cmd.amountPiastres);
    payout.paymentDateUtc = cmd.paymentDateUtc.empty() ? m_clock->nowUtcIso() : cmd.paymentDateUtc;
    payout.paymentMethod = (cmd.paymentMethod == "bank_transfer")
        ? domain::PayoutMethod::BankTransfer
        : domain::PayoutMethod::Cash;
    payout.referenceNumber = cmd.referenceNumber;
    payout.notes = cmd.notes;
    payout.createdBy = cmd.createdByUserId;
    payout.approvedBy = cmd.approvedByUserId;
    payout.receiptNumber = m_payoutRepo->nextReceiptNumber();
    payout.createdAtUtc = m_clock->nowUtcIso();

    auto saved = m_payoutRepo->save(payout);

    // Allocate payout amount across eligible approved sessions that are unpaid or partially paid
    std::int64_t remainingToAllocate = cmd.amountPiastres;
    SessionFilter fSess;
    fSess.instructorId = cmd.instructorId;
    auto instructorSessions = m_sessionRepo->findAll(fSess);
    for (auto& s : instructorSessions) {
        if (remainingToAllocate <= 0) break;
        if (s.approvalStatus != domain::SessionApprovalStatus::Approved) continue;
        if (s.settlementStatus == domain::SessionSettlementStatus::FullyPaid) continue;
        if (s.earnedAmount.piasters() <= 0) continue;

        std::int64_t sessionEarned = s.earnedAmount.piasters();
        std::int64_t alloc = std::min(remainingToAllocate, sessionEarned);

        domain::InstructorPayoutAllocation allocation;
        allocation.payoutId = saved.id;
        allocation.sessionId = s.id;
        allocation.allocatedAmount = domain::Money(alloc);
        allocation.createdAtUtc = saved.createdAtUtc;
        m_payoutRepo->saveAllocation(allocation);

        if (alloc >= sessionEarned) {
            s.settlementStatus = domain::SessionSettlementStatus::FullyPaid;
        } else {
            s.settlementStatus = domain::SessionSettlementStatus::PartiallyPaid;
        }
        m_sessionRepo->save(s);

        remainingToAllocate -= alloc;
    }

    std::string actorId = cmd.createdByUserId.value_or("system");
    m_auditService->record(actorId, "INSTRUCTOR_PAYOUT_RECORDED", "InstructorPayout",
                           std::to_string(saved.id),
                           "Disbursed payout receipt #" + saved.receiptNumber + " of " + saved.amount.format() +
                           " to " + instOpt->fullName);

    InstructorPayoutDto dto;
    dto.id = saved.id;
    dto.instructorId = saved.instructorId;
    dto.instructorName = instOpt->fullName;
    dto.amountPiastres = saved.amount.piasters();
    dto.paymentDateUtc = saved.paymentDateUtc;
    dto.paymentMethod = (saved.paymentMethod == domain::PayoutMethod::BankTransfer) ? "bank_transfer" : "cash";
    dto.referenceNumber = saved.referenceNumber;
    dto.receiptNumber = saved.receiptNumber;
    dto.notes = saved.notes;
    dto.createdAtUtc = saved.createdAtUtc;

    return Result<InstructorPayoutDto>::ok(std::move(dto));
}

Result<std::vector<InstructorPayoutDto>> InstructorPayrollUseCases::listPayouts(std::int64_t instructorId) {
    m_authContext->enforcePermission("payroll.read");

    auto list = m_payoutRepo->findByInstructor(instructorId);
    std::string instName;
    if (auto instOpt = m_instructorRepo->findById(instructorId); instOpt.has_value()) {
        instName = instOpt->fullName;
    }

    std::vector<InstructorPayoutDto> dtos;
    dtos.reserve(list.size());
    for (const auto& p : list) {
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
    summary.compensationType = domain::compensationTypeToString(instOpt->compensationType);
    summary.currentHourlyRatePiastres = instOpt->baseRate.piasters() > 0 ? instOpt->baseRate.piasters() : instOpt->hourlyRate.piasters();
    summary.monthlySalaryPiastres = instOpt->monthlySalary.piasters() > 0 ? instOpt->monthlySalary.piasters() : summary.currentHourlyRatePiastres;

    SessionFilter fAll;
    fAll.instructorId = instructorId;
    summary.totalScheduledSessions = m_sessionRepo->count(fAll);

    auto sessions = m_sessionRepo->findAll(fAll);
    int pendingAppr = 0;
    int approvedUnpaid = 0;
    int settled = 0;
    for (const auto& s : sessions) {
        if (s.approvalStatus == domain::SessionApprovalStatus::Pending && s.status == domain::SessionStatus::Completed) {
            pendingAppr++;
        } else if (s.approvalStatus == domain::SessionApprovalStatus::Approved) {
            if (s.settlementStatus == domain::SessionSettlementStatus::FullyPaid) {
                settled++;
            } else {
                approvedUnpaid++;
            }
        }
    }
    summary.pendingApprovalSessionsCount = pendingAppr;
    summary.approvedUnpaidSessionsCount = approvedUnpaid;
    summary.settledSessionsCount = settled;

    summary.totalApprovedPayableMinutes = m_sessionRepo->sumApprovedMinutes(instructorId);
    summary.totalCompletedSessions = m_sessionRepo->countLessonsTaught(instructorId);
    summary.totalApprovedSessions = summary.totalCompletedSessions;

    summary.grossEarnedPiastres = m_sessionRepo->sumApprovedEarnings(instructorId);
    summary.totalPaidOutPiastres = m_payoutRepo->totalPaidOut(instructorId);
    summary.outstandingDuesPiastres = summary.grossEarnedPiastres - summary.totalPaidOutPiastres;
    if (summary.outstandingDuesPiastres < 0) {
        summary.outstandingDuesPiastres = 0;
    }

    if (summary.grossEarnedPiastres > 0 && summary.outstandingDuesPiastres <= 0) {
        summary.settlementStatus = "fully_paid";
    } else if (summary.totalPaidOutPiastres > 0 && summary.outstandingDuesPiastres > 0) {
        summary.settlementStatus = "partially_paid";
    } else {
        summary.settlementStatus = "unpaid";
    }

    return Result<InstructorPayrollSummaryDto>::ok(summary);
}

Result<InstructorDto> InstructorPayrollUseCases::createInstructor(const CreateInstructorCommand& cmd) {
    if (cmd.fullName.empty()) {
        return Result<InstructorDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Instructor name cannot be empty"));
    }

    try {
        domain::Instructor inst;
        inst.fullName = cmd.fullName;
        inst.phone = cmd.phone;
        inst.specialization = cmd.specialization;
        inst.compensationType = domain::compensationTypeFromString(cmd.compensationType);
        inst.baseRate = domain::Money(cmd.baseRatePiastres);
        inst.hourlyRate = inst.baseRate;
        inst.monthlySalary = domain::Money(cmd.monthlySalaryPiastres > 0 ? cmd.monthlySalaryPiastres : (inst.compensationType == domain::CompensationType::MonthlySalary ? cmd.baseRatePiastres : 0));
        inst.contractStartDate = cmd.contractStartDate;
        inst.contractEndDate = cmd.contractEndDate;
        inst.notes = cmd.notes;
        inst.status = domain::InstructorStatus::Active;

        auto saved = m_instructorRepo->save(inst);
        std::string actorId = m_authContext->currentUser().has_value() ? m_authContext->currentUser()->id() : "system";
        m_auditService->record(actorId, "INSTRUCTOR_CREATED", "Instructor", std::to_string(saved.id),
                               "Created instructor profile: " + saved.fullName);

        return Result<InstructorDto>::ok(toInstructorDto(saved));
    } catch (const std::exception& e) {
        return Result<InstructorDto>::error(
            domain::DomainError(domain::ErrorCode::DatabaseError, e.what()));
    }
}

Result<InstructorDto> InstructorPayrollUseCases::updateInstructor(const UpdateInstructorCommand& cmd) {
    auto instOpt = m_instructorRepo->findById(cmd.id);
    if (!instOpt.has_value()) {
        return Result<InstructorDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Instructor not found"));
    }

    if (cmd.fullName.empty()) {
        return Result<InstructorDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Instructor name cannot be empty"));
    }

    try {
        auto inst = *instOpt;
        inst.fullName = cmd.fullName;
        inst.phone = cmd.phone;
        inst.specialization = cmd.specialization;
        inst.compensationType = domain::compensationTypeFromString(cmd.compensationType);
        inst.baseRate = domain::Money(cmd.baseRatePiastres);
        inst.hourlyRate = inst.baseRate;
        inst.monthlySalary = domain::Money(cmd.monthlySalaryPiastres > 0 ? cmd.monthlySalaryPiastres : (inst.compensationType == domain::CompensationType::MonthlySalary ? cmd.baseRatePiastres : 0));
        inst.contractStartDate = cmd.contractStartDate;
        inst.contractEndDate = cmd.contractEndDate;
        inst.notes = cmd.notes;
        inst.status = (cmd.status == "inactive") ? domain::InstructorStatus::Inactive : domain::InstructorStatus::Active;

        auto saved = m_instructorRepo->save(inst);
        std::string actorId = m_authContext->currentUser().has_value() ? m_authContext->currentUser()->id() : "system";
        m_auditService->record(actorId, "INSTRUCTOR_UPDATED", "Instructor", std::to_string(saved.id),
                               "Updated instructor profile: " + saved.fullName);

        return Result<InstructorDto>::ok(toInstructorDto(saved));
    } catch (const std::exception& e) {
        return Result<InstructorDto>::error(
            domain::DomainError(domain::ErrorCode::DatabaseError, e.what()));
    }
}

Result<std::vector<InstructorDto>> InstructorPayrollUseCases::listInstructors(const InstructorFilter& filter) {
    auto list = m_instructorRepo->findAll(filter);
    std::vector<InstructorDto> dtos;
    dtos.reserve(list.size());
    for (const auto& inst : list) {
        dtos.push_back(toInstructorDto(inst));
    }
    return Result<std::vector<InstructorDto>>::ok(std::move(dtos));
}

Result<InstructorDto> InstructorPayrollUseCases::getInstructor(std::int64_t id) {
    auto instOpt = m_instructorRepo->findById(id);
    if (!instOpt.has_value()) {
        return Result<InstructorDto>::error(
            domain::DomainError(domain::ErrorCode::ValidationFailed, "Instructor not found"));
    }
    return Result<InstructorDto>::ok(toInstructorDto(*instOpt));
}

InstructorDto InstructorPayrollUseCases::toInstructorDto(const domain::Instructor& inst) {
    return InstructorDto{
        .id = inst.id,
        .userId = std::nullopt,
        .fullName = inst.fullName,
        .phone = inst.phone,
        .specialization = inst.specialization,
        .compensationType = domain::compensationTypeToString(inst.compensationType),
        .baseRatePiastres = inst.baseRate.piasters() > 0 ? inst.baseRate.piasters() : inst.hourlyRate.piasters(),
        .monthlySalaryPiastres = inst.monthlySalary.piasters() > 0 ? inst.monthlySalary.piasters() : inst.baseRate.piasters(),
        .contractStartDate = inst.contractStartDate,
        .contractEndDate = inst.contractEndDate,
        .notes = inst.notes,
        .status = (inst.status == domain::InstructorStatus::Active) ? "active" : "inactive"
    };
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
    dto.settlementStatus = s.settlementStatusToString();
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
