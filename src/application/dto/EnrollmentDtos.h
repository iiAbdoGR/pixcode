#pragma once

#include <string>
#include <cstdint>
#include <optional>

namespace pixcode::application {

struct EnrollStudentCommand {
    std::int64_t studentId{0};
    std::int64_t groupId{0};
    std::int64_t discountPiastres{0};
    std::string notes;
};

struct EnrollmentDto {
    std::int64_t id{0};
    std::int64_t studentId{0};
    std::string studentCode;
    std::string studentName;
    std::int64_t groupId{0};
    std::string groupName;
    std::string courseName;
    std::int64_t agreedFeePiastres{0};
    std::int64_t discountPiastres{0};
    std::int64_t netFeePiastres{0};
    std::int64_t paidPiastres{0};
    std::int64_t balancePiastres{0};
    std::string status;
    std::string enrolledAtUtc;
};

struct RecordPaymentCommand {
    std::int64_t enrollmentId{0};
    std::int64_t amountPiastres{0};
    std::string paymentMethod; // "cash", "bank_transfer", "other"
    std::string notes;
    std::optional<std::string> recordedByUserId;
};

struct PaymentDto {
    std::int64_t id{0};
    std::int64_t enrollmentId{0};
    std::int64_t amountPiastres{0};
    std::string paymentMethod;
    std::string paidAtUtc;
    std::string receiptNumber;
    std::string notes;
    std::string createdAtUtc;
};

struct InstructorDto {
    std::int64_t id{0};
    std::optional<std::string> userId;
    std::string fullName;
    std::string phone;
    std::string notes;
    std::string status;
};

struct CreateInstructorCommand {
    std::string fullName;
    std::string phone;
    std::string notes;
    std::optional<std::string> userId;
};

} // namespace pixcode::application

