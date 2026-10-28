#include "StaffManagementUseCases.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace pixcode::application {

StaffManagementUseCases::StaffManagementUseCases(
    std::shared_ptr<IUserRepository> userRepo,
    std::shared_ptr<IRoleRepository> roleRepo,
    std::shared_ptr<IInstructorRepository> instructorRepo,
    std::shared_ptr<IPasswordHasher> hasher,
    std::shared_ptr<IClock> clock,
    std::shared_ptr<IAuthorizationContext> authContext,
    std::shared_ptr<AuditService> auditService)
    : m_userRepo(std::move(userRepo)),
      m_roleRepo(std::move(roleRepo)),
      m_instructorRepo(std::move(instructorRepo)),
      m_hasher(std::move(hasher)),
      m_clock(std::move(clock)),
      m_authContext(std::move(authContext)),
      m_auditService(std::move(auditService)) {}

Result<StaffDto> StaffManagementUseCases::createStaff(const CreateStaffCommand& cmd) {
    m_authContext->enforcePermission("system.users.manage");

    if (cmd.username.length() < 3) {
        return Result<StaffDto>::error(
            domain::DomainError(domain::ErrorCode::InvalidUsername, "Username must be at least 3 characters"));
    }

    if (auto existing = m_userRepo->findByUsername(cmd.username); existing.has_value()) {
        return Result<StaffDto>::error(
            domain::DomainError(domain::ErrorCode::UserAlreadyExists, "Username is already in use"));
    }

    std::string failureReason;
    if (!m_hasher->isPasswordStrong(cmd.password, failureReason)) {
        return Result<StaffDto>::error(
            domain::DomainError(domain::ErrorCode::WeakPassword, failureReason));
    }

    const auto now = m_clock->nowUtcIso();
    const auto hash = m_hasher->hashPassword(cmd.password);

    // Generate unique ID
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string newId = "usr_" + std::to_string(ms);

    domain::User user(
        newId,
        cmd.username,
        hash,
        true, // isActive
        true, // mustChangePassword on initial staff creation
        0,
        std::nullopt,
        cmd.preferredLanguage.empty() ? "ar" : cmd.preferredLanguage,
        now,
        now,
        std::nullopt
    );

    m_userRepo->save(user);

    // Assign roles
    bool isInstructor = false;
    for (const auto& roleCode : cmd.roleCodes) {
        if (roleCode == "instructor") isInstructor = true;
        auto roleOpt = m_roleRepo->findByCode(roleCode);
        if (roleOpt.has_value()) {
            m_userRepo->assignRole(newId, roleOpt->id());
            user.addRole(*roleOpt);
        }
    }

    // Link or create instructor profile
    if (isInstructor) {
        domain::Instructor inst;
        inst.userId = newId;
        inst.fullName = cmd.fullName.empty() ? cmd.username : cmd.fullName;
        inst.phone = cmd.phone;
        inst.specialization = cmd.specialization;
        inst.hourlyRate = domain::Money(cmd.hourlyRatePiastres);
        m_instructorRepo->save(inst);
    } else if (cmd.linkedInstructorId.has_value() && *cmd.linkedInstructorId > 0) {
        auto instOpt = m_instructorRepo->findById(*cmd.linkedInstructorId);
        if (instOpt.has_value()) {
            auto inst = *instOpt;
            inst.userId = newId;
            m_instructorRepo->save(inst);
        }
    }

    std::string actorId = m_authContext->currentUser().has_value() ? m_authContext->currentUser()->id() : "system";
    m_auditService->record(actorId, "USER_CREATED", "User", newId, "Created employee account: " + cmd.username);

    return Result<StaffDto>::ok(buildStaffDto(user));
}

