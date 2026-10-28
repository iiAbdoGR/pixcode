#include "StudentsView.h"
#include "../dialogs/StudentFormDialog.h"
#include "../dialogs/StudentDetailDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"

namespace pixcode::desktop {

StudentsView::StudentsView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QWidget(parent), m_appContext(std::move(appContext)) {

    setupUi();
    updateTranslations();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &StudentsView::updateTranslations);

    loadStudents();
}

void StudentsView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(16);

    // Header layout
    auto* headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("headerTitle");

    m_addButton = new QPushButton(this);
    m_addButton->setObjectName("primaryButton");
    connect(m_addButton, &QPushButton::clicked, this, &StudentsView::onAddStudentClicked);

    m_detailsButton = new QPushButton(this);
    m_detailsButton->setObjectName("secondaryButton");
    connect(m_detailsButton, &QPushButton::clicked, this, &StudentsView::onViewDetailsClicked);

    m_editButton = new QPushButton(this);
    m_editButton->setObjectName("secondaryButton");
    connect(m_editButton, &QPushButton::clicked, this, &StudentsView::onEditStudentClicked);

    m_archiveButton = new QPushButton(this);
    m_archiveButton->setObjectName("secondaryButton");
    connect(m_archiveButton, &QPushButton::clicked, this, &StudentsView::onArchiveStudentClicked);

    m_refreshButton = new QPushButton(this);
    m_refreshButton->setObjectName("secondaryButton");
    connect(m_refreshButton, &QPushButton::clicked, this, &StudentsView::refresh);

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_detailsButton);
    headerLayout->addWidget(m_editButton);
    headerLayout->addWidget(m_archiveButton);
    headerLayout->addWidget(m_refreshButton);
    headerLayout->addWidget(m_addButton);

    // Filters layout
    auto* filterLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StudentsView::onSearchChanged);

    m_statusCombo = new QComboBox(this);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StudentsView::onStatusFilterChanged);

    m_countLabel = new QLabel(this);
    m_countLabel->setStyleSheet("color: #7E939C; font-weight: 600;");

    filterLayout->addWidget(m_searchEdit, 3);
    filterLayout->addWidget(m_statusCombo, 1);
    filterLayout->addStretch(1);
    filterLayout->addWidget(m_countLabel);

    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &StudentsView::onRowDoubleClicked);

    mainLayout->addLayout(headerLayout);
    mainLayout->addLayout(filterLayout);
    mainLayout->addWidget(m_table);
}

void StudentsView::updateTranslations() {
    bool isRtl = LocalizationManager::instance().isRtl();

    m_titleLabel->setText(isRtl ? "إدارة الطلاب" : "Student Management");
    m_addButton->setText(isRtl ? "+ طالب جديد" : "+ Add Student");
    m_detailsButton->setText(isRtl ? "التفاصيل والاشتراكات" : "Details & Enrollments");
    m_editButton->setText(isRtl ? "تعديل" : "Edit");
    m_archiveButton->setText(isRtl ? "أرشفة" : "Archive");
    m_refreshButton->setText(isRtl ? "تحديث" : "Refresh");

    m_searchEdit->setPlaceholderText(isRtl ? "البحث بالاسم، رقم الهاتف، أو كود الطالب..."
                                           : "Search by name, phone, or student code...");

    m_statusCombo->clear();
    m_statusCombo->addItem(isRtl ? "جميع الحالات" : "All Statuses", "all");
    m_statusCombo->addItem(isRtl ? "الطلاب النشطون فقط" : "Active Only", "active");
    m_statusCombo->addItem(isRtl ? "الطلاب المؤرشفون" : "Archived Only", "archived");
    m_statusCombo->setCurrentIndex(1); // default active

    m_table->setHorizontalHeaderLabels({
        isRtl ? "كود الطالب" : "Student Code",
        isRtl ? "الاسم الكامل" : "Full Name",
        isRtl ? "الهاتف" : "Phone",
        isRtl ? "ولي الأمر" : "Guardian",
        isRtl ? "الحالة" : "Status",
        isRtl ? "العنوان" : "Address"
    });
}

void StudentsView::refresh() {
    loadStudents();
}

void StudentsView::onSearchChanged(const QString& /*text*/) {
    loadStudents();
}

