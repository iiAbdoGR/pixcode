#include "EnrollmentDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"
#include "domain/value_objects/Money.h"

namespace pixcode::desktop {

EnrollmentDialog::EnrollmentDialog(std::shared_ptr<ApplicationContext> appContext,
                                   std::optional<std::int64_t> studentId,
                                   std::optional<std::int64_t> groupId,
                                   QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_preselectedStudentId(studentId),
      m_preselectedGroupId(groupId) {

    setupUi();
    loadData();
}

void EnrollmentDialog::setupUi() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setWindowTitle(isRtl ? "تسجيل طالب في مجموعة" : "Enroll Student in Group");
    setMinimumWidth(500);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    m_studentCombo = new QComboBox(this);
    m_studentCombo->setEditable(true);

    m_groupCombo = new QComboBox(this);

    m_groupDetailsLabel = new QLabel(this);
    m_groupDetailsLabel->setStyleSheet("color: #52606D; font-size: 13px;");

    m_feeLabel = new QLabel("0.00 EGP", this);
    m_feeLabel->setStyleSheet("font-weight: 600;");

    m_discountEdit = new QLineEdit("0", this);

    m_netFeeLabel = new QLabel("0.00 EGP", this);
    m_netFeeLabel->setStyleSheet("font-weight: bold; font-size: 15px; color: #2E3A44;");

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(70);

    formLayout->addRow(isRtl ? "الطالب *:" : "Student *:", m_studentCombo);
    formLayout->addRow(isRtl ? "المجموعة الدراسية *:" : "Course Group *:", m_groupCombo);
    formLayout->addRow("", m_groupDetailsLabel);
    formLayout->addRow(isRtl ? "رسوم المجموعة الأصلية:" : "Original Fee:", m_feeLabel);
    formLayout->addRow(isRtl ? "الخصم المعتمد (ج.م):" : "Authorized Discount (EGP):", m_discountEdit);
    formLayout->addRow(isRtl ? "صافي الرسوم المستحقة:" : "Net Fee Required:", m_netFeeLabel);
    formLayout->addRow(isRtl ? "ملاحظات التسجيل:" : "Enrollment Notes:", m_notesEdit);

    mainLayout->addLayout(formLayout);

    connect(m_groupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EnrollmentDialog::onGroupSelectionChanged);
    connect(m_discountEdit, &QLineEdit::textChanged,
            this, &EnrollmentDialog::onDiscountChanged);

    auto* buttonsLayout = new QHBoxLayout();
    m_cancelButton = new QPushButton(isRtl ? "إلغاء" : "Cancel", this);
    m_cancelButton->setObjectName("secondaryButton");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    m_enrollButton = new QPushButton(isRtl ? "تأكيد التسجيل" : "Confirm Enrollment", this);
    m_enrollButton->setObjectName("primaryButton");
    connect(m_enrollButton, &QPushButton::clicked, this, &EnrollmentDialog::onEnrollClicked);

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(m_cancelButton);
    buttonsLayout->addWidget(m_enrollButton);

    mainLayout->addLayout(buttonsLayout);
}

void EnrollmentDialog::loadData() {
    bool isRtl = LocalizationManager::instance().isRtl();

    // 1. Load active students
    application::StudentFilter sFilter;
    sFilter.status = domain::StudentStatus::Active;
    sFilter.pageSize = 500;
    auto sRes = m_appContext->studentUseCases()->listStudents(sFilter);
    if (sRes.isOk()) {
        for (const auto& s : sRes.value()) {
            QString label = QString("%1 (%2) - %3")
                .arg(QString::fromStdString(s.fullName))
                .arg(QString::fromStdString(s.studentCode))
                .arg(QString::fromStdString(s.phone));
            m_studentCombo->addItem(label, QVariant::fromValue(static_cast<qint64>(s.id)));
        }
    }

    if (m_preselectedStudentId.has_value()) {
        int idx = m_studentCombo->findData(QVariant::fromValue(static_cast<qint64>(*m_preselectedStudentId)));
        if (idx >= 0) {
            m_studentCombo->setCurrentIndex(idx);
            m_studentCombo->setEnabled(false);
        }
    }

    // 2. Load open groups
    application::GroupFilter gFilter;
    gFilter.status = domain::GroupStatus::Open;
    gFilter.pageSize = 200;
    auto gRes = m_appContext->groupUseCases()->listGroups(gFilter);
    if (gRes.isOk()) {
        for (const auto& g : gRes.value()) {
            QString capInfo = (g.capacity > 0)
                ? QString("%1/%2").arg(g.enrolledCount).arg(g.capacity)
                : QString::number(g.enrolledCount);
            QString label = QString("%1 [%2] (%3: %4)")
                .arg(QString::fromStdString(g.name))
                .arg(QString::fromStdString(g.courseName))
                .arg(isRtl ? "المسجلين" : "Enrolled")
                .arg(capInfo);
            m_groupCombo->addItem(label, QVariant::fromValue(static_cast<qint64>(g.id)));
        }
    }

    if (m_preselectedGroupId.has_value()) {
        int idx = m_groupCombo->findData(QVariant::fromValue(static_cast<qint64>(*m_preselectedGroupId)));
        if (idx >= 0) {
            m_groupCombo->setCurrentIndex(idx);
        }
    }

    if (m_groupCombo->count() > 0) {
        onGroupSelectionChanged(m_groupCombo->currentIndex());
    }
}

void EnrollmentDialog::onGroupSelectionChanged(int index) {
    if (index < 0) return;
    bool isRtl = LocalizationManager::instance().isRtl();
    std::int64_t gid = m_groupCombo->itemData(index).toLongLong();

    auto res = m_appContext->groupUseCases()->getGroup(gid);
    if (res.isOk()) {
        const auto& g = res.value();
        m_currentGroupFeePiastres = g.feePiastres;

        QString schedule = QString::fromStdString(g.scheduleDescription);
        QString cap = (g.capacity > 0) ? QString("%1 / %2").arg(g.enrolledCount).arg(g.capacity) : "Unlimited";
        m_groupDetailsLabel->setText(QString("%1: %2 | %3: %4")
            .arg(isRtl ? "المواعيد" : "Schedule", schedule.isEmpty() ? "-" : schedule)
            .arg(isRtl ? "الأماكن" : "Capacity", cap));

        domain::Money originalFee(g.feePiastres);
        m_feeLabel->setText(QString::fromStdString(originalFee.format()));

        updateFinancials();
    }
}

void EnrollmentDialog::onDiscountChanged(const QString& /*text*/) {
    updateFinancials();
}

void EnrollmentDialog::updateFinancials() {
    double discountEgp = m_discountEdit->text().trimmed().toDouble();
    if (discountEgp < 0) discountEgp = 0;
    std::int64_t discountPiastres = static_cast<std::int64_t>(discountEgp * 100.0 + 0.5);

    if (discountPiastres > m_currentGroupFeePiastres) {
        discountPiastres = m_currentGroupFeePiastres;
    }

    std::int64_t netPiastres = m_currentGroupFeePiastres - discountPiastres;
    domain::Money netMoney(netPiastres);
    m_netFeeLabel->setText(QString::fromStdString(netMoney.format()));
}

void EnrollmentDialog::onEnrollClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();

