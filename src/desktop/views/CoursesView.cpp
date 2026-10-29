#include "CoursesView.h"
#include "../dialogs/CourseFormDialog.h"
#include "../dialogs/GroupFormDialog.h"
#include "../dialogs/EnrollmentDialog.h"
#include "../dialogs/InstructorFormDialog.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMessageBox>
#include "../localization/LocalizationManager.h"
#include "domain/value_objects/Money.h"

namespace pixcode::desktop {

CoursesView::CoursesView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent)
    : QWidget(parent), m_appContext(std::move(appContext)) {

    setupUi();
    updateTranslations();

    connect(&LocalizationManager::instance(), &LocalizationManager::languageChanged,
            this, &CoursesView::updateTranslations);

    refresh();
}

void CoursesView::selectCoursesTab() {
    if (m_tabs) m_tabs->setCurrentIndex(0);
}

void CoursesView::selectGroupsTab() {
    if (m_tabs) m_tabs->setCurrentIndex(1);
}

void CoursesView::selectInstructorsTab() {
    if (m_tabs) m_tabs->setCurrentIndex(2);
}

void CoursesView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(16);

    auto* headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("headerTitle");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    m_tabs = new QTabWidget(this);

    // Tab 1: Courses
    m_coursesTab = new QWidget(this);
    auto* cLayout = new QVBoxLayout(m_coursesTab);
    cLayout->setContentsMargins(16, 16, 16, 16);
    cLayout->setSpacing(12);

    auto* cActions = new QHBoxLayout();
    m_addCourseBtn = new QPushButton(this);
    m_addCourseBtn->setObjectName("primaryButton");
    connect(m_addCourseBtn, &QPushButton::clicked, this, &CoursesView::onAddCourseClicked);

    m_editCourseBtn = new QPushButton(this);
    m_editCourseBtn->setObjectName("secondaryButton");
    connect(m_editCourseBtn, &QPushButton::clicked, this, &CoursesView::onEditCourseClicked);

    m_refreshCoursesBtn = new QPushButton(this);
    m_refreshCoursesBtn->setObjectName("secondaryButton");
    connect(m_refreshCoursesBtn, &QPushButton::clicked, this, &CoursesView::loadCourses);

    cActions->addWidget(m_addCourseBtn);
    cActions->addWidget(m_editCourseBtn);
    cActions->addStretch();
    cActions->addWidget(m_refreshCoursesBtn);
    cLayout->addLayout(cActions);

    m_coursesTable = new QTableWidget(this);
    m_coursesTable->setColumnCount(4);
    m_coursesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_coursesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_coursesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_coursesTable->horizontalHeader()->setStretchLastSection(true);
    m_coursesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(m_coursesTable, &QTableWidget::cellDoubleClicked, this, &CoursesView::onEditCourseClicked);
    cLayout->addWidget(m_coursesTable);

    // Tab 2: Groups
    m_groupsTab = new QWidget(this);
    auto* gLayout = new QVBoxLayout(m_groupsTab);
    gLayout->setContentsMargins(16, 16, 16, 16);
    gLayout->setSpacing(12);

    auto* gActions = new QHBoxLayout();
    m_addGroupBtn = new QPushButton(this);
    m_addGroupBtn->setObjectName("primaryButton");
    connect(m_addGroupBtn, &QPushButton::clicked, this, &CoursesView::onAddGroupClicked);

    m_editGroupBtn = new QPushButton(this);
    m_editGroupBtn->setObjectName("secondaryButton");
    connect(m_editGroupBtn, &QPushButton::clicked, this, &CoursesView::onEditGroupClicked);

    m_enrollInGroupBtn = new QPushButton(this);
    m_enrollInGroupBtn->setObjectName("secondaryButton");
    connect(m_enrollInGroupBtn, &QPushButton::clicked, this, &CoursesView::onEnrollInGroupClicked);

    m_refreshGroupsBtn = new QPushButton(this);
    m_refreshGroupsBtn->setObjectName("secondaryButton");
    connect(m_refreshGroupsBtn, &QPushButton::clicked, this, &CoursesView::loadGroups);

    gActions->addWidget(m_addGroupBtn);
    gActions->addWidget(m_editGroupBtn);
    gActions->addWidget(m_enrollInGroupBtn);
    gActions->addStretch();
    gActions->addWidget(m_refreshGroupsBtn);
    gLayout->addLayout(gActions);

    m_groupsTable = new QTableWidget(this);
    m_groupsTable->setColumnCount(7);
    m_groupsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_groupsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_groupsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_groupsTable->horizontalHeader()->setStretchLastSection(true);
    m_groupsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(m_groupsTable, &QTableWidget::cellDoubleClicked, this, &CoursesView::onEditGroupClicked);
    gLayout->addWidget(m_groupsTable);

    // Tab 3: Instructors
    m_instructorsTab = new QWidget(this);
    auto* instLayout = new QVBoxLayout(m_instructorsTab);
    instLayout->setContentsMargins(16, 16, 16, 16);
    instLayout->setSpacing(12);

    auto* instActions = new QHBoxLayout();
    m_addInstructorBtn = new QPushButton(this);
    m_addInstructorBtn->setObjectName("primaryButton");
    connect(m_addInstructorBtn, &QPushButton::clicked, this, &CoursesView::onAddInstructorClicked);

    m_editInstructorBtn = new QPushButton(this);
    m_editInstructorBtn->setObjectName("secondaryButton");
    connect(m_editInstructorBtn, &QPushButton::clicked, this, &CoursesView::onEditInstructorClicked);

    m_refreshInstructorsBtn = new QPushButton(this);
    m_refreshInstructorsBtn->setObjectName("secondaryButton");
    connect(m_refreshInstructorsBtn, &QPushButton::clicked, this, &CoursesView::loadInstructors);

    instActions->addWidget(m_addInstructorBtn);
    instActions->addWidget(m_editInstructorBtn);
    instActions->addStretch();
    instActions->addWidget(m_refreshInstructorsBtn);
    instLayout->addLayout(instActions);

    m_instructorsTable = new QTableWidget(this);
    m_instructorsTable->setColumnCount(6);
    m_instructorsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_instructorsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_instructorsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_instructorsTable->horizontalHeader()->setStretchLastSection(true);
    m_instructorsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(m_instructorsTable, &QTableWidget::cellDoubleClicked, this, &CoursesView::onEditInstructorClicked);
    instLayout->addWidget(m_instructorsTable);

    m_tabs->addTab(m_coursesTab, "");
    m_tabs->addTab(m_groupsTab, "");
    m_tabs->addTab(m_instructorsTab, "");
    mainLayout->addWidget(m_tabs);
}

