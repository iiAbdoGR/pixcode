#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QTimeEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
#include <memory>
#include <optional>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class ScheduleSessionDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScheduleSessionDialog(std::shared_ptr<ApplicationContext> appContext,
                                  std::int64_t groupId,
                                  std::optional<std::int64_t> sessionId = std::nullopt,
                                  QWidget* parent = nullptr);

signals:
    void sessionScheduled(std::int64_t sessionId);

private slots:
    void onSaveClicked();
    void onStartTimeChanged(const QTime& time);
    void onEndTimeChanged(const QTime& time);
    void onDurationChanged(int minutes);

private:
    void setupUi();
    void loadData();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::int64_t m_groupId;
    std::optional<std::int64_t> m_sessionId;

    QDateEdit* m_dateEdit;
    QTimeEdit* m_startTimeEdit;
    QTimeEdit* m_endTimeEdit;
    QSpinBox* m_durationSpin;
    QComboBox* m_instructorCombo;
    QTextEdit* m_notesEdit;
    QPushButton* m_saveBtn;
    QPushButton* m_cancelBtn;

    bool m_syncingTime{false};
};

} // namespace pixcode::desktop
