#include "DashboardView.h"
#include <QtWidgets/QFrame>
#include <QtWidgets/QScrollArea>
#include "../localization/LocalizationManager.h"
#include "../styles/Theme.h"
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
    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QString("QScrollArea { background-color: %1; }").arg(Theme::BackgroundLight));

    auto* contentWidget = new QWidget(scrollArea);
    contentWidget->setStyleSheet(QString("background-color: %1;").arg(Theme::BackgroundLight));
    auto* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(24);

    // 1. Welcome Card
    auto* welcomeCard = new QFrame(this);
    welcomeCard->setStyleSheet(QString(
        "QFrame { background-color: %1; border-radius: 8px; border: 1px solid %2; padding: 20px; }"
    ).arg(Theme::SurfaceWhite).arg(Theme::SurfaceBorder));

    auto* welcomeLayout = new QHBoxLayout(welcomeCard);
    welcomeLayout->setContentsMargins(12, 12, 12, 12);

    auto* welcomeTextLayout = new QVBoxLayout();
    m_welcomeLabel = new QLabel(this);
    m_welcomeLabel->setObjectName("headerTitle");
    m_welcomeLabel->setStyleSheet(QString("font-size: 20px; font-weight: 800; color: %1;").arg(Theme::PrimaryDark));

    auto* subLabel = new QLabel(this);
    subLabel->setText(LocalizationManager::instance().get("dash.welcome_sub", "Pix Code Academy Management Hub"));
    subLabel->setStyleSheet(QString("font-size: 13px; color: %1;").arg(Theme::TextSecondary));

    welcomeTextLayout->addWidget(m_welcomeLabel);
    welcomeTextLayout->addWidget(subLabel);
    welcomeLayout->addLayout(welcomeTextLayout);
    welcomeLayout->addStretch();

    m_roleBadge = new QLabel(this);
    m_roleBadge->setStyleSheet(QString(
        "background-color: %1; color: %2; font-weight: 700; font-size: 13px; "
        "border-radius: 6px; padding: 6px 14px; border: 1px solid %3;"
    ).arg(Theme::SurfaceLight).arg(Theme::PrimaryDark).arg(Theme::SurfaceBorder));
    welcomeLayout->addWidget(m_roleBadge);

    mainLayout->addWidget(welcomeCard);

    // 2. Operational Metrics Section
    auto* opGroup = new QGroupBox(LocalizationManager::instance().get("dash.section.operational", "Academic Operations"), this);
    opGroup->setStyleSheet(QString(
        "QGroupBox { font-size: 14px; font-weight: 700; color: %1; border: 1px solid %2; "
        "border-radius: 8px; margin-top: 10px; padding-top: 16px; background-color: transparent; } "
        "QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 6px; }"
    ).arg(Theme::PrimaryDark).arg(Theme::SurfaceBorder));

    auto* opGridLayout = new QGridLayout(opGroup);
    opGridLayout->setSpacing(16);
    opGridLayout->setContentsMargins(16, 16, 16, 16);

    auto makeCard = [](const QString& title, QLabel*& valLabel, const QString& accentColor) {
        auto* card = new QFrame();
        card->setStyleSheet(QString(
            "QFrame { background-color: %1; border: 1px solid %2; border-radius: 8px; padding: 16px; }"
        ).arg(Theme::SurfaceWhite).arg(Theme::SurfaceBorder));
        auto* l = new QVBoxLayout(card);
        l->setSpacing(6);
        auto* t = new QLabel(title);
        t->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600;").arg(Theme::TextSecondary));
        valLabel = new QLabel("-");
        valLabel->setStyleSheet(QString("font-size: 24px; font-weight: 800; color: %1;").arg(accentColor));
        l->addWidget(t);
        l->addWidget(valLabel);
        return card;
    };

    opGridLayout->addWidget(makeCard(
        LocalizationManager::instance().get("dash.active_students", "Active Students"),
        m_studentsCountLabel, Theme::Primary), 0, 0);

    opGridLayout->addWidget(makeCard(
        LocalizationManager::instance().get("dash.active_groups", "Open Study Groups"),
        m_groupsCountLabel, Theme::Secondary), 0, 1);

    opGridLayout->addWidget(makeCard(
        LocalizationManager::instance().get("dash.today_sessions", "Today's Sessions"),
        m_todaySessionsLabel, Theme::Slate), 0, 2);

    opGridLayout->addWidget(makeCard(
        LocalizationManager::instance().get("dash.att_rate", "Overall Attendance Rate"),
        m_attendanceRateLabel, Theme::Success), 0, 3);

    mainLayout->addWidget(opGroup);

    // 3. Financial Metrics Section (Strictly Owner Only)
    m_financialGroup = new QGroupBox(LocalizationManager::instance().get("dash.section.financial", "Financial Overview & KPIs (Authorized Only)"), this);
    m_financialGroup->setStyleSheet(QString(
        "QGroupBox { font-size: 14px; font-weight: 700; color: %1; border: 1px solid %2; "
        "border-radius: 8px; margin-top: 10px; padding-top: 16px; background-color: transparent; } "
        "QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 6px; }"
    ).arg(Theme::Primary).arg(Theme::Border));

    auto* finGridLayout = new QGridLayout(m_financialGroup);
    finGridLayout->setSpacing(16);
    finGridLayout->setContentsMargins(16, 16, 16, 16);

    finGridLayout->addWidget(makeCard(
        LocalizationManager::instance().get("dash.total_income", "Total Collected Income"),
        m_totalIncomeLabel, Theme::Success), 0, 0);

    finGridLayout->addWidget(makeCard(
        LocalizationManager::instance().get("dash.receivables", "Outstanding Student Dues"),
        m_receivablesLabel, Theme::Warning), 0, 1);

    finGridLayout->addWidget(makeCard(
        LocalizationManager::instance().get("dash.inst_payables", "Instructor Payables (Approved)"),
        m_instructorPayablesLabel, Theme::Secondary), 0, 2);

    finGridLayout->addWidget(makeCard(
        LocalizationManager::instance().get("dash.net_cash", "Net Operational Cash Balance"),
        m_netCashLabel, Theme::Primary), 0, 3);

    mainLayout->addWidget(m_financialGroup);

    mainLayout->addStretch();
    scrollArea->setWidget(contentWidget);
    outerLayout->addWidget(scrollArea);
}