void StudentsView::onStatusFilterChanged(int /*index*/) {
    loadStudents();
}

void StudentsView::loadStudents() {
    bool isRtl = LocalizationManager::instance().isRtl();

    application::StudentFilter filter;
    filter.searchText = m_searchEdit->text().trimmed().toStdString();

    QString statusVal = m_statusCombo->currentData().toString();
    if (statusVal == "active") {
        filter.status = domain::StudentStatus::Active;
    } else if (statusVal == "archived") {
        filter.status = domain::StudentStatus::Archived;
    }
    filter.pageSize = 500;

    auto res = m_appContext->studentRepository()->findAll(filter);
    m_table->setRowCount(static_cast<int>(res.size()));

    for (int i = 0; i < static_cast<int>(res.size()); ++i) {
        const auto& s = res[i];

        auto* itemCode = new QTableWidgetItem(QString::fromStdString(s.studentCode));
        itemCode->setData(Qt::UserRole, QVariant::fromValue(static_cast<qint64>(s.id)));
        itemCode->setFont(QFont("Segoe UI", 9, QFont::Bold));

        auto* itemName = new QTableWidgetItem(QString::fromStdString(s.fullName));
        auto* itemPhone = new QTableWidgetItem(QString::fromStdString(s.phone));

        QString guardian = QString::fromStdString(s.guardianName);
        if (!QString::fromStdString(s.guardianPhone).isEmpty()) {
            guardian += " (" + QString::fromStdString(s.guardianPhone) + ")";
        }
        auto* itemGuardian = new QTableWidgetItem(guardian);

        bool isActive = (s.status == domain::StudentStatus::Active);
        auto* itemStatus = new QTableWidgetItem(isActive ? (isRtl ? "نشط" : "Active") : (isRtl ? "مؤرشف" : "Archived"));
        itemStatus->setForeground(QBrush(QColor(isActive ? "#2E7D32" : "#D32F2F")));

        auto* itemAddress = new QTableWidgetItem(QString::fromStdString(s.address));

        m_table->setItem(i, 0, itemCode);
        m_table->setItem(i, 1, itemName);
        m_table->setItem(i, 2, itemPhone);
        m_table->setItem(i, 3, itemGuardian);
        m_table->setItem(i, 4, itemStatus);
        m_table->setItem(i, 5, itemAddress);
    }

    m_countLabel->setText(QString(isRtl ? "إجمالي الطلاب: %1" : "Total: %1").arg(res.size()));
}

void StudentsView::onAddStudentClicked() {
    StudentFormDialog dlg(m_appContext, std::nullopt, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadStudents();
    }
}

void StudentsView::onRowDoubleClicked(int row, int /*column*/) {
    if (row < 0) return;
    std::int64_t studentId = m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
    StudentDetailDialog dlg(m_appContext, studentId, this);
    connect(&dlg, &StudentDetailDialog::studentUpdated, this, &StudentsView::refresh);
    dlg.exec();
}

void StudentsView::onViewDetailsClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد طالب من القائمة." : "Please select a student from the list.");
        return;
    }
    onRowDoubleClicked(row, 0);
}

void StudentsView::onEditStudentClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد طالب من القائمة." : "Please select a student from the list.");
        return;
    }
    std::int64_t studentId = m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
    StudentFormDialog dlg(m_appContext, studentId, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadStudents();
    }
}

void StudentsView::onArchiveStudentClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد طالب من القائمة." : "Please select a student from the list.");
        return;
    }
    std::int64_t studentId = m_table->item(row, 0)->data(Qt::UserRole).toLongLong();
    if (QMessageBox::question(this, isRtl ? "تأكيد" : "Confirm",
            isRtl ? "هل أنت متأكد من تغيير حالة أرشفة هذا الطالب؟" : "Toggle archive status for this student?") == QMessageBox::Yes) {
        auto res = m_appContext->studentUseCases()->getStudent(studentId);
        if (res.isOk()) {
            if (res.value().status == "active") {
                m_appContext->studentUseCases()->archiveStudent(studentId);
            } else {
                m_appContext->studentUseCases()->activateStudent(studentId);
            }
            loadStudents();
        }
    }
}

} // namespace pixcode::desktop
