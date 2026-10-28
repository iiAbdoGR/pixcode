#include "StudentFormDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

StudentFormDialog::StudentFormDialog(std::shared_ptr<ApplicationContext> appContext,
                                     std::optional<std::int64_t> studentId,
                                     QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_studentId(studentId) {

    setupUi();
    if (m_studentId.has_value()) {
        loadStudentData();
    }
}

void StudentFormDialog::setupUi() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setWindowTitle(m_studentId.has_value() 
        ? (isRtl ? "تعديل بيانات الطالب" : "Edit Student") 
        : (isRtl ? "إضافة طالب جديد" : "Add New Student"));
    setMinimumWidth(450);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(isRtl ? "الاسم الكامل للطالب *" : "Student Full Name *");

    m_phoneEdit = new QLineEdit(this);
    m_phoneEdit->setPlaceholderText(isRtl ? "رقم الهاتف" : "Phone Number");

    m_guardianNameEdit = new QLineEdit(this);
    m_guardianNameEdit->setPlaceholderText(isRtl ? "اسم ولي الأمر" : "Guardian Name");

    m_guardianPhoneEdit = new QLineEdit(this);
    m_guardianPhoneEdit->setPlaceholderText(isRtl ? "هاتف ولي الأمر" : "Guardian Phone");

    m_addressEdit = new QLineEdit(this);
    m_addressEdit->setPlaceholderText(isRtl ? "العنوان" : "Address");

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(80);
    m_notesEdit->setPlaceholderText(isRtl ? "ملاحظات إضافية" : "Additional Notes");

    formLayout->addRow(isRtl ? "الاسم الكامل *:" : "Full Name *:", m_nameEdit);
    formLayout->addRow(isRtl ? "رقم الهاتف:" : "Phone:", m_phoneEdit);
    formLayout->addRow(isRtl ? "ولي الأمر:" : "Guardian Name:", m_guardianNameEdit);
    formLayout->addRow(isRtl ? "هاتف ولي الأمر:" : "Guardian Phone:", m_guardianPhoneEdit);
    formLayout->addRow(isRtl ? "العنوان:" : "Address:", m_addressEdit);
    formLayout->addRow(isRtl ? "ملاحظات:" : "Notes:", m_notesEdit);

    mainLayout->addLayout(formLayout);

    auto* buttonsLayout = new QHBoxLayout();
    m_cancelButton = new QPushButton(isRtl ? "إلغاء" : "Cancel", this);
    m_cancelButton->setObjectName("secondaryButton");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    m_saveButton = new QPushButton(isRtl ? "حفظ" : "Save", this);
    m_saveButton->setObjectName("primaryButton");
    connect(m_saveButton, &QPushButton::clicked, this, &StudentFormDialog::onSaveClicked);

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_cancelButton);
    buttonsLayout->addWidget(m_saveButton);

    mainLayout->addLayout(buttonsLayout);
}

void StudentFormDialog::loadStudentData() {
    auto res = m_appContext->studentUseCases()->getStudent(*m_studentId);
    if (res.isOk()) {
        const auto& s = res.value();
        m_nameEdit->setText(QString::fromStdString(s.fullName));
        m_phoneEdit->setText(QString::fromStdString(s.phone));
        m_guardianNameEdit->setText(QString::fromStdString(s.guardianName));
        m_guardianPhoneEdit->setText(QString::fromStdString(s.guardianPhone));
        m_addressEdit->setText(QString::fromStdString(s.address));
        m_notesEdit->setText(QString::fromStdString(s.notes));
    }
}

void StudentFormDialog::onSaveClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Validation Error",
                             isRtl ? "يرجى إدخال اسم الطالب الكامل." : "Student full name is required.");
        m_nameEdit->setFocus();
        return;
    }

    if (m_studentId.has_value()) {
        application::UpdateStudentCommand cmd{
            .id = *m_studentId,
            .fullName = name.toStdString(),
            .phone = m_phoneEdit->text().trimmed().toStdString(),
            .guardianName = m_guardianNameEdit->text().trimmed().toStdString(),
            .guardianPhone = m_guardianPhoneEdit->text().trimmed().toStdString(),
            .address = m_addressEdit->text().trimmed().toStdString(),
            .notes = m_notesEdit->toPlainText().trimmed().toStdString()
        };
        auto res = m_appContext->studentUseCases()->updateStudent(cmd);
        if (res.isOk()) {
            emit studentSaved(res.value().id);
            accept();
        } else {
            QMessageBox::critical(this, isRtl ? "خطأ" : "Error",
                                  QString::fromStdString(res.error().message()));
        }
    } else {
        application::CreateStudentCommand cmd{
            .fullName = name.toStdString(),
            .phone = m_phoneEdit->text().trimmed().toStdString(),
            .guardianName = m_guardianNameEdit->text().trimmed().toStdString(),
            .guardianPhone = m_guardianPhoneEdit->text().trimmed().toStdString(),
            .address = m_addressEdit->text().trimmed().toStdString(),
            .notes = m_notesEdit->toPlainText().trimmed().toStdString()
        };
        auto res = m_appContext->studentUseCases()->createStudent(cmd);
        if (res.isOk()) {
            emit studentSaved(res.value().id);
            accept();
        } else {
            QMessageBox::critical(this, isRtl ? "خطأ" : "Error",
                                  QString::fromStdString(res.error().message()));
        }
    }
}

} // namespace pixcode::desktop
