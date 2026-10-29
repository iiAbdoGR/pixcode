#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
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

private:
    void setupUi();
    void loadUserData();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::optional<std::string> m_userId;

    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QComboBox* m_roleCombo;
    QCheckBox* m_activeCheck;

    QPushButton* m_saveBtn;
    QPushButton* m_cancelBtn;
};

} // namespace pixcode::desktop
