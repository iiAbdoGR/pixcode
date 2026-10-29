#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
#include <memory>
#include <string>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class ResetPasswordDialog : public QDialog {
    Q_OBJECT

public:
    explicit ResetPasswordDialog(std::shared_ptr<ApplicationContext> appContext,
                                const std::string& userId,
                                const std::string& username,
                                QWidget* parent = nullptr);

signals:
    void passwordResetSuccess();

private slots:
    void onResetClicked();

private:
    void setupUi();

    std::shared_ptr<ApplicationContext> m_appContext;
    std::string m_userId;
    std::string m_username;

    QLineEdit* m_newPasswordEdit;
    QLineEdit* m_confirmPasswordEdit;
    QCheckBox* m_mustChangeCheck;
    QPushButton* m_resetBtn;
    QPushButton* m_cancelBtn;
};

} // namespace pixcode::desktop

