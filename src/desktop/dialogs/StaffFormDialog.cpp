#include "StaffFormDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"

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
    bool isRtl = LocalizationManager::instance().isRtl();
    setWindowTitle(m_userId.has_value()
        ? (isRtl ? "تعديل حساب موظف والصلاحيات" : "Edit Staff Account & Roles")
        : (isRtl ? "إضافة حساب موظف جديد" : "Add New Staff Account"));
    setMinimumWidth(480);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText(isRtl ? "اسم المستخدم للولوج *" : "Login username *");
    if (m_userId.has_value()) {
        m_usernameEdit->setReadOnly(true);
    }

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(isRtl ? "كلمة المرور (8 أحرف على الأقل) *" : "Password (min 8 chars) *");
    if (m_userId.has_value()) {
        m_passwordEdit->setVisible(false);
    }

    m_fullNameEdit = new QLineEdit(this);
    m_fullNameEdit->setPlaceholderText(isRtl ? "الاسم الرباعي للموظف *" : "Staff Full Name *");

    m_phoneEdit = new QLineEdit(this);
    m_phoneEdit->setPlaceholderText(isRtl ? "رقم الهاتف الشخصي" : "Phone number");

    m_activeCheck = new QCheckBox(isRtl ? "الحساب مفعل ونشط" : "Account Active", this);
    m_activeCheck->setChecked(true);

    formLayout->addRow(isRtl ? "اسم المستخدم *:" : "Username *:", m_usernameEdit);
    if (!m_userId.has_value()) {
        formLayout->addRow(isRtl ? "كلمة المرور *:" : "Password *:", m_passwordEdit);
    }
    formLayout->addRow(isRtl ? "الاسم الكامل *:" : "Full Name *:", m_fullNameEdit);
    formLayout->addRow(isRtl ? "رقم الهاتف:" : "Phone:", m_phoneEdit);
    formLayout->addRow(isRtl ? "الحالة:" : "Status:", m_activeCheck);

    mainLayout->addLayout(formLayout);

    // Roles selection
    auto* rolesBox = new QGroupBox(isRtl ? "الأدوار الوظيفية والصلاحيات" : "Assigned Roles & Permissions", this);
    auto* rolesLayout = new QVBoxLayout(rolesBox);

    m_roleSuperAdmin = new QCheckBox(isRtl ? "مدير النظام / المالك (SuperAdmin / Owner)" : "Owner / SuperAdmin", rolesBox);
    m_roleAdmin = new QCheckBox(isRtl ? "مدير فرع / إداري (Admin)" : "Administrator (Admin)", rolesBox);
    m_roleSecretary = new QCheckBox(isRtl ? "سكرتارية واستقبال (Secretary)" : "Secretary (Front Desk)", rolesBox);
    m_roleAccountant = new QCheckBox(isRtl ? "محاسب / مالية (Accountant)" : "Accountant (Finance)", rolesBox);
    m_roleInstructor = new QCheckBox(isRtl ? "مدرب / محاضر (Instructor)" : "Instructor (Teacher)", rolesBox);

    rolesLayout->addWidget(m_roleSuperAdmin);
    rolesLayout->addWidget(m_roleAdmin);
    rolesLayout->addWidget(m_roleSecretary);
    rolesLayout->addWidget(m_roleAccountant);
    rolesLayout->addWidget(m_roleInstructor);

    mainLayout->addWidget(rolesBox);

    // Instructor specific box
    m_instructorBox = new QGroupBox(isRtl ? "بيانات التدريب والأجر بالساعة" : "Instructor Details & Hourly Rate", this);
    auto* instLayout = new QFormLayout(m_instructorBox);

    m_specEdit = new QLineEdit(m_instructorBox);
    m_specEdit->setPlaceholderText(isRtl ? "مثال: برمجة بايثون / خوارزميات" : "e.g. Python, Web Development");

    m_hourlyRateSpin = new QDoubleSpinBox(m_instructorBox);
    m_hourlyRateSpin->setRange(0.0, 100000.0);
    m_hourlyRateSpin->setDecimals(2);
    m_hourlyRateSpin->setSuffix(isRtl ? " ج.م" : " EGP");

    instLayout->addRow(isRtl ? "التخصص:" : "Specialization:", m_specEdit);
    instLayout->addRow(isRtl ? "أجر الساعة:" : "Hourly Rate:", m_hourlyRateSpin);

    m_instructorBox->setVisible(false);
    mainLayout->addWidget(m_instructorBox);

    connect(m_roleInstructor, &QCheckBox::toggled, this, &StaffFormDialog::onRoleToggled);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    m_saveBtn = new QPushButton(isRtl ? "حفظ" : "Save", this);
    m_saveBtn->setObjectName("primaryButton");
    m_cancelBtn = new QPushButton(isRtl ? "إلغاء" : "Cancel", this);
    m_cancelBtn->setObjectName("secondaryButton");

    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_saveBtn, &QPushButton::clicked, this, &StaffFormDialog::onSaveClicked);
}

