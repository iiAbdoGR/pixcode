#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <memory>
#include <vector>
#include "../app/ApplicationContext.h"
#include "application/dto/AttendanceDtos.h"

namespace pixcode::desktop {

class AttendanceView : public QWidget {
    Q_OBJECT

public:
    explicit AttendanceView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

    void refresh();

private slots:
    void onGroupChanged(int index);
    void onSessionChanged(int index);
    void onAddSessionClicked();
    void onMarkAllPresentClicked();
    void onSaveAttendanceClicked();

private:
    void setupUi();
    void updateTranslations();
    void loadGroups();
    void loadSessionsForGroup(std::int64_t groupId);
    void loadAttendanceForSession(std::int64_t sessionId);
    void updateStatsLabel(const application::SessionAttendanceStatsDto& stats);

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_titleLabel;
    QComboBox* m_groupCombo;
    QComboBox* m_sessionCombo;
    QPushButton* m_addSessionBtn;
    QPushButton* m_markAllPresentBtn;
    QPushButton* m_saveAttendanceBtn;
    QLabel* m_statsLabel;
    QTableWidget* m_table;

    std::int64_t m_currentGroupId{0};
    std::int64_t m_currentSessionId{0};
};

} // namespace pixcode::desktop
