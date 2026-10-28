#include "CourseFormDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"
#include "domain/value_objects/Money.h"

namespace pixcode::desktop {

CourseFormDialog::CourseFormDialog(std::shared_ptr<ApplicationContext> appContext,
                                   std::optional<std::int64_t> courseId,
                                   QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_courseId(courseId) {

    setupUi();
    if (m_courseId.has_value()) {
        loadCourseData();
    }
}

void CourseFormDialog::setupUi() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setWindowTitle(m_courseId.has_value() 
        ? (isRtl ? "تعديل الكورس" : "Edit Course") 
        : (isRtl ? "إضافة كورس جديد" : "Add New Course"));
    setMinimumWidth(450);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(isRtl ? "اسم الكورس *" : "Course Name *");

    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setMaximumHeight(80);
    m_descriptionEdit->setPlaceholderText(isRtl ? "وصف محتوى الكورس" : "Course Description");

    m_durationSpinBox = new QSpinBox(this);
    m_durationSpinBox->setRange(1, 100);
    m_durationSpinBox->setValue(8);
    m_durationSpinBox->setSuffix(isRtl ? " أسابيع" : " weeks");

    m_feeEdit = new QLineEdit("0", this);
    m_feeEdit->setPlaceholderText(isRtl ? "السعر الافتراضي بالجنيه" : "Default Price (EGP)");

    formLayout->addRow(isRtl ? "اسم الكورس *:" : "Course Name *:", m_nameEdit);
    formLayout->addRow(isRtl ? "الوصف:" : "Description:", m_descriptionEdit);
    formLayout->addRow(isRtl ? "المدة المقترحة:" : "Duration:", m_durationSpinBox);
    formLayout->addRow(isRtl ? "السعر الافتراضي (ج.م):" : "Default Price (EGP):", m_feeEdit);

    mainLayout->addLayout(formLayout);

    auto* buttonsLayout = new QHBoxLayout();
    m_cancelButton = new QPushButton(isRtl ? "إلغاء" : "Cancel", this);
    m_cancelButton->setObjectName("secondaryButton");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    m_saveButton = new QPushButton(isRtl ? "حفظ" : "Save", this);
    m_saveButton->setObjectName("primaryButton");
    connect(m_saveButton, &QPushButton::clicked, this, &CourseFormDialog::onSaveClicked);

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_cancelButton);
    buttonsLayout->addWidget(m_saveButton);

    mainLayout->addLayout(buttonsLayout);
}

void CourseFormDialog::loadCourseData() {
    auto res = m_appContext->courseUseCases()->getCourse(*m_courseId);
    if (res.isOk()) {
        const auto& c = res.value();
        m_nameEdit->setText(QString::fromStdString(c.name));
        m_descriptionEdit->setText(QString::fromStdString(c.description));
        m_durationSpinBox->setValue(c.durationWeeks);
        domain::Money fee(c.defaultFeePiastres);
        m_feeEdit->setText(QString::number(fee.wholeEgp()));
    }
}

void CourseFormDialog::onSaveClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Validation Error",
                             isRtl ? "يرجى إدخال اسم الكورس." : "Course name is required.");
        m_nameEdit->setFocus();
        return;
    }

    double feeEgp = m_feeEdit->text().trimmed().toDouble();
    if (feeEgp < 0) feeEgp = 0;
    std::int64_t feePiastres = static_cast<std::int64_t>(feeEgp * 100.0 + 0.5);

    if (m_courseId.has_value()) {
        application::UpdateCourseCommand cmd{
            .id = *m_courseId,
            .name = name.toStdString(),
            .description = m_descriptionEdit->toPlainText().trimmed().toStdString(),
            .durationWeeks = m_durationSpinBox->value(),
            .defaultFeePiastres = feePiastres,
            .status = "active"
        };
        auto res = m_appContext->courseUseCases()->updateCourse(cmd);
        if (res.isOk()) {
            emit courseSaved(res.value().id);
            accept();
        } else {
            QMessageBox::critical(this, isRtl ? "خطأ" : "Error",
                                  QString::fromStdString(res.error().message()));
        }
    } else {
        application::CreateCourseCommand cmd{
            .name = name.toStdString(),
            .description = m_descriptionEdit->toPlainText().trimmed().toStdString(),
            .durationWeeks = m_durationSpinBox->value(),
            .defaultFeePiastres = feePiastres
        };
        auto res = m_appContext->courseUseCases()->createCourse(cmd);
        if (res.isOk()) {
            emit courseSaved(res.value().id);
            accept();
        } else {
            QMessageBox::critical(this, isRtl ? "خطأ" : "Error",
                                  QString::fromStdString(res.error().message()));
        }
    }
}

} // namespace pixcode::desktop
