#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <memory>
#include "../app/ApplicationContext.h"
#include "../navigation/NavigationSidebar.h"
#include "DashboardView.h"
#include "UsersView.h"
#include "AuditView.h"

namespace pixcode::desktop {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

private slots:
    void onDestinationChanged(NavDestination dest);
    void onUserLoggedIn(const QString& username, const QStringList& roles);
    void onUserLoggedOut();
    void onSessionLocked();
    void onAutoLockTick(int secondsRemaining);
    void onLanguageToggleClicked();
    void onChangePasswordClicked();
    void onLogoutClicked();

private:
    void setupUi();
    void updateTranslations();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_brandLabel;
    QLabel* m_userBadge;
    QLabel* m_autoLockLabel;
    QPushButton* m_changePasswordBtn;
    QPushButton* m_langToggleBtn;
    QPushButton* m_logoutBtn;

    NavigationSidebar* m_sidebar;
    QStackedWidget* m_stackedWidget;

    DashboardView* m_dashboardView;
    UsersView* m_usersView;
    AuditView* m_auditView;
    QWidget* m_placeholderView;
};

} // namespace pixcode::desktop

