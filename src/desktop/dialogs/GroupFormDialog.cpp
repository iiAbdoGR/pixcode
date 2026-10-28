#include "GroupFormDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"
#include "domain/value_objects/Money.h"

namespace pixcode::desktop {

GroupFormDialog::GroupFormDialog(std::shared_ptr<ApplicationContext> appContext,
                                 std::optional<std::int64_t> groupId,
                                 QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_groupId(groupId) {

    setupUi();
    loadData();
}

void GroupFormDialog::setupUi() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setWindowTitle(m_groupId.has_value() 
        ? (isRtl ? "تعديل المجموعة" : "Edit Group") 
        : (isRtl ? "إنشاء مجموعة دراسية جديدة" : "Create New Class Group"));
    setMinimumWidth(480);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    m_courseCombo = new QComboBox(this);
    connect(m_courseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GroupFormDialog::onCourseChanged);

    m_instructorCombo = new QComboBox(this);
    m_instructorCombo->addItem(isRtl ? "بدون مدرب حالياً" : "None / Unassigned", QVariant::fromValue(static_cast<qint64>(0)));

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(isRtl ? "اسم المجموعة (مثال: مجموعة أ - شتاء 2026) *" : "Group Name *");

    m_scheduleEdit = new QLineEdit(this);
    m_scheduleEdit->setPlaceholderText(isRtl ? "الأحد والثلاثاء 5:00 - 7:00 م" : "Sun/Tue 5:00 PM - 7:00 PM");

    m_startDateEdit = new QLineEdit(this);
    m_startDateEdit->setPlaceholderText("YYYY-MM-DD");

    m_endDateEdit = new QLineEdit(this);
    m_endDateEdit->setPlaceholderText("YYYY-MM-DD");

    m_capacitySpinBox = new QSpinBox(this);
    m_capacitySpinBox->setRange(0, 500);
    m_capacitySpinBox->setValue(15);
    m_capacitySpinBox->setSpecialValueText(isRtl ? "غير محدود" : "Unlimited");

    m_feeEdit = new QLineEdit("0", this);

    m_statusCombo = new QComboBox(this);
    m_statusCombo->addItem(isRtl ? "مفتوحة للتسجيل (Open)" : "Open", "open");
    m_statusCombo->addItem(isRtl ? "مكتملة العدد (Full)" : "Full", "full");
    m_statusCombo->addItem(isRtl ? "مغلقة / منتهية (Closed)" : "Closed", "closed");
    m_statusCombo->addItem(isRtl ? "ملغاة (Cancelled)" : "Cancelled", "cancelled");

    formLayout->addRow(isRtl ? "الكورس التابع له *:" : "Course Template *:", m_courseCombo);
    formLayout->addRow(isRtl ? "المدرب المسؤول:" : "Instructor:", m_instructorCombo);
    formLayout->addRow(isRtl ? "اسم المجموعة *:" : "Group Name *:", m_nameEdit);
    formLayout->addRow(isRtl ? "مواعيد المحاضرات:" : "Schedule:", m_scheduleEdit);
    formLayout->addRow(isRtl ? "تاريخ البدء:" : "Start Date:", m_startDateEdit);
    formLayout->addRow(isRtl ? "تاريخ الانتهاء:" : "End Date:", m_endDateEdit);
    formLayout->addRow(isRtl ? "السعة القصوى للطلاب:" : "Capacity:", m_capacitySpinBox);
    formLayout->addRow(isRtl ? "رسوم المجموعة (ج.م):" : "Group Fee (EGP):", m_feeEdit);
    formLayout->addRow(isRtl ? "حالة المجموعة:" : "Status:", m_statusCombo);

    mainLayout->addLayout(formLayout);

    auto* buttonsLayout = new QHBoxLayout();
    m_cancelButton = new QPushButton(isRtl ? "إلغاء" : "Cancel", this);
    m_cancelButton->setObjectName("secondaryButton");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    m_saveButton = new QPushButton(isRtl ? "حفظ المجموعة" : "Save Group", this);
    m_saveButton->setObjectName("primaryButton");
    connect(m_saveButton, &QPushButton::clicked, this, &GroupFormDialog::onSaveClicked);

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_cancelButton);
    buttonsLayout->addWidget(m_saveButton);

    mainLayout->addLayout(buttonsLayout);
}

