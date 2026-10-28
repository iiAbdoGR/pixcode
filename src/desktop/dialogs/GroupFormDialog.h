#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QPushButton>
#include <memory>
#include <optional>
#include "../app/ApplicationContext.h"
#include "application/dto/GroupDtos.h"

namespace pixcode::desktop {

class GroupFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit GroupFormDialog(std::shared_ptr<ApplicationContext> appContext,
                             std::optional<std::int64_t> groupId = std::nullopt,
                             QWidget* parent = nullptr);

signals:
    void groupSaved(std::int64_t groupId);

private slots:
    void onCourseChanged(int index);
    void onSaveClicked();

private:
    void setupUi();
    void loadData();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::optional<std::int64_t> m_groupId;

    QComboBox* m_courseCombo;
    QComboBox* m_instructorCombo;
    QLineEdit* m_nameEdit;
    QLineEdit* m_scheduleEdit;
    QLineEdit* m_startDateEdit;
    QLineEdit* m_endDateEdit;
    QSpinBox*  m_capacitySpinBox;
    QLineEdit* m_feeEdit;
    QComboBox* m_statusCombo;

    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

} // namespace pixcode::desktop

