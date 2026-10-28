#include "RegistrationView.h"
#include "../dialogs/EnrollmentDialog.h"
#include "../dialogs/PaymentDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include <QtSql/QSqlQuery>
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

    m_enrollButton = new QPushButton(this);
    m_enrollButton->setObjectName("primaryButton");
    connect(m_enrollButton, &QPushButton::clicked, this, &RegistrationView::onNewEnrollmentClicked);

    m_paymentButton = new QPushButton(this);
    m_paymentButton->setObjectName("secondaryButton");
    connect(m_paymentButton, &QPushButton::clicked, this, &RegistrationView::onRecordPaymentClicked);

    m_cancelEnrollmentButton = new QPushButton(this);
    m_cancelEnrollmentButton->setObjectName("secondaryButton");
    connect(m_cancelEnrollmentButton, &QPushButton::clicked, this, &RegistrationView::onCancelEnrollmentClicked);

    m_refreshButton = new QPushButton(this);
    m_refreshButton->setObjectName("secondaryButton");
    connect(m_refreshButton, &QPushButton::clicked, this, &RegistrationView::refresh);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
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

    m_titleLabel->setText(isRtl ? "إدارة الاشتراكات والتسجيل" : "Registration & Enrollments");
    m_enrollButton->setText(isRtl ? "+ تسجيل طالب في كورس" : "+ New Enrollment");
    m_paymentButton->setText(isRtl ? "تسجيل دفعة / إيصال" : "Record Payment");
    m_cancelEnrollmentButton->setText(isRtl ? "إلغاء اشتراك" : "Cancel Enrollment");
    m_refreshButton->setText(isRtl ? "تحديث" : "Refresh");

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

void RegistrationView::loadEnrollments() {
    bool isRtl = LocalizationManager::instance().isRtl();
    auto db = m_appContext->databaseManager()->getConnection();

    QSqlQuery q(db);
    q.prepare(R"(
        SELECT e.id, s.student_code, s.full_name, g.name AS group_name, c.name AS course_name,
               e.net_fee_piastres, e.status
        FROM enrollments e
        JOIN students s ON e.student_id = s.id
        JOIN groups g ON e.group_id = g.id
        JOIN courses c ON g.course_id = c.id
        ORDER BY e.id DESC
        LIMIT 200;
    )");

    if (!q.exec()) return;

    QList<QList<QVariant>> rows;
    while (q.next()) {
        rows.append({
            q.value("id"),
            q.value("student_code"),
            q.value("full_name"),
            q.value("group_name"),
            q.value("course_name"),
            q.value("net_fee_piastres"),
            q.value("status")
        });
    }

    m_table->setRowCount(rows.size());

    for (int i = 0; i < rows.size(); ++i) {
        const auto& r = rows[i];
        std::int64_t eid = r[0].toLongLong();
        std::int64_t netPiastres = r[5].toLongLong();
        std::int64_t paidPiastres = m_appContext->paymentRepository()->totalPaid(eid);
        std::int64_t balancePiastres = netPiastres - paidPiastres;

        domain::Money netMoney(netPiastres);
        domain::Money paidMoney(paidPiastres);
        domain::Money balMoney(balancePiastres);

        auto* itemCode = new QTableWidgetItem(r[1].toString());
        itemCode->setData(Qt::UserRole, QVariant::fromValue(static_cast<qint64>(eid)));
        itemCode->setFont(QFont("Segoe UI", 9, QFont::Bold));

        auto* itemName = new QTableWidgetItem(r[2].toString());
        auto* itemGroup = new QTableWidgetItem(r[3].toString());
        auto* itemCourse = new QTableWidgetItem(r[4].toString());
        auto* itemNet = new QTableWidgetItem(QString::fromStdString(netMoney.format()));
        auto* itemPaid = new QTableWidgetItem(QString::fromStdString(paidMoney.format()));
        auto* itemBal = new QTableWidgetItem(QString::fromStdString(balMoney.format()));

        if (balancePiastres > 0) {
            itemBal->setForeground(QBrush(QColor("#D32F2F")));
        } else {
            itemBal->setForeground(QBrush(QColor("#2E7D32")));
        }

        QString st = r[6].toString();
        if (st == "active") st = isRtl ? "ساري" : "Active";
        else if (st == "cancelled") st = isRtl ? "ملغي" : "Cancelled";
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
    if (QMessageBox::question(this, isRtl ? "تأكيد الإلغاء" : "Confirm Cancellation",
            isRtl ? "هل أنت متأكد من رغبتك في إلغاء هذا الاشتراك؟" : "Are you sure you want to cancel this enrollment?") == QMessageBox::Yes) {
        auto res = m_appContext->enrollmentUseCases()->cancelEnrollment(enrollmentId, "User requested cancellation");
        if (res.isOk()) {
            refresh();
        } else {
            QMessageBox::critical(this, isRtl ? "خطأ" : "Error",
                                  QString::fromStdString(res.error().message()));
        }
    }
}

} // namespace pixcode::desktop