void GroupFormDialog::loadData() {
    // 1. Load active courses
    application::CourseFilter cFilter;
    cFilter.status = domain::CourseStatus::Active;
    cFilter.pageSize = 200;
    auto cRes = m_appContext->courseUseCases()->listCourses(cFilter);
    if (cRes.isOk()) {
        for (const auto& c : cRes.value()) {
            domain::Money defaultFee(c.defaultFeePiastres);
            QString label = QString("%1 (%2)").arg(QString::fromStdString(c.name), QString::fromStdString(defaultFee.format()));
            m_courseCombo->addItem(label, QVariant::fromValue(static_cast<qint64>(c.id)));
        }
    }

    // 2. Load instructors
    application::InstructorFilter iFilter;
    auto instructors = m_appContext->instructorRepository()->findAll(iFilter);
    for (const auto& inst : instructors) {
        m_instructorCombo->addItem(QString::fromStdString(inst.fullName), QVariant::fromValue(static_cast<qint64>(inst.id)));
    }

    // If editing existing group:
    if (m_groupId.has_value()) {
        auto gRes = m_appContext->groupUseCases()->getGroup(*m_groupId);
        if (gRes.isOk()) {
            const auto& g = gRes.value();
            int cIdx = m_courseCombo->findData(QVariant::fromValue(static_cast<qint64>(g.courseId)));
            if (cIdx >= 0) m_courseCombo->setCurrentIndex(cIdx);

            if (g.instructorId.has_value()) {
                int iIdx = m_instructorCombo->findData(QVariant::fromValue(static_cast<qint64>(*g.instructorId)));
                if (iIdx >= 0) m_instructorCombo->setCurrentIndex(iIdx);
            }

            m_nameEdit->setText(QString::fromStdString(g.name));
            m_scheduleEdit->setText(QString::fromStdString(g.scheduleDescription));
            m_startDateEdit->setText(QString::fromStdString(g.startDate));
            m_endDateEdit->setText(QString::fromStdString(g.endDate));
            m_capacitySpinBox->setValue(g.capacity);

            domain::Money fee(g.feePiastres);
            m_feeEdit->setText(QString::number(fee.wholeEgp()));

            int sIdx = m_statusCombo->findData(QString::fromStdString(g.status));
            if (sIdx >= 0) m_statusCombo->setCurrentIndex(sIdx);
        }
    } else {
        if (m_courseCombo->count() > 0) {
            onCourseChanged(0);
        }
    }
}

void GroupFormDialog::onCourseChanged(int index) {
    if (index < 0 || m_groupId.has_value()) return;
    std::int64_t courseId = m_courseCombo->itemData(index).toLongLong();
    auto res = m_appContext->courseUseCases()->getCourse(courseId);
    if (res.isOk()) {
        domain::Money defaultFee(res.value().defaultFeePiastres);
        m_feeEdit->setText(QString::number(defaultFee.wholeEgp()));
    }
}

void GroupFormDialog::onSaveClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Validation Error",
                             isRtl ? "يرجى إدخال اسم المجموعة." : "Group name is required.");
        m_nameEdit->setFocus();
        return;
    }
    if (m_courseCombo->currentIndex() < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Validation Error",
                             isRtl ? "يرجى اختيار الكورس التابع له." : "Course is required.");
        return;
    }

    std::int64_t courseId = m_courseCombo->currentData().toLongLong();

    std::optional<std::int64_t> instructorId;
    std::int64_t iVal = m_instructorCombo->currentData().toLongLong();
    if (iVal > 0) instructorId = iVal;

    double feeEgp = m_feeEdit->text().trimmed().toDouble();
    if (feeEgp < 0) feeEgp = 0;
    std::int64_t feePiastres = static_cast<std::int64_t>(feeEgp * 100.0 + 0.5);

    if (m_groupId.has_value()) {
        application::UpdateGroupCommand cmd{
            .id = *m_groupId,
            .instructorId = instructorId,
            .name = name.toStdString(),
            .startDate = m_startDateEdit->text().trimmed().toStdString(),
            .endDate = m_endDateEdit->text().trimmed().toStdString(),
            .scheduleDescription = m_scheduleEdit->text().trimmed().toStdString(),
            .capacity = m_capacitySpinBox->value(),
            .feePiastres = feePiastres,
            .status = m_statusCombo->currentData().toString().toStdString()
        };
        auto res = m_appContext->groupUseCases()->updateGroup(cmd);
        if (res.isOk()) {
            emit groupSaved(res.value().id);
            accept();
        } else {
            QMessageBox::critical(this, isRtl ? "خطأ" : "Error",
                                  QString::fromStdString(res.error().message()));
        }
    } else {
        application::CreateGroupCommand cmd{
            .courseId = courseId,
            .instructorId = instructorId,
            .name = name.toStdString(),
            .startDate = m_startDateEdit->text().trimmed().toStdString(),
            .endDate = m_endDateEdit->text().trimmed().toStdString(),
            .scheduleDescription = m_scheduleEdit->text().trimmed().toStdString(),
            .capacity = m_capacitySpinBox->value(),
            .feePiastres = feePiastres
        };
        auto res = m_appContext->groupUseCases()->createGroup(cmd);
        if (res.isOk()) {
            emit groupSaved(res.value().id);
            accept();
        } else {
            QMessageBox::critical(this, isRtl ? "خطأ" : "Error",
                                  QString::fromStdString(res.error().message()));
        }
    }
}

} // namespace pixcode::desktop

