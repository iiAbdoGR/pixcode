#include "EnrollmentUseCases.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace pixcode::application {

static std::string currentUtcIso() {
    auto now = std::chrono::system_clock::now();
    auto itt = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&itt), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

EnrollmentUseCases::EnrollmentUseCases(std::shared_ptr<IEnrollmentRepository> enrollments,
                                       std::shared_ptr<IGroupRepository> groups,
                                       std::shared_ptr<IStudentRepository> students,
                                       std::shared_ptr<IPaymentRepository> payments,
                                       std::shared_ptr<ICourseRepository> courses)
    : m_enrollments(std::move(enrollments)),
      m_groups(std::move(groups)),
      m_students(std::move(students)),
      m_payments(std::move(payments)),
      m_courses(std::move(courses)) {}

Result<EnrollmentDto> EnrollmentUseCases::enrollStudent(const EnrollStudentCommand& cmd) {
    auto student = m_students->findById(cmd.studentId);
    if (!student) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Student not found."));
    }
    if (student->status != domain::StudentStatus::Active) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Student is archived and cannot be enrolled."));
    }

    auto group = m_groups->findById(cmd.groupId);
    if (!group) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Group not found."));
    }
    if (group->status != domain::GroupStatus::Open) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Group is not open for new enrollments."));
    }

    if (group->capacity > 0) {
        int enrolled = m_enrollments->countActiveInGroup(cmd.groupId);
        if (enrolled >= group->capacity) {
            return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Group has reached maximum capacity."));
        }
    }

    auto existing = m_enrollments->findActive(cmd.studentId, cmd.groupId);
    if (existing) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Student is already actively enrolled in this group."));
    }

    if (auto err = domain::Enrollment::validate(cmd.studentId, cmd.groupId, group->fee, domain::Money{cmd.discountPiastres}); err.has_value()) {
        return Result<EnrollmentDto>::error(*err);
    }

    std::int64_t netFee = group->fee.piasters() - cmd.discountPiastres;

    domain::Enrollment e;
    e.studentId = cmd.studentId;
    e.groupId = cmd.groupId;
    e.agreedFee = group->fee;
    e.discount = domain::Money{cmd.discountPiastres};
    e.netFee = domain::Money{netFee};
    e.status = domain::EnrollmentStatus::Active;
    e.notes = cmd.notes;

    auto saved = m_enrollments->save(e);
    return Result<EnrollmentDto>::ok(buildEnrollmentDto(saved));
}

Result<EnrollmentDto> EnrollmentUseCases::cancelEnrollment(const CancelSubscriptionCommand& cmd) {
    auto existing = m_enrollments->findById(cmd.enrollmentId);
    if (!existing) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Enrollment not found."));
    }
    if (existing->status == domain::EnrollmentStatus::Cancelled) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Enrollment is already cancelled."));
    }

    existing->status = domain::EnrollmentStatus::Cancelled;
    existing->cancellationReason = cmd.reason;
    existing->cancelledAtUtc = currentUtcIso();
    if (!cmd.cancelledByUserId.empty()) {
        existing->cancelledByUserId = cmd.cancelledByUserId;
    }

    auto saved = m_enrollments->save(*existing);
    return Result<EnrollmentDto>::ok(buildEnrollmentDto(saved));
}

Result<EnrollmentDto> EnrollmentUseCases::cancelEnrollment(std::int64_t enrollmentId, const std::string& reason) {
    return cancelEnrollment(CancelSubscriptionCommand{
        .enrollmentId = enrollmentId,
        .reason = reason,
        .cancelledByUserId = ""
    });
}

Result<EnrollmentDto> EnrollmentUseCases::restoreSubscription(const RestoreSubscriptionCommand& cmd) {
    auto existing = m_enrollments->findById(cmd.enrollmentId);
    if (!existing) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Enrollment not found."));
    }
    if (existing->status != domain::EnrollmentStatus::Cancelled) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Only cancelled enrollments can be restored."));
    }

    auto student = m_students->findById(existing->studentId);
    if (!student || student->status != domain::StudentStatus::Active) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Student is inactive or archived; cannot restore subscription."));
    }

    auto group = m_groups->findById(existing->groupId);
    if (!group) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Group not found."));
    }
    if (group->status != domain::GroupStatus::Open) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Group is not open for enrollments."));
    }
    if (group->capacity > 0) {
        int enrolled = m_enrollments->countActiveInGroup(existing->groupId);
        if (enrolled >= group->capacity) {
            return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Group has reached maximum capacity. Cannot restore subscription."));
        }
    }

    auto activeExisting = m_enrollments->findActive(existing->studentId, existing->groupId);
    if (activeExisting) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Student already has an active enrollment in this group."));
    }

    existing->status = domain::EnrollmentStatus::Active;
    existing->restorationReason = cmd.reason;
    existing->restoredAtUtc = currentUtcIso();
    if (!cmd.restoredByUserId.empty()) {
        existing->restoredByUserId = cmd.restoredByUserId;
    }

    auto saved = m_enrollments->save(*existing);
    return Result<EnrollmentDto>::ok(buildEnrollmentDto(saved));
}

Result<std::vector<EnrollmentDto>> EnrollmentUseCases::listAll() {
    auto list = m_enrollments->findAll();
    std::vector<EnrollmentDto> dtos;
    dtos.reserve(list.size());
    for (const auto& e : list) {
        dtos.push_back(buildEnrollmentDto(e));
    }
    return Result<std::vector<EnrollmentDto>>::ok(std::move(dtos));
}

