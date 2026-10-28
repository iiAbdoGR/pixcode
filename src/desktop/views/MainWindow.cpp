#include "MainWindow.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"
#include "../dialogs/LoginDialog.h"
#include "../dialogs/ChangePasswordDialog.h"

namespace pixcode::desktop {

MainWindow::MainWindow(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QMainWindow(parent), m_appContext(std::move(appContext)) {
    setupUi();
    updateTranslations();

    connect(m_appContext.get(), &ApplicationContext::userLoggedIn, this, &MainWindow::onUserLoggedIn);
    connect(m_appContext.get(), &ApplicationContext::userLoggedOut, this, &MainWindow::onUserLoggedOut);
    connect(m_appContext.get(), &ApplicationContext::sessionLocked, this, &MainWindow::onSessionLocked);
    connect(m_appContext.get(), &ApplicationContext::autoLockCountdownTick, this, &MainWindow::onAutoLockTick);

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &MainWindow::updateTranslations);
}

void MainWindow::setupUi() {
    setWindowTitle("Pix Code Academy Management System");
    resize(1200, 760);
    setMinimumSize(960, 640);

    auto* centralWidget = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Top Header Bar
    auto* headerFrame = new QFrame(this);
    headerFrame->setStyleSheet("background-color: #FFFFFF; border-bottom: 1px solid #CBD2D9;");
    headerFrame->setFixedHeight(64);
    auto* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(24, 0, 24, 0);
    headerLayout->setSpacing(16);

    m_brandLabel = new QLabel("PIX CODE ACADEMY", headerFrame);
    m_brandLabel->setStyleSheet("font-size: 15px; font-weight: 800; color: #2E3A44; letter-spacing: 1px;");

    m_autoLockLabel = new QLabel(headerFrame);
    m_autoLockLabel->setStyleSheet("color: #7E939C; font-size: 12px; font-weight: 600;");

    m_userBadge = new QLabel(headerFrame);
    m_userBadge->setStyleSheet("background-color: #DDE4E1; color: #2E3A44; font-weight: 700; "
                               "border-radius: 4px; padding: 4px 12px; font-size: 12px;");

    m_changePasswordBtn = new QPushButton(headerFrame);
    m_changePasswordBtn->setObjectName("secondaryButton");
    m_changePasswordBtn->setFixedHeight(32);
    connect(m_changePasswordBtn, &QPushButton::clicked, this, &MainWindow::onChangePasswordClicked);

    m_langToggleBtn = new QPushButton(headerFrame);
    m_langToggleBtn->setObjectName("secondaryButton");
    m_langToggleBtn->setFixedHeight(32);
    connect(m_langToggleBtn, &QPushButton::clicked, this, &MainWindow::onLanguageToggleClicked);

    m_logoutBtn = new QPushButton(headerFrame);
    m_logoutBtn->setObjectName("secondaryButton");
    m_logoutBtn->setFixedHeight(32);
    connect(m_logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);

    headerLayout->addWidget(m_brandLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_autoLockLabel);
    headerLayout->addWidget(m_userBadge);
    headerLayout->addWidget(m_changePasswordBtn);
    headerLayout->addWidget(m_langToggleBtn);
    headerLayout->addWidget(m_logoutBtn);

    // Main Body (Sidebar + Stacked Views)
    auto* bodyWidget = new QWidget(this);
    auto* bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    m_sidebar = new NavigationSidebar(m_appContext, bodyWidget);
    connect(m_sidebar, &NavigationSidebar::destinationChanged, this, &MainWindow::onDestinationChanged);

    m_stackedWidget = new QStackedWidget(bodyWidget);

    m_dashboardView = new DashboardView(m_appContext, this);
    m_usersView = new UsersView(m_appContext, this);
    m_auditView = new AuditView(m_appContext, this);

    m_placeholderView = new QWidget(this);
    auto* plLayout = new QVBoxLayout(m_placeholderView);
    auto* plLabel = new QLabel("Module Scheduled for Subsequent Phase", m_placeholderView);
    plLabel->setAlignment(Qt::AlignCenter);
    plLabel->setStyleSheet("font-size: 18px; color: #7E939C; font-weight: 600;");
    plLayout->addWidget(plLabel);

    // Map destinations to stacked widget
    m_stackedWidget->addWidget(m_dashboardView);   // 0: Dashboard
    m_stackedWidget->addWidget(m_placeholderView); // 1: Placeholder for Phase 2-6
    m_stackedWidget->addWidget(m_usersView);       // 2: Users
    m_stackedWidget->addWidget(m_auditView);       // 3: Audit

    bodyLayout->addWidget(m_sidebar);
    bodyLayout->addWidget(m_stackedWidget);

    rootLayout->addWidget(headerFrame);
    rootLayout->addWidget(bodyWidget);

    setCentralWidget(centralWidget);
}

void MainWindow::updateTranslations() {
    setWindowTitle(trText("app.title"));
    m_logoutBtn->setText(trText("header.logout"));
    m_langToggleBtn->setText(trText("header.switch_lang"));
    m_changePasswordBtn->setText(LocalizationManager::instance().isRtl() ? "كلمة المرور" : "Password");

    if (m_appContext->authContext()->currentUser().has_value()) {
        const auto& user = *m_appContext->authContext()->currentUser();
        QString roleStr = user.isSuperAdmin() ? "SuperAdmin" :
            (!user.roles().empty() ? QString::fromStdString(user.roles().front().code()) : "User");
        m_userBadge->setText(QString::fromStdString(user.username()) + " (" + roleStr + ")");
    }
}

void MainWindow::onDestinationChanged(NavDestination dest) {
    switch (dest) {
        case NavDestination::Dashboard:
            m_dashboardView->refresh();
            m_stackedWidget->setCurrentWidget(m_dashboardView);
            break;
        case NavDestination::Users:
            m_usersView->refresh();
            m_stackedWidget->setCurrentWidget(m_usersView);
            break;
        case NavDestination::Audit:
            m_auditView->refresh();
            m_stackedWidget->setCurrentWidget(m_auditView);
            break;
        default:
            m_stackedWidget->setCurrentWidget(m_placeholderView);
            break;
    }
}

void MainWindow::onUserLoggedIn(const QString& username, const QStringList& roles) {
    Q_UNUSED(roles);
    m_userBadge->setText(username);
    m_sidebar->refreshRoleNavigation();
    m_dashboardView->refresh();
    m_stackedWidget->setCurrentWidget(m_dashboardView);
    show();
}

void MainWindow::onUserLoggedOut() {
    hide();
    LoginDialog login(m_appContext, this);
    if (login.exec() == QDialog::Accepted) {
        show();
    } else {
        close();
    }
}

void MainWindow::onSessionLocked() {
    hide();
    LoginDialog login(m_appContext, this);
    if (login.exec() == QDialog::Accepted) {
        show();
    } else {
        close();
    }
}

void MainWindow::onAutoLockTick(int secondsRemaining) {
    m_autoLockLabel->setText(QString(trText("header.autolock")).arg(secondsRemaining));
}

void MainWindow::onLanguageToggleClicked() {
    auto& loc = LocalizationManager::instance();
    if (loc.currentLanguage() == "ar") {
        loc.setLanguage("en");
    } else {
        loc.setLanguage("ar");
    }
}

void MainWindow::onChangePasswordClicked() {
    ChangePasswordDialog dlg(m_appContext, this);
    dlg.exec();
}

void MainWindow::onLogoutClicked() {
    auto reply = QMessageBox::question(
        this,
        LocalizationManager::instance().isRtl() ? "تسجيل الخروج" : "Sign Out",
        LocalizationManager::instance().isRtl() ? "هل أنت متأكد من رغبتك في تسجيل الخروج؟" : "Are you sure you want to sign out?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_appContext->logout();
    }
}

} // namespace pixcode::desktop

