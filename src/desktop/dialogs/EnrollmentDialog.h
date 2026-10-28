#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <memory>
#include <optional>
#include "../app/ApplicationContext.h"
#include "application/dto/EnrollmentDtos.h"

namespace pixcode::desktop {

class EnrollmentDialog : public QDialog {
    Q_OBJECT

public:
    explicit EnrollmentDialog(std::shared_ptr<ApplicationContext> appContext,
                              std::optional<std::int64_t> studentId = std::nullopt,
                              std::optional<std::int64_t> groupId = std::nullopt,
                              QWidget* parent = nullptr);

signals:
    void enrollmentCompleted(const application::EnrollmentDto& enrollment);

private slots:
    void onGroupSelectionChanged(int index);
    void onDiscountChanged(const QString& text);
    void onEnrollClicked();

private:
    void setupUi();
    void loadData();
    void updateFinancials();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::optional<std::int64_t> m_preselectedStudentId;
    std::optional<std::int64_t> m_preselectedGroupId;

    QComboBox* m_studentCombo;
    QComboBox* m_groupCombo;
    QLabel* m_groupDetailsLabel;
    QLabel* m_feeLabel;
    QLineEdit* m_discountEdit;
    QLabel* m_netFeeLabel;
    QTextEdit* m_notesEdit;

    QPushButton* m_enrollButton;
    QPushButton* m_cancelButton;

    std::int64_t m_currentGroupFeePiastres{0};
};

} // namespace pixcode::desktop