Result<StaffDto> StaffManagementUseCases::updateStaff(const UpdateStaffCommand& cmd) {
    m_authContext->enforcePermission("system.users.manage");

    auto userOpt = m_userRepo->findById(cmd.userId);
    if (!userOpt.has_value()) {
        return Result<StaffDto>::error(
            domain::DomainError(domain::ErrorCode::UserNotFound, "User not found"));
    }

    auto user = *userOpt;
    const auto now = m_clock->nowUtcIso();
    const bool isLastSuper = m_userRepo->countActiveSuperAdmins() <= 1;

    user.setActive(cmd.isActive, now, isLastSuper);
    user.setPreferredLanguage(cmd.preferredLanguage, now);

    // Update roles
    user.clearRoles();
    bool isInstructor = false;
    for (const auto& roleCode : cmd.roleCodes) {
        if (roleCode == "instructor") isInstructor = true;
        auto roleOpt = m_roleRepo->findByCode(roleCode);
        if (roleOpt.has_value()) {
            user.addRole(*roleOpt);
        }
    }

    m_userRepo->save(user);

    // Sync instructor profile if instructor role is present
    auto instOpt = m_instructorRepo->findByUserId(cmd.userId);
    if (isInstructor) {
        domain::Instructor inst;
        if (instOpt.has_value()) {
            inst = *instOpt;
        } else {
            inst.userId = cmd.userId;
        }
        if (!cmd.fullName.empty()) inst.fullName = cmd.fullName;
        if (!cmd.phone.empty()) inst.phone = cmd.phone;
        inst.specialization = cmd.specialization;
        inst.hourlyRate = domain::Money(cmd.hourlyRatePiastres);
        m_instructorRepo->save(inst);
    }

    std::string actorId = m_authContext->currentUser().has_value() ? m_authContext->currentUser()->id() : "system";
    m_auditService->record(actorId, "USER_UPDATED", "User", user.id(), "Updated staff account: " + user.username());

    return Result<StaffDto>::ok(buildStaffDto(user));
}

Result<void> StaffManagementUseCases::resetPassword(const ResetStaffPasswordCommand& cmd) {
    m_authContext->enforcePermission("system.users.manage");

    auto userOpt = m_userRepo->findById(cmd.userId);
    if (!userOpt.has_value()) {
        return Result<void>::error(
            domain::DomainError(domain::ErrorCode::UserNotFound, "User not found"));
    }

    std::string failureReason;
    if (!m_hasher->isPasswordStrong(cmd.newPassword, failureReason)) {
        return Result<void>::error(
            domain::DomainError(domain::ErrorCode::WeakPassword, failureReason));
    }

    auto user = *userOpt;
    const auto now = m_clock->nowUtcIso();
    const auto hash = m_hasher->hashPassword(cmd.newPassword);

    user.resetPasswordByAdmin(hash, now);
    m_userRepo->save(user);

    std::string actorId = m_authContext->currentUser().has_value() ? m_authContext->currentUser()->id() : "system";
    m_auditService->record(actorId, "PASSWORD_RESET_BY_ADMIN", "User", user.id(),
                           "Admin reset password for user: " + user.username());

    return Result<void>::ok();
}

Result<std::vector<StaffDto>> StaffManagementUseCases::listStaff() {
    m_authContext->enforcePermission("system.users.manage");

    auto users = m_userRepo->findAll();
    std::vector<StaffDto> dtos;
    dtos.reserve(users.size());
    for (const auto& u : users) {
        dtos.push_back(buildStaffDto(u));
    }
    return Result<std::vector<StaffDto>>::ok(std::move(dtos));
}

Result<StaffDto> StaffManagementUseCases::getStaff(const std::string& userId) {
    m_authContext->enforcePermission("system.users.manage");

    auto userOpt = m_userRepo->findById(userId);
    if (!userOpt.has_value()) {
        return Result<StaffDto>::error(
            domain::DomainError(domain::ErrorCode::UserNotFound, "User not found"));
    }
    return Result<StaffDto>::ok(buildStaffDto(*userOpt));
}

StaffDto StaffManagementUseCases::buildStaffDto(const domain::User& user) {
    StaffDto dto;
    dto.id = user.id();
    dto.username = user.username();
    dto.isActive = user.isActive();
    dto.mustChangePassword = user.mustChangePassword();
    dto.failedLoginCount = user.failedLoginCount();
    dto.lockoutUntilUtc = user.lockoutUntilUtc();
    dto.preferredLanguage = user.preferredLanguage();
    dto.createdAtUtc = user.createdAtUtc();
    dto.lastLoginAtUtc = user.lastLoginAtUtc();

    for (const auto& r : user.roles()) {
        dto.roleCodes.push_back(r.code());
    }

    auto instOpt = m_instructorRepo->findByUserId(user.id());
    if (instOpt.has_value()) {
        dto.linkedInstructorId = instOpt->id;
        dto.linkedInstructorName = instOpt->fullName;
    }

    return dto;
}

} // namespace pixcode::application
