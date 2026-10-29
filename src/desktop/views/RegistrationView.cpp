#include "RegistrationView.h"
#include "../dialogs/EnrollmentDialog.h"
#include "../dialogs/PaymentDialog.h"
#include "../dialogs/CancelSubscriptionDialog.h"
#include "../dialogs/RestoreSubscriptionDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"
#include "domain/value_objects/Money.h"

namespace pixcode::desktop {

RegistrationView::RegistrationView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QWidget(parent), m_appContext(std::move(appContext)) {

    setupUi();
    updateTranslations();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &RegistrationView::updateTranslations);

    refresh();
}

void RegistrationView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(16);

    auto* headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("headerTitle");

    m_statusFilterCombo = new QComboBox(this);
    m_statusFilterCombo->addItem("All", "all");
    m_statusFilterCombo->addItem("Active", "active");
    m_statusFilterCombo->addItem("Cancelled", "cancelled");
    connect(m_statusFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RegistrationView::onFilterChanged);

    m_enrollButton = new QPushButton(this);
    m_enrollButton->setObjectName("primaryButton");
    connect(m_enrollButton, &QPushButton::clicked, this, &RegistrationView::onNewEnrollmentClicked);

    m_paymentButton = new QPushButton(this);
    m_paymentButton->setObjectName("secondaryButton");
    connect(m_paymentButton, &QPushButton::clicked, this, &RegistrationView::onRecordPaymentClicked);

    m_cancelEnrollmentButton = new QPushButton(this);
    m_cancelEnrollmentButton->setObjectName("secondaryButton");
    connect(m_cancelEnrollmentButton, &QPushButton::clicked, this, &RegistrationView::onCancelEnrollmentClicked);

    m_restoreEnrollmentButton = new QPushButton(this);
    m_restoreEnrollmentButton->setObjectName("secondaryButton");
    connect(m_restoreEnrollmentButton, &QPushButton::clicked, this, &RegistrationView::onRestoreEnrollmentClicked);

    m_refreshButton = new QPushButton(this);
    m_refreshButton->setObjectName("secondaryButton");
    connect(m_refreshButton, &QPushButton::clicked, this, &RegistrationView::refresh);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_statusFilterCombo);
    headerLayout->addWidget(m_restoreEnrollmentButton);
    headerLayout->addWidget(m_cancelEnrollmentButton);
    headerLayout->addWidget(m_paymentButton);
    headerLayout->addWidget(m_refreshButton);
    headerLayout->addWidget(m_enrollButton);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(m_table);
}

void RegistrationView::updateTranslations() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setLayoutDirection(isRtl ? Qt::RightToLeft : Qt::LeftToRight);

    m_titleLabel->setText(LocalizationManager::instance().get("nav.enrollments", "Registration & Enrollments"));
    m_enrollButton->setText(isRtl ? "+ تسجيل طالب في كورس" : "+ New Enrollment");
    m_paymentButton->setText(isRtl ? "تسجيل دفعة / إيصال" : "Record Payment");
    m_cancelEnrollmentButton->setText(isRtl ? "إلغاء اشتراك" : "Cancel Subscription");
    m_restoreEnrollmentButton->setText(isRtl ? "استعادة اشتراك" : "Restore Subscription");
    m_refreshButton->setText(isRtl ? "تحديث" : "Refresh");

    m_statusFilterCombo->setItemText(0, isRtl ? "الكل" : "All Statuses");
    m_statusFilterCombo->setItemText(1, isRtl ? "نشط فقط" : "Active Only");
    m_statusFilterCombo->setItemText(2, isRtl ? "ملغي فقط" : "Cancelled Only");

    m_table->setHorizontalHeaderLabels({
        isRtl ? "كود الطالب" : "Student Code",
        isRtl ? "اسم الطالب" : "Student Name",
        isRtl ? "المجموعة" : "Group",
        isRtl ? "الكورس" : "Course",
        isRtl ? "الصافي المستحق" : "Net Fee",
        isRtl ? "المدفوع" : "Paid",
        isRtl ? "المتبقي" : "Balance",
        isRtl ? "الحالة" : "Status"
    });
}