void CoursesView::updateTranslations() {
    bool isRtl = LocalizationManager::instance().isRtl();
    setLayoutDirection(isRtl ? Qt::RightToLeft : Qt::LeftToRight);

    m_titleLabel->setText(LocalizationManager::instance().get("nav.courses", "Academic Courses & Groups"));

    m_tabs->setTabText(0, isRtl ? "الكورسات" : "Courses");
    m_tabs->setTabText(1, isRtl ? "المجموعات" : "Groups");
    m_tabs->setTabText(2, isRtl ? "المحاضرون" : "Instructors");

    // Course table headers
    m_coursesTable->setHorizontalHeaderLabels({
        isRtl ? "اسم الكورس" : "Course Name",
        isRtl ? "المدة (أسابيع)" : "Duration (Wks)",
        isRtl ? "السعر الافتراضي" : "Default Fee",
        isRtl ? "الحالة" : "Status"
    });

    m_addCourseBtn->setText(isRtl ? "+ إضافة كورس" : "+ Add Course");
    m_editCourseBtn->setText(isRtl ? "تعديل الكورس" : "Edit Course");
    m_refreshCoursesBtn->setText(isRtl ? "تحديث" : "Refresh");

    // Group table headers
    m_groupsTable->setHorizontalHeaderLabels({
        isRtl ? "اسم المجموعة" : "Group Name",
        isRtl ? "الكورس" : "Course",
        isRtl ? "المحاضر" : "Instructor",
        isRtl ? "المواعيد" : "Schedule",
        isRtl ? "العدد / السعة" : "Enrolled / Cap",
        isRtl ? "السعر" : "Fee",
        isRtl ? "الحالة" : "Status"
    });

    m_addGroupBtn->setText(isRtl ? "+ فتح مجموعة" : "+ Open Group");
    m_editGroupBtn->setText(isRtl ? "تعديل المجموعة" : "Edit Group");
    m_enrollInGroupBtn->setText(isRtl ? "تسجيل طالب" : "Enroll Student");
    m_refreshGroupsBtn->setText(isRtl ? "تحديث" : "Refresh");

    // Instructor table headers
    m_instructorsTable->setHorizontalHeaderLabels({
        isRtl ? "اسم المحاضر" : "Instructor Name",
        isRtl ? "رقم الهاتف" : "Phone",
        isRtl ? "التخصص" : "Specialization",
        isRtl ? "نوع الاستحقاق" : "Compensation Mode",
        isRtl ? "القيمة الأساسية" : "Base Rate",
        isRtl ? "الحالة" : "Status"
    });

    m_addInstructorBtn->setText(isRtl ? "+ إضافة محاضر" : "+ Add Instructor");
    m_editInstructorBtn->setText(isRtl ? "تعديل المحاضر" : "Edit Instructor");
    m_refreshInstructorsBtn->setText(isRtl ? "تحديث" : "Refresh");
}

