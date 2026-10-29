#include "InstructorFormDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QLabel>
#include <QtGui/QDoubleValidator>
#include "../localization/LocalizationManager.h"
#include "../styles/Theme.h"

namespace pixcode::desktop {

InstructorFormDialog::InstructorFormDialog(std::shared_ptr<ApplicationContext> appContext,
                                           std::optional<std::int64_t> instructorId,
                                           QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_instructorId(instructorId) {
    setupUi();
    if (m_instructorId.has_value()) {
        loadData();
    }
}

void InstructorFormDialog::setupUi() {
    bool isEdit = m_instructorId.has_value();
    setWindowTitle(isEdit
        ? LocalizationManager::instance().get("dialog.instructor.edit_title", "Edit Instructor")
        : LocalizationManager::instance().get("dialog.instructor.create_title", "Add Instructor"));
    setMinimumWidth(440);
    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }")
        .arg(Theme::Background)
        .arg(Theme::Primary));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(16);
    rootLayout->setContentsMargins(24, 24, 24, 24);

    auto* titleLabel = new QLabel(isEdit
        ? LocalizationManager::instance().get("dialog.instructor.edit_title", "Edit Instructor")
        : LocalizationManager::instance().get("dialog.instructor.create_title", "Add Instructor"), this);
    QFont f = titleLabel->font();
    f.setBold(true);
    f.setPointSize(12);
    titleLabel->setFont(f);
    titleLabel->setStyleSheet(QString("color: %1;").arg(Theme::Primary));
    rootLayout->addWidget(titleLabel);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    const QString inputStyle = QString("QLineEdit, QComboBox, QTextEdit { background-color: white; border: 1px solid %1; border-radius: 4px; padding: 6px; }")
        .arg(Theme::Border);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setStyleSheet(inputStyle);
    formLayout->addRow(LocalizationManager::instance().get("dialog.instructor.name", "Full Name:"), m_nameEdit);

    m_phoneEdit = new QLineEdit(this);
    m_phoneEdit->setStyleSheet(inputStyle);
    formLayout->addRow(LocalizationManager::instance().get("dialog.instructor.phone", "Phone Number:"), m_phoneEdit);

    m_specEdit = new QLineEdit(this);
    m_specEdit->setStyleSheet(inputStyle);
    formLayout->addRow(LocalizationManager::instance().get("dialog.instructor.spec", "Specialization:"), m_specEdit);

    m_compTypeCombo = new QComboBox(this);
    m_compTypeCombo->setStyleSheet(inputStyle);
    m_compTypeCombo->addItem(LocalizationManager::instance().get("instructor.comp.hourly", "Hourly Rate"), "hourly");
    m_compTypeCombo->addItem(LocalizationManager::instance().get("instructor.comp.per_session", "Per Session"), "per_session");
    m_compTypeCombo->addItem(LocalizationManager::instance().get("instructor.comp.monthly_salary", "Monthly Salary"), "monthly_salary");
    m_compTypeCombo->addItem(LocalizationManager::instance().get("instructor.comp.fixed_contract", "Fixed Contract"), "fixed_contract");
    formLayout->addRow(LocalizationManager::instance().get("dialog.instructor.comp_type", "Compensation Mode:"), m_compTypeCombo);

    m_baseRateEdit = new QLineEdit(this);
    m_baseRateEdit->setStyleSheet(inputStyle);
    m_baseRateEdit->setValidator(new QDoubleValidator(0.0, 1000000.0, 2, this));
    m_baseRateEdit->setPlaceholderText("0.00");
    formLayout->addRow(LocalizationManager::instance().get("dialog.instructor.base_rate", "Rate / Session / Hourly (EGP):"), m_baseRateEdit);

    m_monthlySalaryEdit = new QLineEdit(this);
    m_monthlySalaryEdit->setStyleSheet(inputStyle);
    m_monthlySalaryEdit->setValidator(new QDoubleValidator(0.0, 1000000.0, 2, this));
    m_monthlySalaryEdit->setPlaceholderText("0.00");
    formLayout->addRow(LocalizationManager::instance().get("dialog.instructor.monthly_salary", "Monthly Salary (EGP):"), m_monthlySalaryEdit);

    m_startDateEdit = new QLineEdit(this);
    m_startDateEdit->setStyleSheet(inputStyle);
    m_startDateEdit->setPlaceholderText("YYYY-MM-DD");
    formLayout->addRow(LocalizationManager::instance().get("dialog.instructor.contract_start", "Contract Start Date:"), m_startDateEdit);

    m_endDateEdit = new QLineEdit(this);
    m_endDateEdit->setStyleSheet(inputStyle);
    m_endDateEdit->setPlaceholderText("YYYY-MM-DD");
    formLayout->addRow(LocalizationManager::instance().get("dialog.instructor.contract_end", "Contract End Date:"), m_endDateEdit);

    m_statusCombo = new QComboBox(this);
    m_statusCombo->setStyleSheet(inputStyle);
    m_statusCombo->addItem(LocalizationManager::instance().get("common.status_active", "Active"), "active");
    m_statusCombo->addItem(LocalizationManager::instance().get("common.status_inactive", "Inactive"), "inactive");
    formLayout->addRow(LocalizationManager::instance().get("dialog.instructor.status", "Status:"), m_statusCombo);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setStyleSheet(inputStyle);
    m_notesEdit->setMaximumHeight(70);
    formLayout->addRow(LocalizationManager::instance().get("dialog.instructor.notes", "Notes:"), m_notesEdit);

