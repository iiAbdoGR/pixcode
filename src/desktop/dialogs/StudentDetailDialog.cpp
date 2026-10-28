#include "StudentDetailDialog.h"
#include "StudentFormDialog.h"
#include "EnrollmentDialog.h"
#include "PaymentDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"
#include "domain/value_objects/Money.h"

namespace pixcode::desktop {

StudentDetailDialog::StudentDetailDialog(std::shared_ptr<ApplicationContext> appContext,
                                         std::int64_t studentId,
                                         QWidget* parent)
    : QDialog(parent),
      m_appContext(std::move(appContext)),
      m_studentId(studentId) {

    setupUi();
    loadData();
}

void StudentDetailDialog::setupUi() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setWindowTitle(isRtl ? "تفاصيل الطالب والاشتراكات المالية" : "Student Details & Financial Record");
    setMinimumSize(850, 550);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    // Header info box
    auto* infoFrame = new QFrame(this);
    infoFrame->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD2D9; border-radius: 8px; padding: 12px;");
    auto* infoLayout = new QFormLayout(infoFrame);
    infoLayout->setSpacing(8);

    m_nameLabel = new QLabel(infoFrame);
    m_nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1F2933;");

    m_codeLabel = new QLabel(infoFrame);
    m_codeLabel->setStyleSheet("font-weight: 600; color: #4C5F6B;");

    m_phoneLabel = new QLabel(infoFrame);
    m_guardianLabel = new QLabel(infoFrame);
    m_addressLabel = new QLabel(infoFrame);
    m_statusLabel = new QLabel(infoFrame);
    m_notesLabel = new QLabel(infoFrame);
    m_notesLabel->setStyleSheet("color: #7E939C; font-style: italic;");

    infoLayout->addRow(isRtl ? "الاسم:" : "Name:", m_nameLabel);
    infoLayout->addRow(isRtl ? "كود الطالب:" : "Student Code:", m_codeLabel);
    infoLayout->addRow(isRtl ? "الهاتف:" : "Phone:", m_phoneLabel);
    infoLayout->addRow(isRtl ? "ولي الأمر:" : "Guardian:", m_guardianLabel);
    infoLayout->addRow(isRtl ? "العنوان:" : "Address:", m_addressLabel);
    infoLayout->addRow(isRtl ? "الحالة:" : "Status:", m_statusLabel);
    infoLayout->addRow(isRtl ? "ملاحظات:" : "Notes:", m_notesLabel);

    mainLayout->addWidget(infoFrame);

    // Enrollments section
    auto* enrollHeaderLayout = new QHBoxLayout();
    auto* enrollTitle = new QLabel(isRtl ? "الاشتراكات والمجموعات المسجل بها:" : "Enrolled Groups & Payments:", this);
    enrollTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #2E3A44;");

    m_newEnrollmentBtn = new QPushButton(isRtl ? "+ تسجيل في مجموعة" : "+ Enroll in Group", this);
    m_newEnrollmentBtn->setObjectName("primaryButton");
    connect(m_newEnrollmentBtn, &QPushButton::clicked, this, &StudentDetailDialog::onNewEnrollmentClicked);

    m_recordPaymentBtn = new QPushButton(isRtl ? "تسجيل دفعة" : "Record Payment", this);
    m_recordPaymentBtn->setObjectName("secondaryButton");
    connect(m_recordPaymentBtn, &QPushButton::clicked, this, &StudentDetailDialog::onRecordPaymentClicked);

    enrollHeaderLayout->addWidget(enrollTitle);
    enrollHeaderLayout->addStretch();
    enrollHeaderLayout->addWidget(m_newEnrollmentBtn);
    enrollHeaderLayout->addWidget(m_recordPaymentBtn);

    mainLayout->addLayout(enrollHeaderLayout);

    m_enrollmentsTable = new QTableWidget(this);
    m_enrollmentsTable->setColumnCount(8);
    m_enrollmentsTable->setHorizontalHeaderLabels({
        isRtl ? "المجموعة" : "Group",
        isRtl ? "الكورس" : "Course",
        isRtl ? "الرسوم المتفق عليها" : "Agreed Fee",
        isRtl ? "الخصم" : "Discount",
        isRtl ? "الصافي" : "Net Fee",
        isRtl ? "المدفوع" : "Paid",
        isRtl ? "المتبقي" : "Balance",
        isRtl ? "الحالة" : "Status"
    });
    m_enrollmentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_enrollmentsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_enrollmentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_enrollmentsTable->horizontalHeader()->setStretchLastSection(true);
    m_enrollmentsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    mainLayout->addWidget(m_enrollmentsTable);

    // Actions layout
    auto* bottomLayout = new QHBoxLayout();
    m_editBtn = new QPushButton(isRtl ? "تعديل بيانات الطالب" : "Edit Student Info", this);
    m_editBtn->setObjectName("secondaryButton");
    connect(m_editBtn, &QPushButton::clicked, this, &StudentDetailDialog::onEditClicked);

    m_toggleArchiveBtn = new QPushButton(this);
    m_toggleArchiveBtn->setObjectName("secondaryButton");
    connect(m_toggleArchiveBtn, &QPushButton::clicked, this, &StudentDetailDialog::onToggleArchiveClicked);

    m_closeBtn = new QPushButton(isRtl ? "إغلاق" : "Close", this);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    bottomLayout->addWidget(m_editBtn);
    bottomLayout->addWidget(m_toggleArchiveBtn);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(bottomLayout);
}

