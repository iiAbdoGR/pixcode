#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

private slots:
    void onLoginClicked();
    void onToggleLanguageClicked();

private:
    void setupUi();
    void updateTranslations();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_brandLabel;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QLabel* m_errorLabel;
    QPushButton* m_loginButton;
    QPushButton* m_langButton;
};

} // namespace pixcode::desktop

