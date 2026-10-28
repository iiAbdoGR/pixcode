#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <memory>

#include "infrastructure/database/DatabaseManager.h"
#include "infrastructure/migrations/MigrationRunner.h"
#include "infrastructure/security/Argon2idPasswordHasher.h"
#include "infrastructure/system/SystemClock.h"
#include "infrastructure/repositories/SqliteUserRepository.h"
#include "infrastructure/repositories/SqliteRoleRepository.h"
#include "infrastructure/repositories/SqliteAuditLogRepository.h"
#include "infrastructure/repositories/SqliteStudentRepository.h"
#include "infrastructure/repositories/SqliteCourseRepository.h"
#include "infrastructure/repositories/SqliteInstructorRepository.h"
#include "infrastructure/repositories/SqliteGroupRepository.h"
#include "infrastructure/repositories/SqliteEnrollmentRepository.h"
#include "infrastructure/repositories/SqlitePaymentRepository.h"

#include "application/authorization/AuthorizationContext.h"
#include "application/use_cases/AuthenticateUserUseCase.h"
#include "application/use_cases/FirstRunSetupUseCase.h"
#include "application/use_cases/ChangePasswordUseCase.h"
#include "application/use_cases/AuditService.h"
#include "application/use_cases/StudentUseCases.h"
#include "application/use_cases/CourseUseCases.h"
#include "application/use_cases/GroupUseCases.h"
#include "application/use_cases/EnrollmentUseCases.h"

namespace pixcode::desktop {

class ApplicationContext : public QObject {
    Q_OBJECT

public:
    explicit ApplicationContext(const QString& dbPath, QObject* parent = nullptr);
    ~ApplicationContext() override;

    [[nodiscard]] bool initialize(QString& outError);

    // Dependencies
    [[nodiscard]] std::shared_ptr<infrastructure::DatabaseManager> databaseManager() const noexcept { return m_dbManager; }
    [[nodiscard]] std::shared_ptr<application::IUserRepository> userRepository() const noexcept { return m_userRepo; }
    [[nodiscard]] std::shared_ptr<application::IRoleRepository> roleRepository() const noexcept { return m_roleRepo; }
    [[nodiscard]] std::shared_ptr<application::IAuditLogRepository> auditRepository() const noexcept { return m_auditRepo; }
    [[nodiscard]] std::shared_ptr<application::IAuthorizationContext> authContext() const noexcept { return m_authContext; }

    [[nodiscard]] std::shared_ptr<application::IStudentRepository> studentRepository() const noexcept { return m_studentRepo; }
    [[nodiscard]] std::shared_ptr<application::ICourseRepository> courseRepository() const noexcept { return m_courseRepo; }
    [[nodiscard]] std::shared_ptr<application::IInstructorRepository> instructorRepository() const noexcept { return m_instructorRepo; }
    [[nodiscard]] std::shared_ptr<application::IGroupRepository> groupRepository() const noexcept { return m_groupRepo; }
    [[nodiscard]] std::shared_ptr<application::IEnrollmentRepository> enrollmentRepository() const noexcept { return m_enrollmentRepo; }
    [[nodiscard]] std::shared_ptr<application::IPaymentRepository> paymentRepository() const noexcept { return m_paymentRepo; }

    // Use cases
    [[nodiscard]] std::shared_ptr<application::AuthenticateUserUseCase> authenticateUseCase() const noexcept { return m_authUseCase; }
    [[nodiscard]] std::shared_ptr<application::FirstRunSetupUseCase> firstRunUseCase() const noexcept { return m_firstRunUseCase; }
    [[nodiscard]] std::shared_ptr<application::ChangePasswordUseCase> changePasswordUseCase() const noexcept { return m_changePasswordUseCase; }
    [[nodiscard]] std::shared_ptr<application::AuditService> auditService() const noexcept { return m_auditService; }

    [[nodiscard]] std::shared_ptr<application::StudentUseCases> studentUseCases() const noexcept { return m_studentUseCases; }
    [[nodiscard]] std::shared_ptr<application::CourseUseCases> courseUseCases() const noexcept { return m_courseUseCases; }
    [[nodiscard]] std::shared_ptr<application::GroupUseCases> groupUseCases() const noexcept { return m_groupUseCases; }
    [[nodiscard]] std::shared_ptr<application::EnrollmentUseCases> enrollmentUseCases() const noexcept { return m_enrollmentUseCases; }

    // Session Management
    void login(const application::LoginResultDto& resultDto);
    void logout();
    void lockSession();
    void notifyUserActivity();

    [[nodiscard]] bool isSessionLocked() const noexcept { return m_isLocked; }
    [[nodiscard]] int secondsUntilAutoLock() const noexcept { return m_secondsRemaining; }

signals:
    void userLoggedIn(const QString& username, const QStringList& roles);
    void userLoggedOut();
    void sessionLocked();
    void autoLockCountdownTick(int secondsRemaining);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    static constexpr int DefaultIdleTimeoutSeconds = 300; // 5 minutes

    std::shared_ptr<infrastructure::DatabaseManager> m_dbManager;
    std::shared_ptr<infrastructure::MigrationRunner> m_migrationRunner;
    std::shared_ptr<infrastructure::Argon2idPasswordHasher> m_passwordHasher;
    std::shared_ptr<infrastructure::SystemClock> m_clock;
    std::shared_ptr<infrastructure::SqliteUserRepository> m_userRepo;
    std::shared_ptr<infrastructure::SqliteRoleRepository> m_roleRepo;
    std::shared_ptr<infrastructure::SqliteAuditLogRepository> m_auditRepo;

    std::shared_ptr<infrastructure::SqliteStudentRepository> m_studentRepo;
    std::shared_ptr<infrastructure::SqliteCourseRepository> m_courseRepo;
    std::shared_ptr<infrastructure::SqliteInstructorRepository> m_instructorRepo;
    std::shared_ptr<infrastructure::SqliteGroupRepository> m_groupRepo;
    std::shared_ptr<infrastructure::SqliteEnrollmentRepository> m_enrollmentRepo;
    std::shared_ptr<infrastructure::SqlitePaymentRepository> m_paymentRepo;

    std::shared_ptr<application::AuditService> m_auditService;
    std::shared_ptr<application::AuthorizationContext> m_authContext;

    std::shared_ptr<application::AuthenticateUserUseCase> m_authUseCase;
    std::shared_ptr<application::FirstRunSetupUseCase> m_firstRunUseCase;
    std::shared_ptr<application::ChangePasswordUseCase> m_changePasswordUseCase;

    std::shared_ptr<application::StudentUseCases> m_studentUseCases;
    std::shared_ptr<application::CourseUseCases> m_courseUseCases;
    std::shared_ptr<application::GroupUseCases> m_groupUseCases;
    std::shared_ptr<application::EnrollmentUseCases> m_enrollmentUseCases;

    QTimer* m_idleTimer;
    int m_secondsRemaining;
    bool m_isLocked;
};

} // namespace pixcode::desktop