void CoursesView::refresh() {
    loadCourses();
    loadGroups();
    loadInstructors();
}

void CoursesView::loadCourses() {
    bool isRtl = LocalizationManager::instance().isRtl();
    auto res = m_appContext->courseUseCases()->listCourses({});
    if (!res.isOk()) return;

    const auto& list = res.value();
    m_coursesTable->setRowCount(static_cast<int>(list.size()));

    for (int i = 0; i < static_cast<int>(list.size()); ++i) {
        const auto& c = list[i];
        auto* itemName = new QTableWidgetItem(QString::fromStdString(c.name));
        itemName->setData(Qt::UserRole, QVariant::fromValue(static_cast<qint64>(c.id)));
        itemName->setFont(QFont("Segoe UI", 9, QFont::Bold));

        auto* itemDur = new QTableWidgetItem(QString("%1 %2").arg(c.durationWeeks).arg(isRtl ? "أسبوع" : "weeks"));

        domain::Money fee(c.defaultFeePiastres);
        auto* itemFee = new QTableWidgetItem(QString::fromStdString(fee.format()));

        auto* itemStatus = new QTableWidgetItem(c.status == "active" ? (isRtl ? "نشط" : "Active") : (isRtl ? "معطل" : "Inactive"));

        m_coursesTable->setItem(i, 0, itemName);
        m_coursesTable->setItem(i, 1, itemDur);
        m_coursesTable->setItem(i, 2, itemFee);
        m_coursesTable->setItem(i, 3, itemStatus);
    }
}

void CoursesView::loadGroups() {
    bool isRtl = LocalizationManager::instance().isRtl();
    auto res = m_appContext->groupUseCases()->listGroups({});
    if (!res.isOk()) return;

    const auto& list = res.value();
    m_groupsTable->setRowCount(static_cast<int>(list.size()));

    for (int i = 0; i < static_cast<int>(list.size()); ++i) {
        const auto& g = list[i];
        auto* itemName = new QTableWidgetItem(QString::fromStdString(g.name));
        itemName->setData(Qt::UserRole, QVariant::fromValue(static_cast<qint64>(g.id)));
        itemName->setFont(QFont("Segoe UI", 9, QFont::Bold));

        auto* itemCourse = new QTableWidgetItem(QString::fromStdString(g.courseName));
        auto* itemInst = new QTableWidgetItem(QString::fromStdString(g.instructorName).isEmpty() ? "-" : QString::fromStdString(g.instructorName));
        auto* itemSched = new QTableWidgetItem(QString::fromStdString(g.scheduleDescription));

        QString capStr = (g.capacity > 0) ? QString("%1 / %2").arg(g.enrolledCount).arg(g.capacity) : QString::number(g.enrolledCount);
        auto* itemCap = new QTableWidgetItem(capStr);

        domain::Money fee(g.feePiastres);
        auto* itemFee = new QTableWidgetItem(QString::fromStdString(fee.format()));

        QString stStr = QString::fromStdString(g.status);
        if (g.status == "open") stStr = isRtl ? "مفتوحة" : "Open";
        else if (g.status == "full") stStr = isRtl ? "مكتملة" : "Full";
        else if (g.status == "closed") stStr = isRtl ? "مغلقة" : "Closed";
        else if (g.status == "cancelled") stStr = isRtl ? "ملغاة" : "Cancelled";
        auto* itemStatus = new QTableWidgetItem(stStr);

        m_groupsTable->setItem(i, 0, itemName);
        m_groupsTable->setItem(i, 1, itemCourse);
        m_groupsTable->setItem(i, 2, itemInst);
        m_groupsTable->setItem(i, 3, itemSched);
        m_groupsTable->setItem(i, 4, itemCap);
        m_groupsTable->setItem(i, 5, itemFee);
        m_groupsTable->setItem(i, 6, itemStatus);
    }
}

