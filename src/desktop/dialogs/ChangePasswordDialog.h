#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class ChangePasswordDialog : public QDialog {
    Q_OBJECT

public:
    explicit ChangePasswordDialog(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

private slots:
    void onChangeClicked();

private:
    void setupUi();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLineEdit* m_currentPasswordEdit;
    QLineEdit* m_newPasswordEdit;
    QLineEdit* m_confirmPasswordEdit;
    QLabel* m_errorLabel;
    QPushButton* m_submitButton;
};

} // namespace pixcode::desktop

