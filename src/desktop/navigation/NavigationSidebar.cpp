#include "NavigationSidebar.h"
#include "../localization/LocalizationManager.h"
#include <QtWidgets/QLabel>

namespace pixcode::desktop {

NavigationSidebar::NavigationSidebar(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QFrame(parent),
      m_appContext(std::move(appContext)),
      m_currentDestination(NavDestination::Dashboard) {
    setObjectName("navigationSidebar");
    setFixedWidth(240);
    setupUi();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &NavigationSidebar::updateTranslations);
}

void NavigationSidebar::setupUi() {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 16, 0, 16);
    m_layout->setSpacing(4);

    // Header / Brand Title
    auto* brandLabel = new QLabel(" PIX CODE", this);
    brandLabel->setStyleSheet("color: #FFFFFF; font-weight: 800; font-size: 16px; padding: 12px 20px;");
    m_layout->addWidget(brandLabel);
    m_layout->addSpacing(8);

    m_buttons.push_back({NavDestination::Dashboard, createNavButton(NavDestination::Dashboard, "nav.dashboard")});
    m_buttons.push_back({NavDestination::Registration, createNavButton(NavDestination::Registration, "nav.registration")});
    m_buttons.push_back({NavDestination::Students, createNavButton(NavDestination::Students, "nav.students")});
    m_buttons.push_back({NavDestination::Courses, createNavButton(NavDestination::Courses, "nav.courses")});
    m_buttons.push_back({NavDestination::Instructors, createNavButton(NavDestination::Instructors, "nav.instructors")});
    m_buttons.push_back({NavDestination::Attendance, createNavButton(NavDestination::Attendance, "nav.attendance")});
    m_buttons.push_back({NavDestination::Finance, createNavButton(NavDestination::Finance, "nav.finance")});
    m_buttons.push_back({NavDestination::Payroll, createNavButton(NavDestination::Payroll, "nav.payroll")});
    m_buttons.push_back({NavDestination::Users, createNavButton(NavDestination::Users, "nav.users")});
    m_buttons.push_back({NavDestination::Audit, createNavButton(NavDestination::Audit, "nav.audit")});
    m_buttons.push_back({NavDestination::Backup, createNavButton(NavDestination::Backup, "nav.backup")});

    for (const auto& [dest, btn] : m_buttons) {
        m_layout->addWidget(btn);
    }

    m_layout->addStretch();
    refreshRoleNavigation();
}

QPushButton* NavigationSidebar::createNavButton(NavDestination dest, const QString& key) {
    auto* btn = new QPushButton(trText(key), this);
    btn->setObjectName("navButton");
    btn->setCheckable(true);
    btn->setProperty("dest", static_cast<int>(dest));
    btn->setProperty("i18nKey", key);

    connect(btn, &QPushButton::clicked, this, &NavigationSidebar::onButtonClicked);
    return btn;
}

void NavigationSidebar::refreshRoleNavigation() {
    const auto& auth = m_appContext->authContext();
    if (!auth || !auth->currentUser().has_value()) {
        return;
    }

    const auto& user = *auth->currentUser();
    const bool isSuper = user.isSuperAdmin();

    for (auto& [dest, btn] : m_buttons) {
        bool visible = false;

        switch (dest) {
            case NavDestination::Dashboard:
                visible = true; // Everyone sees their role-tailored dashboard
                break;
            case NavDestination::Registration:
                visible = isSuper || user.hasPermission("students.create");
                break;
            case NavDestination::Students:
                visible = isSuper || user.hasPermission("students.read");
                break;
            case NavDestination::Courses:
                visible = isSuper || user.hasPermission("courses.read") || user.hasPermission("groups.read.all") || user.hasPermission("groups.read.assigned");
                break;
            case NavDestination::Instructors:
                visible = isSuper || user.hasPermission("instructors.manage") || user.hasPermission("courses.read");
                break;
            case NavDestination::Attendance:
                visible = isSuper || user.hasPermission("attendance.student.mark") || user.hasPermission("attendance.instructor.confirm");
                break;
            case NavDestination::Finance:
                // General Finance is strictly restricted to Owner
                visible = isSuper || user.isOwner();
                break;
            case NavDestination::Payroll:
                visible = isSuper || user.hasPermission("payroll.read") || user.hasPermission("payroll.pay_instructor");
                break;
            case NavDestination::Users:
                visible = isSuper || user.hasPermission("system.users.manage");
                break;
            case NavDestination::Audit:
                visible = isSuper || user.hasPermission("system.audit.read");
                break;
            case NavDestination::Backup:
                visible = isSuper || user.hasPermission("system.backup.manage");
                break;
        }

        btn->setVisible(visible);
        btn->setChecked(dest == m_currentDestination);
    }
}

void NavigationSidebar::onButtonClicked() {
    auto* senderBtn = qobject_cast<QPushButton*>(sender());
    if (!senderBtn) return;

    const auto dest = static_cast<NavDestination>(senderBtn->property("dest").toInt());
    m_currentDestination = dest;

    for (auto& [d, btn] : m_buttons) {
        btn->setChecked(d == dest);
    }

    emit destinationChanged(dest);
}

void NavigationSidebar::updateTranslations() {
    for (auto& [dest, btn] : m_buttons) {
        const QString key = btn->property("i18nKey").toString();
        if (!key.isEmpty()) {
            btn->setText(trText(key));
        }
    }
}

} // namespace pixcode::desktop

