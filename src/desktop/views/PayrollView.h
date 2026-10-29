#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTabWidget>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class PayrollView : public QWidget {
    Q_OBJECT

public:
    explicit PayrollView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

    void refresh();

private slots:
    void onInstructorChanged(int index);
    void onUpdateRateClicked();
    void onIssuePayoutClicked();
    void onApproveSessionClicked();
    void onSessionSelectionChanged();

private:
    void setupUi();
    void updateTranslations();
    void loadInstructors();
    void loadInstructorData(std::int64_t instructorId);

    std::shared_ptr<ApplicationContext> m_appContext;
    std::int64_t m_currentInstructorId{0};

    QLabel* m_titleLabel;
    QComboBox* m_instructorCombo;
    QLabel* m_rateLabel;
    QPushButton* m_updateRateBtn;
    QPushButton* m_issuePayoutBtn;
    QPushButton* m_approveSessionBtn;

    // KPI Cards
    QLabel* m_sessionsCountLabel;
    QLabel* m_hoursLabel;
    QLabel* m_earnedLabel;
    QLabel* m_paidLabel;
    QLabel* m_dueLabel;
    QLabel* m_settlementStatusLabel;

    QTabWidget* m_tabWidget;
    QTableWidget* m_sessionsTable;
    QTableWidget* m_archivedSessionsTable;
    QTableWidget* m_payoutsTable;
};

} // namespace pixcode::desktop
