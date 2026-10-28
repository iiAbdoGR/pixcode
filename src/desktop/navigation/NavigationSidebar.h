#pragma once

#include <QtWidgets/QFrame>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <vector>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

enum class NavDestination {
    Dashboard = 0,
    Registration = 1,
    Students = 2,
    Courses = 3,
    Attendance = 4,
    Finance = 5,
    Payroll = 6,
    Users = 7,
    Audit = 8,
    Backup = 9
};

class NavigationSidebar : public QFrame {
    Q_OBJECT

public:
    explicit NavigationSidebar(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

    void refreshRoleNavigation();

signals:
    void destinationChanged(NavDestination destination);

private slots:
    void onButtonClicked();

private:
    void setupUi();
    void updateTranslations();
    QPushButton* createNavButton(NavDestination dest, const QString& key);

    std::shared_ptr<ApplicationContext> m_appContext;
    QVBoxLayout* m_layout;
    std::vector<std::pair<NavDestination, QPushButton*>> m_buttons;
    NavDestination m_currentDestination;
};

} // namespace pixcode::desktop

