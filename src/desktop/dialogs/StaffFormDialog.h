#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGroupBox>
#include <memory>
#include <optional>
#include <string>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class StaffFormDialog : public QDialog {
    Q_OBJECT

public:
    explicit StaffFormDialog(std::shared_ptr<ApplicationContext> appContext,
                             std::optional<std::string> userId = std::nullopt,
                             QWidget* parent = nullptr);

signals:
    void staffSaved();

private slots:
    void onSaveClicked();
    void onRoleToggled();

private:
    void setupUi();
    void loadUserData();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::optional<std::string> m_userId;

    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QLineEdit* m_fullNameEdit;
    QLineEdit* m_phoneEdit;

    QCheckBox* m_roleSuperAdmin;
    QCheckBox* m_roleAdmin;
    QCheckBox* m_roleSecretary;
    QCheckBox* m_roleAccountant;
    QCheckBox* m_roleInstructor;
    QCheckBox* m_activeCheck;

    QGroupBox* m_instructorBox;
    QLineEdit* m_specEdit;
    QDoubleSpinBox* m_hourlyRateSpin;

    QPushButton* m_saveBtn;
    QPushButton* m_cancelBtn;
};

} // namespace pixcode::desktop
