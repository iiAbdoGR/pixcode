#pragma once

#include <vector>
#include <string>
#include <memory>
#include "application/results/Result.h"
#include "application/dto/EnrollmentDtos.h"
#include "application/ports/IEnrollmentRepository.h"
#include "application/ports/IGroupRepository.h"
#include "application/ports/IStudentRepository.h"
#include "application/ports/IPaymentRepository.h"
#include "application/ports/ICourseRepository.h"
#include "domain/entities/Enrollment.h"
#include "domain/entities/Payment.h"

namespace pixcode::application {

class EnrollmentUseCases {
public:
    EnrollmentUseCases(std::shared_ptr<IEnrollmentRepository> enrollments,
                       std::shared_ptr<IGroupRepository> groups,
                       std::shared_ptr<IStudentRepository> students,
                       std::shared_ptr<IPaymentRepository> payments,
                       std::shared_ptr<ICourseRepository> courses);

    Result<EnrollmentDto> enrollStudent(const EnrollStudentCommand& cmd);
    Result<EnrollmentDto> cancelEnrollment(std::int64_t enrollmentId, const std::string& reason);
    Result<std::vector<EnrollmentDto>> listByStudent(std::int64_t studentId);
    Result<std::vector<EnrollmentDto>> listByGroup(std::int64_t groupId);
    Result<EnrollmentDto> getEnrollment(std::int64_t enrollmentId);

    Result<PaymentDto> recordPayment(const RecordPaymentCommand& cmd);
    Result<std::vector<PaymentDto>> listPayments(std::int64_t enrollmentId);

private:
    std::shared_ptr<IEnrollmentRepository> m_enrollments;
    std::shared_ptr<IGroupRepository> m_groups;
    std::shared_ptr<IStudentRepository> m_students;
    std::shared_ptr<IPaymentRepository> m_payments;
    std::shared_ptr<ICourseRepository> m_courses;

    EnrollmentDto buildEnrollmentDto(const domain::Enrollment& e);
    static PaymentDto toPaymentDto(const domain::Payment& p);
};

} // namespace pixcode::application

