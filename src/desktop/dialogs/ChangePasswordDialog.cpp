#include "ChangePasswordDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

ChangePasswordDialog::ChangePasswordDialog(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QDialog(parent), m_appContext(std::move(appContext)) {
    setupUi();
}

void ChangePasswordDialog::setupUi() {
    setWindowTitle(LocalizationManager::instance().isRtl() ? "تغيير كلمة المرور" : "Change Password");
    setFixedSize(420, 360);
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 28, 28, 28);
    mainLayout->setSpacing(16);

    auto* titleLabel = new QLabel(this);
    titleLabel->setObjectName("headerTitle");
    titleLabel->setText(LocalizationManager::instance().isRtl() ? "تحديث كلمة المرور" : "Update Password");

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    m_currentPasswordEdit = new QLineEdit(this);
    m_currentPasswordEdit->setEchoMode(QLineEdit::Password);

    m_newPasswordEdit = new QLineEdit(this);
    m_newPasswordEdit->setEchoMode(QLineEdit::Password);

    m_confirmPasswordEdit = new QLineEdit(this);
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);

    formLayout->addRow(LocalizationManager::instance().isRtl() ? "كلمة المرور الحالية:" : "Current Password:", m_currentPasswordEdit);
    formLayout->addRow(LocalizationManager::instance().isRtl() ? "كلمة المرور الجديدة:" : "New Password:", m_newPasswordEdit);
    formLayout->addRow(LocalizationManager::instance().isRtl() ? "تأكيد كلمة المرور:" : "Confirm Password:", m_confirmPasswordEdit);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName("errorLabel");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();

    m_submitButton = new QPushButton(LocalizationManager::instance().isRtl() ? "حفظ التغييرات" : "Save Changes", this);
    m_submitButton->setMinimumHeight(42);
    connect(m_submitButton, &QPushButton::clicked, this, &ChangePasswordDialog::onChangeClicked);

    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_errorLabel);
    mainLayout->addStretch();
    mainLayout->addWidget(m_submitButton);
}

void ChangePasswordDialog::onChangeClicked() {
    m_errorLabel->hide();

    if (!m_appContext->authContext()->currentUser().has_value()) {
        reject();
        return;
    }

    const auto userId = m_appContext->authContext()->currentUser()->id();
    application::ChangePasswordCommand cmd{
        .userId = userId,
        .currentPassword = m_currentPasswordEdit->text().toStdString(),
        .newPassword = m_newPasswordEdit->text().toStdString(),
        .confirmNewPassword = m_confirmPasswordEdit->text().toStdString()
    };

    auto result = m_appContext->changePasswordUseCase()->execute(cmd);
    if (result.isError()) {
        m_errorLabel->setText(QString::fromStdString(result.error().message()));
        m_errorLabel->show();
        return;
    }

    QMessageBox::information(this, "Success",
                             LocalizationManager::instance().isRtl()
                                 ? "تم تحديث كلمة المرور بنجاح."
                                 : "Password updated successfully.");
    accept();
}

} // namespace pixcode::desktop

