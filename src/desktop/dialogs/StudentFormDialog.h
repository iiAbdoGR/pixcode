#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>
#include <memory>
#include <optional>
#include "../app/ApplicationContext.h"
#include "application/dto/StudentDtos.h"

namespace pixcode::desktop {

class StudentFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit StudentFormDialog(std::shared_ptr<ApplicationContext> appContext,
                               std::optional<std::int64_t> studentId = std::nullopt,
                               QWidget* parent = nullptr);

signals:
    void studentSaved(std::int64_t studentId);

private slots:
    void onSaveClicked();

private:
    void setupUi();
    void loadStudentData();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::optional<std::int64_t> m_studentId;

    QLineEdit* m_nameEdit;
    QLineEdit* m_phoneEdit;
    QLineEdit* m_guardianNameEdit;
    QLineEdit* m_guardianPhoneEdit;
    QLineEdit* m_addressEdit;
    QTextEdit* m_notesEdit;
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

} // namespace pixcode::desktop