void StaffFormDialog::onRoleToggled() {
    m_instructorBox->setVisible(m_roleInstructor->isChecked());
}

void StaffFormDialog::loadUserData() {
    if (!m_userId.has_value()) return;

    auto userOpt = m_appContext->userRepository()->findById(*m_userId);
    if (!userOpt.has_value()) return;

    const auto& user = *userOpt;
    m_usernameEdit->setText(QString::fromStdString(user.username()));
    m_activeCheck->setChecked(user.isActive());

    for (const auto& r : user.roles()) {
        const auto code = r.code();
        if (code == "superadmin") m_roleSuperAdmin->setChecked(true);
        else if (code == "admin") m_roleAdmin->setChecked(true);
        else if (code == "secretary") m_roleSecretary->setChecked(true);
        else if (code == "accountant") m_roleAccountant->setChecked(true);
        else if (code == "instructor") m_roleInstructor->setChecked(true);
    }

    auto instOpt = m_appContext->instructorRepository()->findByUserId(*m_userId);
    if (instOpt.has_value()) {
        m_fullNameEdit->setText(QString::fromStdString(instOpt->fullName));
        m_phoneEdit->setText(QString::fromStdString(instOpt->phone));
        m_specEdit->setText(QString::fromStdString(instOpt->specialization));
        m_hourlyRateSpin->setValue(instOpt->hourlyRate.piasters() / 100.0);
        m_roleInstructor->setChecked(true);
        m_instructorBox->setVisible(true);
    } else {
        m_fullNameEdit->setText(QString::fromStdString(user.username()));
    }
}

void StaffFormDialog::onSaveClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    QString username = m_usernameEdit->text().trimmed();
    QString fullName = m_fullNameEdit->text().trimmed();

    if (username.isEmpty() || fullName.isEmpty()) {
        QMessageBox::warning(this, isRtl ? "خطأ في الإدخال" : "Validation Error",
            isRtl ? "يُرجى إدخال اسم المستخدم والاسم الكامل للموظف." : "Username and full name are required.");
        return;
    }

    std::vector<std::string> roles;
    if (m_roleSuperAdmin->isChecked()) roles.push_back("superadmin");
    if (m_roleAdmin->isChecked()) roles.push_back("admin");
    if (m_roleSecretary->isChecked()) roles.push_back("secretary");
    if (m_roleAccountant->isChecked()) roles.push_back("accountant");
    if (m_roleInstructor->isChecked()) roles.push_back("instructor");

    if (roles.empty()) {
        QMessageBox::warning(this, isRtl ? "خطأ في الصلاحيات" : "Validation Error",
            isRtl ? "يجب تعيين دور وظيفي واحد على الأقل للموظف." : "Please select at least one role.");
        return;
    }

    if (!m_userId.has_value()) {
        // Create new staff
        QString pass = m_passwordEdit->text();
        if (pass.length() < 8) {
            QMessageBox::warning(this, isRtl ? "خطأ في كلمة المرور" : "Validation Error",
                isRtl ? "كلمة المرور يجب ألا تقل عن 8 أحرف." : "Password must be at least 8 characters.");
            return;
        }

        application::CreateStaffCommand cmd;
        cmd.username = username.toStdString();
        cmd.password = pass.toStdString();
        cmd.fullName = fullName.toStdString();
        cmd.phone = m_phoneEdit->text().trimmed().toStdString();
        cmd.roleCodes = roles;
        cmd.preferredLanguage = isRtl ? "ar" : "en";
        cmd.specialization = m_specEdit->text().trimmed().toStdString();
        cmd.hourlyRatePiastres = static_cast<std::int64_t>(m_hourlyRateSpin->value() * 100.0 + 0.5);

        auto res = m_appContext->staffUseCases()->createStaff(cmd);
        if (!res.isOk()) {
            QMessageBox::critical(this, isRtl ? "فشل الإنشاء" : "Error",
                QString::fromStdString(res.error().message()));
            return;
        }
    } else {
        // Update staff
        application::UpdateStaffCommand cmd;
        cmd.userId = *m_userId;
        cmd.fullName = fullName.toStdString();
        cmd.phone = m_phoneEdit->text().trimmed().toStdString();
        cmd.roleCodes = roles;
        cmd.isActive = m_activeCheck->isChecked();
        cmd.specialization = m_specEdit->text().trimmed().toStdString();
        cmd.hourlyRatePiastres = static_cast<std::int64_t>(m_hourlyRateSpin->value() * 100.0 + 0.5);

        auto res = m_appContext->staffUseCases()->updateStaff(cmd);
        if (!res.isOk()) {
            QMessageBox::critical(this, isRtl ? "فشل التعديل" : "Error",
                QString::fromStdString(res.error().message()));
            return;
        }
    }

    emit staffSaved();
    accept();
}

} // namespace pixcode::desktop
