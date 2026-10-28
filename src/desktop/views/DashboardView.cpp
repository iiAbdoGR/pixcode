#include "DashboardView.h"
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtSql/QSqlQuery>
#include "../localization/LocalizationManager.h"
#include "domain/value_objects/Money.h"

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
                               "border-radius: 4px; padding: 4px 10px; max-width: 180px;");

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

    statsGrid->addWidget(makeStatBox("الطلاب المسجلين", m_studentsCountLabel), 0, 0);
    statsGrid->addWidget(makeStatBox("المجموعات الدراسية المفتوحة", m_groupsCountLabel), 0, 1);
    statsGrid->addWidget(makeStatBox("المتحصلات المالية الإجمالية", m_revenueLabel), 0, 2);
    statsGrid->addWidget(makeStatBox("مستخدمو النظام", m_userCountLabel), 1, 0);
    statsGrid->addWidget(makeStatBox("مسؤولو النظام (Admins)", m_adminCountLabel), 1, 1);
    statsGrid->addWidget(makeStatBox("حالة قاعدة البيانات", m_dbHealthLabel), 1, 2);

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

    // Refresh student counts
    application::StudentFilter sFilter;
    sFilter.status = domain::StudentStatus::Active;
    m_studentsCountLabel->setText(QString::number(m_appContext->studentRepository()->count(sFilter)));

    // Refresh groups count
    application::GroupFilter gFilter;
    gFilter.status = domain::GroupStatus::Open;
    m_groupsCountLabel->setText(QString::number(m_appContext->groupRepository()->count(gFilter)));

    // Refresh revenue
    {
        auto db = m_appContext->databaseManager()->getConnection();
        QSqlQuery q(db);
        q.exec("SELECT COALESCE(SUM(amount_piastres), 0) FROM payments;");
        if (q.next()) {
            domain::Money collected(q.value(0).toLongLong());
            m_revenueLabel->setText(QString::fromStdString(collected.format()));
            m_revenueLabel->setStyleSheet("font-size: 22px; font-weight: 800; color: #2E7D32;");
        }
    }

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
