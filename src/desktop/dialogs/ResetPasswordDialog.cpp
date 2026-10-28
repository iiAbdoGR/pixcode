#include "ResetPasswordDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

ResetPasswordDialog::ResetPasswordDialog(std::shared_ptr<ApplicationContext> appContext,
                                         const std::string& userId,
                                         const std::string& username,
                                         QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_userId(userId),
      m_username(username) {

    setupUi();
}

void ResetPasswordDialog::setupUi() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setWindowTitle(isRtl ? "إعادة تعيين كلمة المرور لموظف" : "Reset Staff Password");
    setMinimumWidth(400);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    auto* userLabel = new QLabel(QString(isRtl ? "إعادة تعيين كلمة مرور المستخدم: <b>%1</b>" : "Reset password for user: <b>%1</b>")
                                .arg(QString::fromStdString(m_username)), this);
    mainLayout->addWidget(userLabel);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    m_newPasswordEdit = new QLineEdit(this);
    m_newPasswordEdit->setEchoMode(QLineEdit::Password);
    m_newPasswordEdit->setPlaceholderText(isRtl ? "كلمة المرور الجديدة (8 أحرف على الأقل)" : "New password (min 8 chars)");

    m_confirmPasswordEdit = new QLineEdit(this);
    m_confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    m_confirmPasswordEdit->setPlaceholderText(isRtl ? "تأكيد كلمة المرور" : "Confirm new password");

    m_mustChangeCheck = new QCheckBox(isRtl ? "إلزام المستخدم بتغيير كلمة المرور عند أول تسجيل دخول" : "Force password change on next sign in", this);
    m_mustChangeCheck->setChecked(true);

    formLayout->addRow(isRtl ? "كلمة المرور الجديدة:" : "New Password:", m_newPasswordEdit);
    formLayout->addRow(isRtl ? "تأكيد كلمة المرور:" : "Confirm Password:", m_confirmPasswordEdit);
    formLayout->addRow("", m_mustChangeCheck);

    mainLayout->addLayout(formLayout);

    auto* btnLayout = new QHBoxLayout();
    m_resetBtn = new QPushButton(isRtl ? "تعيين كلمة المرور" : "Set Password", this);
    m_resetBtn->setObjectName("dangerButton");
    m_cancelBtn = new QPushButton(isRtl ? "إلغاء" : "Cancel", this);
    m_cancelBtn->setObjectName("secondaryButton");

    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_resetBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_resetBtn, &QPushButton::clicked, this, &ResetPasswordDialog::onResetClicked);
}

void ResetPasswordDialog::onResetClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    QString p1 = m_newPasswordEdit->text();
    QString p2 = m_confirmPasswordEdit->text();

    if (p1.length() < 8) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Validation Error",
            isRtl ? "كلمة المرور يجب ألا تقل عن 8 أحرف." : "Password must be at least 8 characters.");
        return;
    }

    if (p1 != p2) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Validation Error",
            isRtl ? "كلمتا المرور غير متطابقتين." : "Passwords do not match.");
        return;
    }

    application::ResetStaffPasswordCommand cmd;
    cmd.userId = m_userId;
    cmd.newPassword = p1.toStdString();
    cmd.mustChangePassword = m_mustChangeCheck->isChecked();

    auto res = m_appContext->staffUseCases()->resetPassword(cmd);
    if (!res.isOk()) {
        QMessageBox::critical(this, isRtl ? "فشل" : "Error",
            QString::fromStdString(res.error().message()));
        return;
    }

    QMessageBox::information(this, isRtl ? "تم بنجاح" : "Success",
        isRtl ? "تمت إعادة تعيين كلمة المرور بنجاح." : "Password reset successfully.");

    emit passwordResetSuccess();
    accept();
}

} // namespace pixcode::desktop
