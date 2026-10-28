#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QTimeEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class ScheduleSessionDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScheduleSessionDialog(std::shared_ptr<ApplicationContext> appContext,
                                  std::int64_t groupId,
                                  QWidget* parent = nullptr);

signals:
    void sessionScheduled(std::int64_t sessionId);

private slots:
    void onSaveClicked();

private:
    void setupUi();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::int64_t m_groupId;

    QDateEdit* m_dateEdit;
    QTimeEdit* m_startTimeEdit;
    QTimeEdit* m_endTimeEdit;
    QSpinBox* m_durationSpin;
    QTextEdit* m_notesEdit;
    QPushButton* m_saveBtn;
    QPushButton* m_cancelBtn;
};

} // namespace pixcode::desktop
