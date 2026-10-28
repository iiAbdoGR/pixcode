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
    // 1. Run migrations
    if (!m_migrationRunner->runMigrations(outError)) {
        return false;
    }

    // 2. Check DB integrity
    QString integrityMsg;
    if (!m_dbManager->checkIntegrity(integrityMsg)) {
        outError = "Database integrity check failed: " + integrityMsg;
        return false;
    }

    return true;
}

bool ApplicationContext::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::KeyPress ||
        event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseMove) {
        notifyUserActivity();
    }
    return QObject::eventFilter(watched, event);
}

void ApplicationContext::notifyUserActivity() {
    if (!m_isLocked) {
        m_secondsRemaining = DefaultIdleTimeoutSeconds;
    }
}

void ApplicationContext::login(const application::LoginResultDto& resultDto) {
    auto userOpt = m_userRepo->findById(resultDto.userId);
    if (userOpt.has_value()) {
        m_authContext->setCurrentUser(*userOpt);
    }

    LocalizationManager::instance().setLanguage(QString::fromStdString(resultDto.preferredLanguage));

    m_isLocked = false;
    m_secondsRemaining = DefaultIdleTimeoutSeconds;
    m_idleTimer->start(1000); // 1-second interval

    QStringList roleList;
    for (const auto& r : resultDto.roles) {
        roleList.append(QString::fromStdString(r));
    }

    emit userLoggedIn(QString::fromStdString(resultDto.username), roleList);
}

void ApplicationContext::logout() {
    m_idleTimer->stop();
    m_authContext->setCurrentUser(std::nullopt);
    m_isLocked = false;
    m_secondsRemaining = DefaultIdleTimeoutSeconds;

    emit userLoggedOut();
}

void ApplicationContext::lockSession() {
    if (!m_authContext->isAuthenticated()) return;
    m_isLocked = true;
    emit sessionLocked();
}

} // namespace pixcode::desktop

