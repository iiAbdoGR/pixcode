#include "FinanceView.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include <QtSql/QSqlQuery>
#include "../localization/LocalizationManager.h"
#include "domain/value_objects/Money.h"

namespace pixcode::desktop {

FinanceView::FinanceView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QWidget(parent), m_appContext(std::move(appContext)) {

    setupUi();
    updateTranslations();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &FinanceView::updateTranslations);

    refresh();
}

void FinanceView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(16);

    auto* headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("headerTitle");

    m_viewReceiptBtn = new QPushButton(this);
    m_viewReceiptBtn->setObjectName("secondaryButton");
    connect(m_viewReceiptBtn, &QPushButton::clicked, this, &FinanceView::onViewReceiptClicked);

    m_refreshBtn = new QPushButton(this);
    m_refreshBtn->setObjectName("secondaryButton");
    connect(m_refreshBtn, &QPushButton::clicked, this, &FinanceView::refresh);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_viewReceiptBtn);
    headerLayout->addWidget(m_refreshBtn);

    // Summary KPI cards
    auto* kpiLayout = new QHBoxLayout();
    kpiLayout->setSpacing(16);

    auto* colCard = new QFrame(this);
    colCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD2D9; border-radius: 8px; padding: 16px;");
    auto* colLayout = new QVBoxLayout(colCard);
    auto* colTitle = new QLabel("إجمالي المتحصلات النقدية", colCard);
    colTitle->setStyleSheet("color: #7E939C; font-weight: 600;");
    m_totalCollectedLabel = new QLabel("0.00 EGP", colCard);
    m_totalCollectedLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2E7D32;");
    colLayout->addWidget(colTitle);
    colLayout->addWidget(m_totalCollectedLabel);

    auto* recCard = new QFrame(this);
    recCard->setStyleSheet("background-color: #FFFFFF; border: 1px solid #CBD2D9; border-radius: 8px; padding: 16px;");
    auto* recLayout = new QVBoxLayout(recCard);
    auto* recTitle = new QLabel("المستحقات المتبقية طرف الطلاب", recCard);
    recTitle->setStyleSheet("color: #7E939C; font-weight: 600;");
    m_totalReceivablesLabel = new QLabel("0.00 EGP", recCard);
    m_totalReceivablesLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #D32F2F;");
    recLayout->addWidget(recTitle);
    recLayout->addWidget(m_totalReceivablesLabel);

    kpiLayout->addWidget(colCard);
    kpiLayout->addWidget(recCard);

    // Filters
    auto* filterLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &FinanceView::onSearchChanged);
    filterLayout->addWidget(m_searchEdit);

    // Payments Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &FinanceView::onRowDoubleClicked);

    mainLayout->addLayout(headerLayout);
    mainLayout->addLayout(kpiLayout);
    mainLayout->addLayout(filterLayout);
    mainLayout->addWidget(m_table);
}

void FinanceView::updateTranslations() {
    bool isRtl = LocalizationManager::instance().isRtl();

    m_titleLabel->setText(isRtl ? "المالية وسجل المقبوضات والإيصالات" : "Finance, Payments & Receipts");
    m_viewReceiptBtn->setText(isRtl ? "عرض تفاصيل الإيصال" : "View Receipt Details");
    m_refreshBtn->setText(isRtl ? "تحديث" : "Refresh");
    m_searchEdit->setPlaceholderText(isRtl ? "بحث برقم الإيصال أو اسم الطالب..." : "Search by receipt number or student name...");

    m_table->setHorizontalHeaderLabels({
        isRtl ? "رقم الإيصال" : "Receipt No",
        isRtl ? "اسم الطالب" : "Student Name",
        isRtl ? "المجموعة" : "Group",
        isRtl ? "المبلغ" : "Amount",
        isRtl ? "طريقة الدفع" : "Method",
        isRtl ? "تاريخ ووقت التحصيل" : "Payment Date",
        isRtl ? "ملاحظات" : "Notes"
    });
}

void FinanceView::refresh() {
    loadFinancials();
}

void FinanceView::onSearchChanged(const QString& /*text*/) {
    loadFinancials();
}