void DashboardView::refresh() {
    const auto& auth = m_appContext->authContext();
    if (!auth || !auth->currentUser().has_value()) return;

    const auto& user = *auth->currentUser();
    const QString username = QString::fromStdString(user.username());
    const bool isRtl = LocalizationManager::instance().isRtl();

    QString roleStr = isRtl ? "مسؤول النظام (Admin)" : "System Admin";
    if (user.isOwner()) {
        roleStr = isRtl ? "مالك الأكاديمية (Owner)" : "Academy Owner";
    }

    const QString greeting = isRtl
        ? QString("أهلاً وسهلاً، %1").arg(username)
        : QString("Welcome back, %1").arg(username);

    m_welcomeLabel->setText(greeting);
    m_roleBadge->setText(roleStr);

    // Call Application Use Case (Clean architecture, role-scoped)
    auto res = m_appContext->dashboardUseCases()->getDashboardSummary();
    if (!res.isOk()) return;

    const auto& summary = res.value();

    // 1. Populate Operational KPIs
    m_studentsCountLabel->setText(QString::number(summary.operational.activeStudentsCount));
    m_groupsCountLabel->setText(QString::number(summary.operational.activeGroupsCount));
    m_todaySessionsLabel->setText(QString::number(summary.operational.todaySessionsCount));
    m_attendanceRateLabel->setText(QString("%1%").arg(QString::number(summary.operational.attendanceRatePercentage, 'f', 1)));

    // 2. Populate Financial KPIs (Strictly if populated / Owner)
    if (summary.financial.has_value()) {
        m_financialGroup->setVisible(true);
        const auto& fin = *summary.financial;

        domain::Money inc(fin.totalIncomePiastres);
        domain::Money rec(fin.totalReceivablesPiastres);
        domain::Money pay(fin.approvedInstructorPayablesPiastres);
        domain::Money cash(fin.netCashBalancePiastres);

        m_totalIncomeLabel->setText(QString::fromStdString(inc.format()));
        m_receivablesLabel->setText(QString::fromStdString(rec.format()));
        m_instructorPayablesLabel->setText(QString::fromStdString(pay.format()));
        m_netCashLabel->setText(QString::fromStdString(cash.format()));
    } else {
        // Admin: hide the financial section completely
        m_financialGroup->setVisible(false);
    }
}

void DashboardView::updateTranslations() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setLayoutDirection(isRtl ? Qt::RightToLeft : Qt::LeftToRight);
    refresh();
}

} // namespace pixcode::desktop