void RegistrationView::refresh() {
    loadEnrollments();
}

void RegistrationView::onFilterChanged() {
    loadEnrollments();
}

void RegistrationView::loadEnrollments() {
    bool isRtl = LocalizationManager::instance().isRtl();
    auto res = m_appContext->enrollmentUseCases()->listAll();
    if (!res.isOk()) return;

    const auto& allList = res.value();
    QString filter = m_statusFilterCombo->currentData().toString();

    std::vector<application::EnrollmentDto> filtered;
    for (const auto& item : allList) {
        if (filter == "active" && item.status != "active") continue;
        if (filter == "cancelled" && item.status != "cancelled") continue;
        filtered.push_back(item);
    }

    m_table->setRowCount(static_cast<int>(filtered.size()));

    for (int i = 0; i < static_cast<int>(filtered.size()); ++i) {
        const auto& dto = filtered[i];

        domain::Money netMoney(dto.netFeePiastres);
        domain::Money paidMoney(dto.paidPiastres);
        domain::Money balMoney(dto.balancePiastres);

        auto* itemCode = new QTableWidgetItem(QString::fromStdString(dto.studentCode));
        itemCode->setData(Qt::UserRole, QVariant::fromValue(static_cast<qint64>(dto.id)));
        itemCode->setFont(QFont("Segoe UI", 9, QFont::Bold));

        auto* itemName = new QTableWidgetItem(QString::fromStdString(dto.studentName));
        auto* itemGroup = new QTableWidgetItem(QString::fromStdString(dto.groupName));
        auto* itemCourse = new QTableWidgetItem(QString::fromStdString(dto.courseName));
        auto* itemNet = new QTableWidgetItem(QString::fromStdString(netMoney.format()));
        auto* itemPaid = new QTableWidgetItem(QString::fromStdString(paidMoney.format()));
        auto* itemBal = new QTableWidgetItem(QString::fromStdString(balMoney.format()));

        if (dto.balancePiastres > 0) {
            itemBal->setForeground(QBrush(QColor("#D32F2F")));
        } else {
            itemBal->setForeground(QBrush(QColor("#2E7D32")));
        }

        QString st = QString::fromStdString(dto.status);
        if (dto.status == "active") st = isRtl ? "ساري" : "Active";
        else if (dto.status == "cancelled") st = isRtl ? "ملغي" : "Cancelled";
        else if (dto.status == "transferred") st = isRtl ? "محول" : "Transferred";
        else if (dto.status == "completed") st = isRtl ? "مكتمل" : "Completed";
        auto* itemStatus = new QTableWidgetItem(st);

        m_table->setItem(i, 0, itemCode);
        m_table->setItem(i, 1, itemName);
        m_table->setItem(i, 2, itemGroup);
        m_table->setItem(i, 3, itemCourse);
        m_table->setItem(i, 4, itemNet);
        m_table->setItem(i, 5, itemPaid);
        m_table->setItem(i, 6, itemBal);
        m_table->setItem(i, 7, itemStatus);
    }
}

void RegistrationView::onNewEnrollmentClicked() {
    EnrollmentDialog dlg(m_appContext, std::nullopt, std::nullopt, this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void RegistrationView::onRecordPaymentClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد اشتراك من الجدول." : "Please select an enrollment from the table.");
        return;
    }
    std::int64_t enrollmentId = m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
    PaymentDialog dlg(m_appContext, enrollmentId, this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void RegistrationView::onCancelEnrollmentClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد اشتراك من الجدول." : "Please select an enrollment from the table.");
        return;
    }
    std::int64_t enrollmentId = m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
    CancelSubscriptionDialog dlg(m_appContext, enrollmentId, this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

void RegistrationView::onRestoreEnrollmentClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد اشتراك من الجدول." : "Please select an enrollment from the table.");
        return;
    }
    std::int64_t enrollmentId = m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
    RestoreSubscriptionDialog dlg(m_appContext, enrollmentId, this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
    }
}

} // namespace pixcode::desktop
