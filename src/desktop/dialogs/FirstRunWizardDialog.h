#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class FirstRunWizardDialog : public QDialog {
    Q_OBJECT

public:
    explicit FirstRunWizardDialog(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

private slots:
    void onSubmitClicked();

private:
    void setupUi();
    void updateTranslations();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QLineEdit* m_confirmPasswordEdit;
    QComboBox* m_languageCombo;
    QLabel* m_errorLabel;
    QPushButton* m_submitButton;
};

} // namespace pixcode::desktop