void FinanceView::loadFinancials() {
    bool isRtl = LocalizationManager::instance().isRtl();
    auto db = m_appContext->databaseManager()->getConnection();

    // 1. Calculate KPIs
    {
        QSqlQuery q(db);
        q.exec("SELECT COALESCE(SUM(amount_piastres), 0) FROM payments;");
        if (q.next()) {
            domain::Money collected(q.value(0).toLongLong());
            m_totalCollectedLabel->setText(QString::fromStdString(collected.format()));
        }

        q.exec(R"(
            SELECT COALESCE(SUM(net_fee_piastres), 0) - COALESCE((SELECT SUM(amount_piastres) FROM payments), 0)
            FROM enrollments
            WHERE status = 'active';
        )");
        if (q.next()) {
            std::int64_t diff = q.value(0).toLongLong();
            if (diff < 0) diff = 0;
            domain::Money remaining(diff);
            m_totalReceivablesLabel->setText(QString::fromStdString(remaining.format()));
        }
    }

    // 2. Load payments list
    QString sql = R"(
        SELECT p.id, p.receipt_number, s.full_name, g.name AS group_name,
               p.amount_piastres, p.payment_method, p.paid_at_utc, p.notes
        FROM payments p
        JOIN enrollments e ON p.enrollment_id = e.id
        JOIN students s ON e.student_id = s.id
        JOIN groups g ON e.group_id = g.id
        WHERE 1=1
    )";

    QString search = m_searchEdit->text().trimmed();
    if (!search.isEmpty()) {
        sql += " AND (p.receipt_number LIKE :s1 OR s.full_name LIKE :s2)";
    }
    sql += " ORDER BY p.id DESC LIMIT 300;";

    QSqlQuery q(db);
    q.prepare(sql);
    if (!search.isEmpty()) {
        QString term = QString("%%1%").arg(search);
        q.bindValue(":s1", term);
        q.bindValue(":s2", term);
    }

    if (!q.exec()) return;

    QList<QList<QVariant>> rows;
    while (q.next()) {
        rows.append({
            q.value("id"),
            q.value("receipt_number"),
            q.value("full_name"),
            q.value("group_name"),
            q.value("amount_piastres"),
            q.value("payment_method"),
            q.value("paid_at_utc"),
            q.value("notes")
        });
    }

    m_table->setRowCount(rows.size());

    for (int i = 0; i < rows.size(); ++i) {
        const auto& r = rows[i];
        domain::Money amount(r[4].toLongLong());

        auto* itemRcp = new QTableWidgetItem(r[1].toString());
        itemRcp->setData(Qt::UserRole, r[0]);
        itemRcp->setFont(QFont("Segoe UI", 9, QFont::Bold));

        auto* itemName = new QTableWidgetItem(r[2].toString());
        auto* itemGroup = new QTableWidgetItem(r[3].toString());
        auto* itemAmount = new QTableWidgetItem(QString::fromStdString(amount.format()));
        itemAmount->setForeground(QBrush(QColor("#2E7D32")));

        QString methodStr = r[5].toString();
        if (methodStr == "cash") methodStr = isRtl ? "نقدي" : "Cash";
        else if (methodStr == "bank_transfer") methodStr = isRtl ? "تحويل بنكي" : "Bank Transfer";
        auto* itemMethod = new QTableWidgetItem(methodStr);

        auto* itemDate = new QTableWidgetItem(r[6].toString().left(19).replace('T', ' '));
        auto* itemNotes = new QTableWidgetItem(r[7].toString());

        m_table->setItem(i, 0, itemRcp);
        m_table->setItem(i, 1, itemName);
        m_table->setItem(i, 2, itemGroup);
        m_table->setItem(i, 3, itemAmount);
        m_table->setItem(i, 4, itemMethod);
        m_table->setItem(i, 5, itemDate);
        m_table->setItem(i, 6, itemNotes);
    }
}

void FinanceView::onRowDoubleClicked(int /*row*/, int /*column*/) {
    onViewReceiptClicked();
}

void FinanceView::onViewReceiptClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد إيصال من الجدول." : "Please select a payment receipt from the table.");
        return;
    }

    QString rcp = m_table->item(row, 0)->text();
    QString student = m_table->item(row, 1)->text();
    QString group = m_table->item(row, 2)->text();
    QString amount = m_table->item(row, 3)->text();
    QString method = m_table->item(row, 4)->text();
    QString date = m_table->item(row, 5)->text();
    QString notes = m_table->item(row, 6)->text();

    QString details = QString(
        "=========================================\n"
        "           PIX CODE ACADEMY              \n"
        "        OFFICIAL PAYMENT RECEIPT         \n"
        "=========================================\n\n"
        "Receipt Number : %1\n"
        "Date & Time    : %2\n"
        "Student Name   : %3\n"
        "Course / Group : %4\n"
        "Amount Paid    : %5\n"
        "Payment Method : %6\n"
        "Notes          : %7\n\n"
        "=========================================\n"
        "  Authorized Signature & Academy Stamp   \n"
        "========================================="
    ).arg(rcp, date, student, group, amount, method, notes.isEmpty() ? "-" : notes);

    QMessageBox::information(this, isRtl ? "إيصال قبض رسمي" : "Official Receipt", details);
}

} // namespace pixcode::desktop
