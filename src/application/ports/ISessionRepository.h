#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>
#include "domain/entities/Session.h"

namespace pixcode::application {

struct SessionFilter {
    std::optional<std::int64_t> groupId;
    std::optional<std::int64_t> instructorId;
    std::string fromDate; // YYYY-MM-DD
    std::string toDate;   // YYYY-MM-DD
    std::optional<domain::SessionStatus> status;
    std::optional<domain::SessionApprovalStatus> approvalStatus;
    int page{0};
    int pageSize{100};
};

class ISessionRepository {
public:
    virtual ~ISessionRepository() = default;

    [[nodiscard]] virtual std::optional<domain::Session> findById(std::int64_t id) = 0;
    [[nodiscard]] virtual std::vector<domain::Session> findAll(const SessionFilter& filter) = 0;
    [[nodiscard]] virtual int count(const SessionFilter& filter) = 0;
    virtual domain::Session save(const domain::Session& session) = 0;
    virtual bool remove(std::int64_t id) = 0;

    // Aggregate calculations for instructor payroll
    [[nodiscard]] virtual std::int64_t sumApprovedEarnings(std::int64_t instructorId, const std::string& fromDate = "", const std::string& toDate = "") = 0;
    [[nodiscard]] virtual int sumApprovedMinutes(std::int64_t instructorId, const std::string& fromDate = "", const std::string& toDate = "") = 0;
    [[nodiscard]] virtual int countLessonsTaught(std::int64_t instructorId, const std::string& fromDate = "", const std::string& toDate = "") = 0;
};

} // namespace pixcode::application
