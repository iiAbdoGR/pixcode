#include "StaffFormDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QLabel>
#include "../localization/LocalizationManager.h"
#include "../styles/Theme.h"

namespace pixcode::desktop {

StaffFormDialog::StaffFormDialog(std::shared_ptr<ApplicationContext> appContext,
                                 std::optional<std::string> userId,
                                 QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_userId(std::move(userId)) {
    setupUi();
    if (m_userId.has_value()) {
        loadUserData();
    }
}

void StaffFormDialog::setupUi() {
    const bool isEdit = m_userId.has_value();
    setWindowTitle(isEdit
        ? LocalizationManager::instance().get("dialog.staff.edit_title", "Edit User Account")
        : LocalizationManager::instance().get("dialog.staff.create_title", "Add User Account"));
    setMinimumWidth(400);
    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }")
        .arg(Theme::BackgroundLight)
        .arg(Theme::PrimaryDark));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(16);
    rootLayout->setContentsMargins(24, 24, 24, 24);

    auto* titleLabel = new QLabel(isEdit
        ? LocalizationManager::instance().get("dialog.staff.edit_title", "Edit User Account")
        : LocalizationManager::instance().get("dialog.staff.create_title", "Add User Account"), this);
    QFont f = titleLabel->font();
    f.setBold(true);
    f.setPointSize(12);
    titleLabel->setFont(f);
    titleLabel->setStyleSheet(QString("color: %1;").arg(Theme::PrimaryDark));
    rootLayout->addWidget(titleLabel);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    const QString inputStyle = QString("QLineEdit, QComboBox { background-color: white; border: 1px solid %1; border-radius: 4px; padding: 6px; }")
        .arg(Theme::SurfaceBorder);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setStyleSheet(inputStyle);
    if (isEdit) {
        m_usernameEdit->setReadOnly(true);
    }
    formLayout->addRow(LocalizationManager::instance().get("dialog.staff.username", "Username:"), m_usernameEdit);

    if (!isEdit) {
        m_passwordEdit = new QLineEdit(this);
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        m_passwordEdit->setStyleSheet(inputStyle);
        formLayout->addRow(LocalizationManager::instance().get("dialog.staff.password", "Initial Password:"), m_passwordEdit);
    }

    // Role Selection: Strictly Owner and Admin Only
    m_roleCombo = new QComboBox(this);
    m_roleCombo->setStyleSheet(inputStyle);
    m_roleCombo->addItem(LocalizationManager::instance().get("role.admin", "Admin (Operational duties)"), "Admin");
    m_roleCombo->addItem(LocalizationManager::instance().get("role.owner", "Owner (Full access & financials)"), "Owner");
    formLayout->addRow(LocalizationManager::instance().get("dialog.staff.role", "System Role:"), m_roleCombo);

    m_activeCheck = new QCheckBox(LocalizationManager::instance().get("dialog.staff.active", "Account Active"), this);
    m_activeCheck->setChecked(true);
    formLayout->addRow("", m_activeCheck);

    rootLayout->addLayout(formLayout);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_cancelBtn = new QPushButton(LocalizationManager::instance().get("common.cancel", "Cancel"), this);
    m_cancelBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid %2; padding: 8px 16px; border-radius: 4px; }")
        .arg(Theme::SurfaceLight)
        .arg(Theme::SurfaceBorder));
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    m_saveBtn = new QPushButton(LocalizationManager::instance().get("common.save", "Save"), this);
    m_saveBtn->setStyleSheet(QString("QPushButton { background-color: %1; color: white; font-weight: bold; padding: 8px 16px; border-radius: 4px; border: none; } QPushButton:hover { background-color: %2; }")
        .arg(Theme::SecondaryDark)
        .arg(Theme::PrimaryDark));
    connect(m_saveBtn, &QPushButton::clicked, this, &StaffFormDialog::onSaveClicked);
    btnLayout->addWidget(m_saveBtn);

    rootLayout->addLayout(btnLayout);
}

void StaffFormDialog::loadUserData() {
    auto res = m_appContext->staffUseCases()->getStaff(*m_userId);
    if (!res.isOk()) {
        QMessageBox::critical(this, LocalizationManager::instance().get("common.error", "Error"),
                              QString::fromStdString(res.error().message()));
        reject();
        return;
    }

    const auto& dto = res.value();
    m_usernameEdit->setText(QString::fromStdString(dto.username));
    m_activeCheck->setChecked(dto.isActive);

    bool isOwner = false;
    for (const auto& r : dto.roleCodes) {
        if (r == "Owner" || r == "SuperAdmin") isOwner = true;
    }
    int idx = m_roleCombo->findData(isOwner ? "Owner" : "Admin");
    if (idx >= 0) m_roleCombo->setCurrentIndex(idx);
}

void StaffFormDialog::onSaveClicked() {
    QString username = m_usernameEdit->text().trimmed();
    if (username.length() < 3) {
        QMessageBox::warning(this, LocalizationManager::instance().get("common.warning", "Warning"),
                             LocalizationManager::instance().get("dialog.staff.user_short", "Username must be at least 3 characters."));
        return;
    }

    std::string selectedRole = m_roleCombo->currentData().toString().toStdString();
    std::vector<std::string> roles = { selectedRole };

    try {
        if (m_userId.has_value()) {
            application::UpdateStaffCommand cmd{
                .userId = *m_userId,
                .fullName = "",
                .phone = "",
                .roleCodes = roles,
                .isActive = m_activeCheck->isChecked(),
                .preferredLanguage = "ar",
                .specialization = "",
                .hourlyRatePiastres = 0
            };

            auto res = m_appContext->staffUseCases()->updateStaff(cmd);
            if (!res.isOk()) {
                QMessageBox::critical(this, LocalizationManager::instance().get("common.error", "Error"),
                                      QString::fromStdString(res.error().message()));
                return;
            }
        } else {
            QString password = m_passwordEdit ? m_passwordEdit->text() : "";
            if (password.length() < 8) {
                QMessageBox::warning(this, LocalizationManager::instance().get("common.warning", "Warning"),
                                     LocalizationManager::instance().get("dialog.staff.pass_short", "Password must be at least 8 characters."));
                return;
            }

            application::CreateStaffCommand cmd{
                .username = username.toStdString(),
                .password = password.toStdString(),
                .fullName = "",
                .phone = "",
                .preferredLanguage = "ar",
                .roleCodes = roles,
                .linkedInstructorId = std::nullopt,
                .specialization = "",
                .hourlyRatePiastres = 0
            };

            auto res = m_appContext->staffUseCases()->createStaff(cmd);
            if (!res.isOk()) {
                QMessageBox::critical(this, LocalizationManager::instance().get("common.error", "Error"),
                                      QString::fromStdString(res.error().message()));
                return;
            }
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, LocalizationManager::instance().get("common.error", "Error"),
                              QString::fromStdString(e.what()));
        return;
    }

    emit staffSaved();
    accept();
}

} // namespace pixcode::desktop
