#include "DashboardView.h"
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

DashboardView::DashboardView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QWidget(parent), m_appContext(std::move(appContext)) {
    setupUi();
    updateTranslations();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &DashboardView::updateTranslations);
}

void DashboardView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(24);

    // Welcome Card
    auto* welcomeCard = new QFrame(this);
    welcomeCard->setStyleSheet("background-color: #FFFFFF; border-radius: 8px; border: 1px solid #CBD2D9;");
    auto* cardLayout = new QVBoxLayout(welcomeCard);
    cardLayout->setContentsMargins(24, 24, 24, 24);
    cardLayout->setSpacing(8);

    m_welcomeLabel = new QLabel(this);
    m_welcomeLabel->setObjectName("headerTitle");

    m_roleBadge = new QLabel(this);
    m_roleBadge->setStyleSheet("background-color: #DDE4E1; color: #2E3A44; font-weight: 700; "
                               "border-radius: 4px; padding: 4px 10px; max-width: 140px;");

    cardLayout->addWidget(m_welcomeLabel);
    cardLayout->addWidget(m_roleBadge);

    // System Status Grid
    auto* statsGrid = new QGridLayout();
    statsGrid->setSpacing(16);

    auto makeStatBox = [](const QString& title, QLabel*& valLabel) {
        auto* box = new QFrame();
        box->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD2D9; border-radius: 8px; padding: 16px;");
        auto* l = new QVBoxLayout(box);
        auto* t = new QLabel(title);
        t->setStyleSheet("color: #52606D; font-size: 13px; font-weight: 600;");
        valLabel = new QLabel("-");
        valLabel->setStyleSheet("font-size: 22px; font-weight: 800; color: #2E3A44;");
        l->addWidget(t);
        l->addWidget(valLabel);
        return box;
    };

    statsGrid->addWidget(makeStatBox("Total Users", m_userCountLabel), 0, 0);
    statsGrid->addWidget(makeStatBox("SuperAdmins", m_adminCountLabel), 0, 1);
    statsGrid->addWidget(makeStatBox("Database Status", m_dbHealthLabel), 0, 2);

    mainLayout->addWidget(welcomeCard);
    mainLayout->addLayout(statsGrid);
    mainLayout->addStretch();
}

void DashboardView::refresh() {
    const auto& auth = m_appContext->authContext();
    if (!auth || !auth->currentUser().has_value()) return;

    const auto& user = *auth->currentUser();
    const QString username = QString::fromStdString(user.username());

    QString roleStr = "User";
    if (user.isSuperAdmin()) {
        roleStr = "SuperAdmin (Owner)";
    } else if (!user.roles().empty()) {
        roleStr = QString::fromStdString(user.roles().front().code());
    }

    const QString greeting = LocalizationManager::instance().isRtl()
        ? QString("أهلاً وسهلاً، %1").arg(username)
        : QString("Welcome back, %1").arg(username);

    m_welcomeLabel->setText(greeting);
    m_roleBadge->setText(roleStr);

    // Refresh system counts
    const int totalUsers = m_appContext->userRepository()->countTotalUsers();
    const int superAdmins = m_appContext->userRepository()->countActiveSuperAdmins();

    m_userCountLabel->setText(QString::number(totalUsers));
    m_adminCountLabel->setText(QString::number(superAdmins));

    QString dbCheck;
    if (m_appContext->databaseManager()->checkIntegrity(dbCheck)) {
        m_dbHealthLabel->setText("WAL / Healthy");
        m_dbHealthLabel->setStyleSheet("font-size: 20px; font-weight: 800; color: #2E7D32;");
    } else {
        m_dbHealthLabel->setText("Error");
        m_dbHealthLabel->setStyleSheet("font-size: 20px; font-weight: 800; color: #D32F2F;");
    }
}

void DashboardView::updateTranslations() {
    refresh();
}

} // namespace pixcode::desktop

