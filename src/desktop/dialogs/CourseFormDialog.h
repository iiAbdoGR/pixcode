#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QPushButton>
#include <memory>
#include <optional>
#include "../app/ApplicationContext.h"
#include "application/dto/CourseDtos.h"

namespace pixcode::desktop {

class CourseFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit CourseFormDialog(std::shared_ptr<ApplicationContext> appContext,
                              std::optional<std::int64_t> courseId = std::nullopt,
                              QWidget* parent = nullptr);

signals:
    void courseSaved(std::int64_t courseId);

private slots:
    void onSaveClicked();

private:
    void setupUi();
    void loadCourseData();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::optional<std::int64_t> m_courseId;

    QLineEdit* m_nameEdit;
    QTextEdit* m_descriptionEdit;
    QSpinBox* m_durationSpinBox;
    QLineEdit* m_feeEdit;

    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

} // namespace pixcode::desktop