void StudentDetailDialog::loadData() {
    bool isRtl = LocalizationManager::instance().isRtl();
    auto sRes = m_appContext->studentUseCases()->getStudent(m_studentId);
    if (!sRes.isOk()) return;

    const auto& s = sRes.value();
    m_nameLabel->setText(QString::fromStdString(s.fullName));
    m_codeLabel->setText(QString::fromStdString(s.studentCode));
    m_phoneLabel->setText(QString::fromStdString(s.phone).isEmpty() ? "-" : QString::fromStdString(s.phone));

    QString guardian = QString::fromStdString(s.guardianName);
    if (!QString::fromStdString(s.guardianPhone).isEmpty()) {
        guardian += " (" + QString::fromStdString(s.guardianPhone) + ")";
    }
    m_guardianLabel->setText(guardian.isEmpty() ? "-" : guardian);
    m_addressLabel->setText(QString::fromStdString(s.address).isEmpty() ? "-" : QString::fromStdString(s.address));

    bool isActive = (s.status == "active");
    m_statusLabel->setText(isActive ? (isRtl ? "نشط" : "Active") : (isRtl ? "مؤرشف" : "Archived"));
    m_statusLabel->setStyleSheet(isActive ? "color: #2E7D32; font-weight: bold;" : "color: #D32F2F; font-weight: bold;");

    m_toggleArchiveBtn->setText(isActive ? (isRtl ? "أرشفة الطالب" : "Archive Student")
                                         : (isRtl ? "إلغاء الأرشفة" : "Activate Student"));

    m_notesLabel->setText(QString::fromStdString(s.notes).isEmpty() ? "-" : QString::fromStdString(s.notes));

    // Load enrollments
    auto eRes = m_appContext->enrollmentUseCases()->listByStudent(m_studentId);
    if (eRes.isOk()) {
        const auto& enrollments = eRes.value();
        m_enrollmentsTable->setRowCount(static_cast<int>(enrollments.size()));

        for (int i = 0; i < static_cast<int>(enrollments.size()); ++i) {
            const auto& e = enrollments[i];

            domain::Money agreed(e.agreedFeePiastres);
            domain::Money discount(e.discountPiastres);
            domain::Money net(e.netFeePiastres);
            domain::Money paid(e.paidPiastres);
            domain::Money balance(e.balancePiastres);

            auto* itemGroup = new QTableWidgetItem(QString::fromStdString(e.groupName));
            itemGroup->setData(Qt::UserRole, QVariant::fromValue(static_cast<qint64>(e.id)));

            auto* itemCourse = new QTableWidgetItem(QString::fromStdString(e.courseName));
            auto* itemAgreed = new QTableWidgetItem(QString::fromStdString(agreed.format()));
            auto* itemDiscount = new QTableWidgetItem(QString::fromStdString(discount.format()));
            auto* itemNet = new QTableWidgetItem(QString::fromStdString(net.format()));
            auto* itemPaid = new QTableWidgetItem(QString::fromStdString(paid.format()));
            auto* itemBalance = new QTableWidgetItem(QString::fromStdString(balance.format()));

            if (e.balancePiastres > 0) {
                itemBalance->setForeground(QBrush(QColor("#D32F2F")));
            } else {
                itemBalance->setForeground(QBrush(QColor("#2E7D32")));
            }

            QString stText = QString::fromStdString(e.status);
            if (e.status == "active") stText = isRtl ? "ساري" : "Active";
            else if (e.status == "cancelled") stText = isRtl ? "ملغي" : "Cancelled";
            auto* itemStatus = new QTableWidgetItem(stText);

            m_enrollmentsTable->setItem(i, 0, itemGroup);
            m_enrollmentsTable->setItem(i, 1, itemCourse);
            m_enrollmentsTable->setItem(i, 2, itemAgreed);
            m_enrollmentsTable->setItem(i, 3, itemDiscount);
            m_enrollmentsTable->setItem(i, 4, itemNet);
            m_enrollmentsTable->setItem(i, 5, itemPaid);
            m_enrollmentsTable->setItem(i, 6, itemBalance);
            m_enrollmentsTable->setItem(i, 7, itemStatus);
        }
    }
}

void StudentDetailDialog::onNewEnrollmentClicked() {
    EnrollmentDialog dlg(m_appContext, m_studentId, std::nullopt, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadData();
        emit studentUpdated();
    }
}

void StudentDetailDialog::onRecordPaymentClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_enrollmentsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد اشتراك من الجدول لتسجيل دفعة له." : "Please select an enrollment to record a payment.");
        return;
    }

    std::int64_t enrollmentId = m_enrollmentsTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    PaymentDialog dlg(m_appContext, enrollmentId, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadData();
        emit studentUpdated();
    }
}

void StudentDetailDialog::onEditClicked() {
    StudentFormDialog dlg(m_appContext, m_studentId, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadData();
        emit studentUpdated();
    }
}

void StudentDetailDialog::onToggleArchiveClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    auto sRes = m_appContext->studentUseCases()->getStudent(m_studentId);
    if (!sRes.isOk()) return;

    if (sRes.value().status == "active") {
        if (QMessageBox::question(this, isRtl ? "تأكيد الأرشفة" : "Confirm Archive",
                isRtl ? "هل أنت متأكد من أرشفة هذا الطالب؟" : "Are you sure you want to archive this student?") == QMessageBox::Yes) {
            m_appContext->studentUseCases()->archiveStudent(m_studentId);
            loadData();
            emit studentUpdated();
        }
    } else {
        m_appContext->studentUseCases()->activateStudent(m_studentId);
        loadData();
        emit studentUpdated();
    }
}

void StudentDetailDialog::onRefresh() {
    loadData();
}

} // namespace pixcode::desktop