void CoursesView::loadInstructors() {
    bool isRtl = LocalizationManager::instance().isRtl();
    auto res = m_appContext->payrollUseCases()->listInstructors();
    if (!res.isOk()) return;

    const auto& list = res.value();
    m_instructorsTable->setRowCount(static_cast<int>(list.size()));

    for (int i = 0; i < static_cast<int>(list.size()); ++i) {
        const auto& inst = list[i];
        auto* itemName = new QTableWidgetItem(QString::fromStdString(inst.fullName));
        itemName->setData(Qt::UserRole, QVariant::fromValue(static_cast<qint64>(inst.id)));
        itemName->setFont(QFont("Segoe UI", 9, QFont::Bold));

        auto* itemPhone = new QTableWidgetItem(QString::fromStdString(inst.phone));
        auto* itemSpec = new QTableWidgetItem(QString::fromStdString(inst.specialization));

        QString compStr = isRtl ? "بالساعة" : "Hourly";
        if (inst.compensationType == "per_session") compStr = isRtl ? "بالحصة" : "Per Session";
        else if (inst.compensationType == "monthly_salary") compStr = isRtl ? "راتب شهري" : "Monthly Salary";
        else if (inst.compensationType == "fixed_contract") compStr = isRtl ? "عقد مقطوع" : "Fixed Contract";
        auto* itemComp = new QTableWidgetItem(compStr);

        std::int64_t rateVal = (inst.compensationType == "monthly_salary") ? inst.monthlySalaryPiastres : inst.baseRatePiastres;
        domain::Money rate(rateVal);
        auto* itemRate = new QTableWidgetItem(QString::fromStdString(rate.format()));

        auto* itemStatus = new QTableWidgetItem(inst.status == "active" ? (isRtl ? "نشط" : "Active") : (isRtl ? "معطل" : "Inactive"));

        m_instructorsTable->setItem(i, 0, itemName);
        m_instructorsTable->setItem(i, 1, itemPhone);
        m_instructorsTable->setItem(i, 2, itemSpec);
        m_instructorsTable->setItem(i, 3, itemComp);
        m_instructorsTable->setItem(i, 4, itemRate);
        m_instructorsTable->setItem(i, 5, itemStatus);
    }
}

void CoursesView::onAddCourseClicked() {
    CourseFormDialog dlg(m_appContext, std::nullopt, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadCourses();
    }
}

void CoursesView::onEditCourseClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_coursesTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد كورس من الجدول." : "Please select a course to edit.");
        return;
    }
    std::int64_t courseId = m_coursesTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    CourseFormDialog dlg(m_appContext, courseId, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadCourses();
    }
}

void CoursesView::onAddGroupClicked() {
    GroupFormDialog dlg(m_appContext, std::nullopt, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadGroups();
    }
}

void CoursesView::onEditGroupClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_groupsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد مجموعة من الجدول." : "Please select a group to edit.");
        return;
    }
    std::int64_t groupId = m_groupsTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    GroupFormDialog dlg(m_appContext, groupId, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadGroups();
    }
}

void CoursesView::onEnrollInGroupClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_groupsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد مجموعة من الجدول لتسجيل الطالب بها." : "Please select a group to enroll student into.");
        return;
    }
    std::int64_t groupId = m_groupsTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    EnrollmentDialog dlg(m_appContext, std::nullopt, groupId, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadGroups();
    }
}

void CoursesView::onAddInstructorClicked() {
    InstructorFormDialog dlg(m_appContext, std::nullopt, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadInstructors();
    }
}

void CoursesView::onEditInstructorClicked() {
    bool isRtl = LocalizationManager::instance().isRtl();
    int row = m_instructorsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, isRtl ? "تنبيه" : "Selection Required",
                             isRtl ? "يرجى تحديد محاضر من الجدول." : "Please select an instructor to edit.");
        return;
    }
    std::int64_t instId = m_instructorsTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    InstructorFormDialog dlg(m_appContext, instId, this);
    if (dlg.exec() == QDialog::Accepted) {
        loadInstructors();
    }
}

} // namespace pixcode::desktop