Result<std::vector<EnrollmentDto>> EnrollmentUseCases::listByStudent(std::int64_t studentId) {
    auto list = m_enrollments->findByStudent(studentId);
    std::vector<EnrollmentDto> dtos;
    dtos.reserve(list.size());
    for (const auto& e : list) {
        dtos.push_back(buildEnrollmentDto(e));
    }
    return Result<std::vector<EnrollmentDto>>::ok(std::move(dtos));
}

Result<std::vector<EnrollmentDto>> EnrollmentUseCases::listByGroup(std::int64_t groupId) {
    auto list = m_enrollments->findByGroup(groupId);
    std::vector<EnrollmentDto> dtos;
    dtos.reserve(list.size());
    for (const auto& e : list) {
        dtos.push_back(buildEnrollmentDto(e));
    }
    return Result<std::vector<EnrollmentDto>>::ok(std::move(dtos));
}

Result<EnrollmentDto> EnrollmentUseCases::getEnrollment(std::int64_t enrollmentId) {
    auto e = m_enrollments->findById(enrollmentId);
    if (!e) {
        return Result<EnrollmentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Enrollment not found."));
    }
    return Result<EnrollmentDto>::ok(buildEnrollmentDto(*e));
}

Result<PaymentDto> EnrollmentUseCases::recordPayment(const RecordPaymentCommand& cmd) {
    auto e = m_enrollments->findById(cmd.enrollmentId);
    if (!e) {
        return Result<PaymentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Enrollment not found."));
    }
    if (e->status == domain::EnrollmentStatus::Cancelled) {
        return Result<PaymentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Cannot record payment on a cancelled enrollment."));
    }
    if (cmd.amountPiastres <= 0) {
        return Result<PaymentDto>::error(domain::DomainError(domain::ErrorCode::ValidationFailed, "Payment amount must be greater than zero."));
    }

    std::int64_t alreadyPaid = m_payments->totalPaid(cmd.enrollmentId);
    std::int64_t outstandingBalance = e->netFee.piasters() - alreadyPaid;
    if (cmd.amountPiastres > outstandingBalance) {
        return Result<PaymentDto>::error(domain::DomainError(domain::ErrorCode::BusinessRuleViolation, "Payment amount exceeds outstanding balance."));
    }

    std::string receiptNo = m_payments->nextReceiptNumber();

    domain::Payment p;
    p.enrollmentId = cmd.enrollmentId;
    p.amount = domain::Money{cmd.amountPiastres};
    if (cmd.paymentMethod == "bank_transfer") {
        p.method = domain::PaymentMethod::BankTransfer;
    } else if (cmd.paymentMethod == "other") {
        p.method = domain::PaymentMethod::Other;
    } else {
        p.method = domain::PaymentMethod::Cash;
    }
    p.receiptNumber = receiptNo;
    p.notes = cmd.notes;
    p.recordedBy = cmd.recordedByUserId;

    auto saved = m_payments->save(p);
    return Result<PaymentDto>::ok(toPaymentDto(saved));
}

Result<std::vector<PaymentDto>> EnrollmentUseCases::listPayments(std::int64_t enrollmentId) {
    auto list = m_payments->findByEnrollment(enrollmentId);
    std::vector<PaymentDto> dtos;
    dtos.reserve(list.size());
    for (const auto& p : list) {
        dtos.push_back(toPaymentDto(p));
    }
    return Result<std::vector<PaymentDto>>::ok(std::move(dtos));
}

EnrollmentDto EnrollmentUseCases::buildEnrollmentDto(const domain::Enrollment& e) {
    std::int64_t paid = m_payments->totalPaid(e.id);
    std::int64_t balance = e.netFee.piasters() - paid;

    EnrollmentDto dto;
    dto.id = e.id;
    dto.studentId = e.studentId;
    dto.groupId = e.groupId;
    dto.agreedFeePiastres = e.agreedFee.piasters();
    dto.discountPiastres = e.discount.piasters();
    dto.netFeePiastres = e.netFee.piasters();
    dto.paidPiastres = paid;
    dto.balancePiastres = balance;
    dto.enrolledAtUtc = e.enrolledAtUtc;
    dto.cancelledAtUtc = e.cancelledAtUtc;
    dto.cancellationReason = e.cancellationReason;
    dto.cancelledByUserId = e.cancelledByUserId;
    dto.restoredAtUtc = e.restoredAtUtc;
    dto.restorationReason = e.restorationReason;
    dto.restoredByUserId = e.restoredByUserId;
    dto.notes = e.notes;

    switch (e.status) {
        case domain::EnrollmentStatus::Cancelled: dto.status = "cancelled"; break;
        case domain::EnrollmentStatus::Transferred: dto.status = "transferred"; break;
        case domain::EnrollmentStatus::Completed: dto.status = "completed"; break;
        default: dto.status = "active"; break;
    }

    auto student = m_students->findById(e.studentId);
    if (student) {
        dto.studentCode = student->studentCode;
        dto.studentName = student->fullName;
    }

    auto group = m_groups->findById(e.groupId);
    if (group) {
        dto.groupName = group->name;
        auto course = m_courses->findById(group->courseId);
        if (course) {
            dto.courseName = course->name;
        }
    }

    return dto;
}

PaymentDto EnrollmentUseCases::toPaymentDto(const domain::Payment& p) {
    std::string methodStr = "cash";
    if (p.method == domain::PaymentMethod::BankTransfer) methodStr = "bank_transfer";
    else if (p.method == domain::PaymentMethod::Other) methodStr = "other";

    return PaymentDto{
        .id = p.id,
        .enrollmentId = p.enrollmentId,
        .amountPiastres = p.amount.piasters(),
        .paymentMethod = methodStr,
        .paidAtUtc = p.paidAtUtc,
        .receiptNumber = p.receiptNumber,
        .notes = p.notes,
        .createdAtUtc = p.createdAtUtc
    };
}

} // namespace pixcode::application