    if (m_studentCombo->currentIndex() < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Validation Error",
                             isRtl ? "يرجى اختيار طالب للتسجيل." : "Please select a student.");
        return;
    }
    if (m_groupCombo->currentIndex() < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Validation Error",
                             isRtl ? "يرجى اختيار مجموعة دراسية." : "Please select a group.");
        return;
    }

    std::int64_t studentId = m_studentCombo->currentData().toLongLong();
    std::int64_t groupId = m_groupCombo->currentData().toLongLong();

    double discountEgp = m_discountEdit->text().trimmed().toDouble();
    if (discountEgp < 0) discountEgp = 0;
    std::int64_t discountPiastres = static_cast<std::int64_t>(discountEgp * 100.0 + 0.5);

    application::EnrollStudentCommand cmd{
        .studentId = studentId,
        .groupId = groupId,
        .discountPiastres = discountPiastres,
        .notes = m_notesEdit->toPlainText().trimmed().toStdString()
    };

    auto res = m_appContext->enrollmentUseCases()->enrollStudent(cmd);
    if (res.isOk()) {
        QMessageBox::information(this, isRtl ? "تم بنجاح" : "Success",
                                 isRtl ? "تم تسجيل الطالب في المجموعة بنجاح!" : "Student enrolled successfully!");
        emit enrollmentCompleted(res.value());
        accept();
    } else {
        QMessageBox::critical(this, isRtl ? "تعذر التسجيل" : "Enrollment Failed",
                              QString::fromStdString(res.error().message()));
    }
}

} // namespace pixcode::desktop

