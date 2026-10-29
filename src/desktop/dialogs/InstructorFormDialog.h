#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>
#include <memory>
#include <optional>
#include "../app/ApplicationContext.h"
#include "application/dto/EnrollmentDtos.h"

namespace pixcode::desktop {

class InstructorFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit InstructorFormDialog(std::shared_ptr<ApplicationContext> appContext,
                                  std::optional<std::int64_t> instructorId = std::nullopt,
                                  QWidget* parent = nullptr);

signals:
    void instructorSaved(const application::InstructorDto& instructor);

private slots:
    void onSaveClicked();

private:
    void setupUi();
    void loadData();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::optional<std::int64_t> m_instructorId;

    QLineEdit* m_nameEdit;
    QLineEdit* m_phoneEdit;
    QLineEdit* m_specEdit;
    QComboBox* m_compTypeCombo;
    QLineEdit* m_baseRateEdit;
    QLineEdit* m_monthlySalaryEdit;
    QLineEdit* m_startDateEdit;
    QLineEdit* m_endDateEdit;
    QComboBox* m_statusCombo;
    QTextEdit* m_notesEdit;

    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

} // namespace pixcode::desktop

