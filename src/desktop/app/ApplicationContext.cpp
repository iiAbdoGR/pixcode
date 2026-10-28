#include "ApplicationContext.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtWidgets/QApplication>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

ApplicationContext::ApplicationContext(const QString& dbPath, QObject* parent)
    : QObject(parent),
      m_idleTimer(new QTimer(this)),
      m_secondsRemaining(DefaultIdleTimeoutSeconds),
      m_isLocked(false) {

    // 1. Core infrastructure instances
    m_dbManager = std::make_shared<infrastructure::DatabaseManager>(dbPath);
    m_migrationRunner = std::make_shared<infrastructure::MigrationRunner>(m_dbManager);
    m_passwordHasher = std::make_shared<infrastructure::Argon2idPasswordHasher>();
    m_clock = std::make_shared<infrastructure::SystemClock>();

    // 2. Repositories
    m_userRepo = std::make_shared<infrastructure::SqliteUserRepository>(m_dbManager);
    m_roleRepo = std::make_shared<infrastructure::SqliteRoleRepository>(m_dbManager);
    m_auditRepo = std::make_shared<infrastructure::SqliteAuditLogRepository>(m_dbManager);

    m_studentRepo = std::make_shared<infrastructure::SqliteStudentRepository>(m_dbManager);
    m_courseRepo = std::make_shared<infrastructure::SqliteCourseRepository>(m_dbManager);
    m_instructorRepo = std::make_shared<infrastructure::SqliteInstructorRepository>(m_dbManager);
    m_groupRepo = std::make_shared<infrastructure::SqliteGroupRepository>(m_dbManager);
    m_enrollmentRepo = std::make_shared<infrastructure::SqliteEnrollmentRepository>(m_dbManager);
    m_paymentRepo = std::make_shared<infrastructure::SqlitePaymentRepository>(m_dbManager);

    m_sessionRepo = std::make_shared<infrastructure::SqliteSessionRepository>(m_dbManager);
    m_attendanceRepo = std::make_shared<infrastructure::SqliteAttendanceRepository>(m_dbManager);
    m_payoutRepo = std::make_shared<infrastructure::SqliteInstructorPayoutRepository>(m_dbManager);

    // 3. Application Services
    m_auditService = std::make_shared<application::AuditService>(m_auditRepo, m_clock);
    m_authContext = std::make_shared<application::AuthorizationContext>();

    // 4. Use Cases
    m_authUseCase = std::make_shared<application::AuthenticateUserUseCase>(
        m_userRepo, m_roleRepo, m_passwordHasher, m_clock, m_auditService);

    m_firstRunUseCase = std::make_shared<application::FirstRunSetupUseCase>(
        m_userRepo, m_roleRepo, m_passwordHasher, m_clock, m_auditService);

    m_changePasswordUseCase = std::make_shared<application::ChangePasswordUseCase>(
        m_userRepo, m_passwordHasher, m_clock, m_auditService);

    m_studentUseCases = std::make_shared<application::StudentUseCases>(
        m_studentRepo, m_authContext);

    m_courseUseCases = std::make_shared<application::CourseUseCases>(
        m_courseRepo);

    m_groupUseCases = std::make_shared<application::GroupUseCases>(
        m_groupRepo, m_courseRepo, m_instructorRepo);

    m_enrollmentUseCases = std::make_shared<application::EnrollmentUseCases>(
        m_enrollmentRepo, m_groupRepo, m_studentRepo, m_paymentRepo, m_courseRepo);

    m_staffUseCases = std::make_shared<application::StaffManagementUseCases>(
        m_userRepo, m_roleRepo, m_instructorRepo, m_passwordHasher, m_clock, m_authContext, m_auditService);

    m_attendanceUseCases = std::make_shared<application::AttendanceUseCases>(
        m_attendanceRepo, m_sessionRepo, m_studentRepo, m_groupRepo, m_courseRepo, m_clock, m_authContext, m_auditService);

    m_payrollUseCases = std::make_shared<application::InstructorPayrollUseCases>(
        m_sessionRepo, m_payoutRepo, m_instructorRepo, m_groupRepo, m_courseRepo, m_clock, m_authContext, m_auditService);

    // 5. Shared-PC Idle auto-lock countdown timer
    connect(m_idleTimer, &QTimer::timeout, this, [this]() {
        if (!m_authContext->isAuthenticated() || m_isLocked) return;

        m_secondsRemaining--;
        emit autoLockCountdownTick(m_secondsRemaining);

        if (m_secondsRemaining <= 0) {
            lockSession();
        }
    });

    if (qApp) {
        qApp->installEventFilter(this);
    }
}

ApplicationContext::~ApplicationContext() {
    if (m_idleTimer) {
        m_idleTimer->stop();
    }
}

bool ApplicationContext::initialize(QString& outError) {
    if (!m_migrationRunner->runMigrations(outError)) {
        return false;
    }
    return true;
}

void ApplicationContext::login(const application::LoginResultDto& resultDto) {
    m_isLocked = false;
    m_secondsRemaining = DefaultIdleTimeoutSeconds;
    m_idleTimer->start(1000);

    QStringList roles;
    for (const auto& r : resultDto.roles) {
        roles.append(QString::fromStdString(r));
    }

    emit userLoggedIn(QString::fromStdString(resultDto.username), roles);
}

void ApplicationContext::logout() {
    m_idleTimer->stop();
    m_authContext->setCurrentUser(std::nullopt);
    m_isLocked = false;
    emit userLoggedOut();
}

void ApplicationContext::lockSession() {
    m_idleTimer->stop();
    m_isLocked = true;
    emit sessionLocked();
}

void ApplicationContext::notifyUserActivity() {
    if (m_authContext->isAuthenticated() && !m_isLocked) {
        m_secondsRemaining = DefaultIdleTimeoutSeconds;
    }
}

bool ApplicationContext::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress ||
        event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseMove ||
        event->type() == QEvent::Wheel) {
        notifyUserActivity();
    }
    return QObject::eventFilter(watched, event);
}

} // namespace pixcode::desktop
