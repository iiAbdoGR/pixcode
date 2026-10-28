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
    auto userOpt = m_userRepo->findById(resultDto.userId);
    if (!userOpt.has_value()) return;

    m_authContext->setCurrentUser(std::move(*userOpt));
    m_isLocked = false;
    m_secondsRemaining = DefaultIdleTimeoutSeconds;
    m_idleTimer->start(1000);

    QStringList roleCodes;
    for (const auto& r : resultDto.roles) {
        roleCodes.append(QString::fromStdString(r));
    }
    emit userLoggedIn(QString::fromStdString(resultDto.username), roleCodes);
}

void ApplicationContext::logout() {
    m_idleTimer->stop();
    m_authContext->setCurrentUser(std::nullopt);
    m_isLocked = false;
    emit userLoggedOut();
}

void ApplicationContext::lockSession() {
    if (m_isLocked) return;
    m_isLocked = true;
    m_idleTimer->stop();
    emit sessionLocked();
}

void ApplicationContext::notifyUserActivity() {
    if (!m_isLocked && m_authContext->isAuthenticated()) {
        m_secondsRemaining = DefaultIdleTimeoutSeconds;
    }
}

bool ApplicationContext::eventFilter(QObject* watched, QEvent* event) {
    switch (event->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::Wheel:
        case QEvent::TouchBegin:
            notifyUserActivity();
            break;
        default:
            break;
    }
    return QObject::eventFilter(watched, event);
}

} // namespace pixcode::desktop