    rootLayout->addLayout(formLayout);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_cancelButton = new QPushButton(LocalizationManager::instance().get("common.cancel", "Cancel"), this);
    m_cancelButton->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid %2; padding: 8px 16px; border-radius: 4px; }")
        .arg(Theme::Surface)
        .arg(Theme::Border));
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelButton);

    m_saveButton = new QPushButton(LocalizationManager::instance().get("common.save", "Save"), this);
    m_saveButton->setStyleSheet(QString("QPushButton { background-color: %1; color: white; font-weight: bold; padding: 8px 16px; border-radius: 4px; border: none; } QPushButton:hover { background-color: %2; }")
        .arg(Theme::Secondary)
        .arg(Theme::Primary));
    connect(m_saveButton, &QPushButton::clicked, this, &InstructorFormDialog::onSaveClicked);
    btnLayout->addWidget(m_saveButton);

    rootLayout->addLayout(btnLayout);
}

void InstructorFormDialog::loadData() {
    auto res = m_appContext->payrollUseCases()->getInstructor(*m_instructorId);
    if (!res.isOk()) {
        QMessageBox::critical(this, LocalizationManager::instance().get("common.error", "Error"),
                              QString::fromStdString(res.error().message()));
        reject();
        return;
    }

    const auto& inst = res.value();
    m_nameEdit->setText(QString::fromStdString(inst.fullName));
    m_phoneEdit->setText(QString::fromStdString(inst.phone));
    m_specEdit->setText(QString::fromStdString(inst.specialization));

    int compIdx = m_compTypeCombo->findData(QString::fromStdString(inst.compensationType));
    if (compIdx >= 0) m_compTypeCombo->setCurrentIndex(compIdx);

    m_baseRateEdit->setText(QString::number(inst.baseRatePiastres / 100.0, 'f', 2));
    m_monthlySalaryEdit->setText(QString::number(inst.monthlySalaryPiastres / 100.0, 'f', 2));
    m_startDateEdit->setText(QString::fromStdString(inst.contractStartDate));
    m_endDateEdit->setText(QString::fromStdString(inst.contractEndDate));

    int stIdx = m_statusCombo->findData(QString::fromStdString(inst.status));
    if (stIdx >= 0) m_statusCombo->setCurrentIndex(stIdx);

    m_notesEdit->setPlainText(QString::fromStdString(inst.notes));
}

void InstructorFormDialog::onSaveClicked() {
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, LocalizationManager::instance().get("common.warning", "Warning"),
                             LocalizationManager::instance().get("dialog.instructor.name_req", "Instructor name is required."));
        return;
    }

    double rateVal = m_baseRateEdit->text().toDouble();
    std::int64_t ratePiastres = static_cast<std::int64_t>(std::round(rateVal * 100.0));
    double salaryVal = m_monthlySalaryEdit->text().toDouble();
    std::int64_t salaryPiastres = static_cast<std::int64_t>(std::round(salaryVal * 100.0));
    std::string compType = m_compTypeCombo->currentData().toString().toStdString();
    std::string startDate = m_startDateEdit->text().trimmed().toStdString();
    std::string endDate = m_endDateEdit->text().trimmed().toStdString();
    std::string status = m_statusCombo->currentData().toString().toStdString();

    try {
        if (m_instructorId.has_value()) {
            application::UpdateInstructorCommand cmd{
                .id = *m_instructorId,
                .fullName = name.toStdString(),
                .phone = m_phoneEdit->text().trimmed().toStdString(),
                .specialization = m_specEdit->text().trimmed().toStdString(),
                .compensationType = compType,
                .baseRatePiastres = ratePiastres,
                .monthlySalaryPiastres = salaryPiastres,
                .contractStartDate = startDate,
                .contractEndDate = endDate,
                .notes = m_notesEdit->toPlainText().trimmed().toStdString(),
                .status = status
            };
            auto res = m_appContext->payrollUseCases()->updateInstructor(cmd);
            if (!res.isOk()) {
                QMessageBox::critical(this, LocalizationManager::instance().get("common.error", "Error"),
                                      QString::fromStdString(res.error().message()));
                return;
            }
            emit instructorSaved(res.value());
        } else {
            application::CreateInstructorCommand cmd{
                .fullName = name.toStdString(),
                .phone = m_phoneEdit->text().trimmed().toStdString(),
                .specialization = m_specEdit->text().trimmed().toStdString(),
                .compensationType = compType,
                .baseRatePiastres = ratePiastres,
                .monthlySalaryPiastres = salaryPiastres,
                .contractStartDate = startDate,
                .contractEndDate = endDate,
                .notes = m_notesEdit->toPlainText().trimmed().toStdString()
            };
            auto res = m_appContext->payrollUseCases()->createInstructor(cmd);
            if (!res.isOk()) {
                QMessageBox::critical(this, LocalizationManager::instance().get("common.error", "Error"),
                                      QString::fromStdString(res.error().message()));
                return;
            }
            emit instructorSaved(res.value());
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, LocalizationManager::instance().get("common.error", "Error"),
                              QString::fromStdString(e.what()));
        return;
    }

    accept();
}

} // namespace pixcode::desktop

