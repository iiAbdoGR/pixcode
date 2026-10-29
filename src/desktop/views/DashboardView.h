#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <memory>
#include "../app/ApplicationContext.h"
#include "application/dto/DashboardDtos.h"

namespace pixcode::desktop {

class DashboardView : public QWidget {
    Q_OBJECT

public:
    explicit DashboardView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

    void refresh();

private:
    void setupUi();
    void updateTranslations();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_welcomeLabel;
    QLabel* m_roleBadge;

    // Operational Cards
    QLabel* m_studentsCountLabel;
    QLabel* m_groupsCountLabel;
    QLabel* m_todaySessionsLabel;
    QLabel* m_attendanceRateLabel;

    // Financial Cards (Owner only)
    QGroupBox* m_financialGroup;
    QLabel* m_totalIncomeLabel;
    QLabel* m_receivablesLabel;
    QLabel* m_instructorPayablesLabel;
    QLabel* m_netCashLabel;
};

} // namespace pixcode::desktop
