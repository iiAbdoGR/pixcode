#include "MainWindow.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFrame>
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
    resize(1280, 800);
    setMinimumSize(1024, 680);

    auto* centralWidget = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 1. Top Header
    auto* headerFrame = new QFrame(this);
    headerFrame->setFixedHeight(64);
    headerFrame->setStyleSheet("background-color: #2E3A44; border-bottom: 2px solid #CBD2D9;");

    auto* headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(24, 0, 24, 0);
    headerLayout->setSpacing(16);

    m_brandLabel = new QLabel("PIX CODE ACADEMY", headerFrame);
    m_brandLabel->setStyleSheet("color: #FFFFFF; font-size: 18px; font-weight: 800; letter-spacing: 1px;");

    m_userBadge = new QLabel(headerFrame);
    m_userBadge->setStyleSheet("color: #DDE4E1; font-weight: 600; font-size: 13px;");

    m_autoLockLabel = new QLabel(headerFrame);
    m_autoLockLabel->setStyleSheet("color: #B0C2C6; font-size: 12px;");

    m_changePasswordBtn = new QPushButton(headerFrame);
    m_changePasswordBtn->setObjectName("secondaryButton");
    connect(m_changePasswordBtn, &QPushButton::clicked, this, &MainWindow::onChangePasswordClicked);

    m_langToggleBtn = new QPushButton(headerFrame);
    m_langToggleBtn->setObjectName("secondaryButton");
    connect(m_langToggleBtn, &QPushButton::clicked, this, &MainWindow::onLanguageToggleClicked);

    m_logoutBtn = new QPushButton(headerFrame);
    m_logoutBtn->setObjectName("dangerButton");
    connect(m_logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);

    headerLayout->addWidget(m_brandLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_userBadge);
    headerLayout->addWidget(m_autoLockLabel);
    headerLayout->addWidget(m_changePasswordBtn);
    headerLayout->addWidget(m_langToggleBtn);
    headerLayout->addWidget(m_logoutBtn);

    // 2. Central Layout: Sidebar + Stacked Widget
    auto* bodyWidget = new QWidget(centralWidget);
    auto* bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    m_sidebar = new NavigationSidebar(m_appContext, bodyWidget);
    connect(m_sidebar, &NavigationSidebar::destinationChanged, this, &MainWindow::onDestinationChanged);

    m_stackedWidget = new QStackedWidget(bodyWidget);

    m_dashboardView = new DashboardView(m_appContext, this);
    m_registrationView = new RegistrationView(m_appContext, this);
    m_studentsView = new StudentsView(m_appContext, this);
    m_coursesView = new CoursesView(m_appContext, this);
    m_attendanceView = new AttendanceView(m_appContext, this);
    m_financeView = new FinanceView(m_appContext, this);
    m_payrollView = new PayrollView(m_appContext, this);
    m_usersView = new UsersView(m_appContext, this);
    m_auditView = new AuditView(m_appContext, this);

    m_placeholderView = new QWidget(this);
    auto* plLayout = new QVBoxLayout(m_placeholderView);
    auto* plLabel = new QLabel("Module Scheduled for Subsequent Phase", m_placeholderView);
    plLabel->setAlignment(Qt::AlignCenter);
    plLabel->setStyleSheet("font-size: 18px; color: #7E939C; font-weight: 600;");
    plLayout->addWidget(plLabel);

    // Map destinations to stacked widget
    m_stackedWidget->addWidget(m_dashboardView);    // 0: Dashboard
    m_stackedWidget->addWidget(m_registrationView); // 1: Registration / Enrollments
    m_stackedWidget->addWidget(m_studentsView);     // 2: Students
    m_stackedWidget->addWidget(m_coursesView);      // 3: Courses & Groups
    m_stackedWidget->addWidget(m_attendanceView);   // 4: Attendance
    m_stackedWidget->addWidget(m_financeView);      // 5: Finance & Receipts
    m_stackedWidget->addWidget(m_payrollView);      // 6: Payroll
    m_stackedWidget->addWidget(m_usersView);        // 7: Users
    m_stackedWidget->addWidget(m_auditView);        // 8: Audit
    m_stackedWidget->addWidget(m_placeholderView);  // 9: Placeholder

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
        case NavDestination::Registration:
            m_registrationView->refresh();
            m_stackedWidget->setCurrentWidget(m_registrationView);
            break;
        case NavDestination::Students:
            m_studentsView->refresh();
            m_stackedWidget->setCurrentWidget(m_studentsView);
            break;
        case NavDestination::Courses:
            m_coursesView->refresh();
            m_stackedWidget->setCurrentWidget(m_coursesView);
            break;
        case NavDestination::Attendance:
            m_attendanceView->refresh();
            m_stackedWidget->setCurrentWidget(m_attendanceView);
            break;
        case NavDestination::Finance:
            m_financeView->refresh();
            m_stackedWidget->setCurrentWidget(m_financeView);
            break;
        case NavDestination::Payroll:
            m_payrollView->refresh();
            m_stackedWidget->setCurrentWidget(m_payrollView);
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
    m_autoLockLabel->setText("LOCKED");
    LoginDialog login(m_appContext, this);
    login.setWindowTitle(LocalizationManager::instance().isRtl() ? "الجلسة مقفلة" : "Session Locked");
    if (login.exec() == QDialog::Accepted) {
        m_autoLockLabel->setText("");
    } else {
        close();
    }
}

void MainWindow::onAutoLockTick(int secondsRemaining) {
    if (secondsRemaining <= 60) {
        m_autoLockLabel->setText(QString("Lock in %1s").arg(secondsRemaining));
        m_autoLockLabel->setStyleSheet("color: #ED6C02; font-weight: bold; font-size: 12px;");
    } else {
        m_autoLockLabel->setText("");
    }
}

void MainWindow::onLanguageToggleClicked() {
    auto& loc = LocalizationManager::instance(); loc.setLanguage(loc.currentLanguage() == "ar" ? "en" : "ar");
}

void MainWindow::onChangePasswordClicked() {
    ChangePasswordDialog dlg(m_appContext, this);
    dlg.exec();
}

void MainWindow::onLogoutClicked() {
    m_appContext->logout();
}

} // namespace pixcode::desktop
