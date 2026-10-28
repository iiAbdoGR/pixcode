#include "CoursesView.h"
#include "../dialogs/CourseFormDialog.h"
#include "../dialogs/GroupFormDialog.h"
#include "../dialogs/EnrollmentDialog.h"
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
    gLayout->addWidget(m_groupsTable);

    m_tabs->addTab(m_coursesTab, "الكورسات");
    m_tabs->addTab(m_groupsTab, "المجموعات الدراسية");

    mainLayout->addWidget(m_tabs);
}

void CoursesView::updateTranslations() {
    bool isRtl = LocalizationManager::instance().isRtl();

    m_titleLabel->setText(isRtl ? "إدارة الكورسات والمجموعات الدراسية" : "Course & Group Management");
    m_tabs->setTabText(0, isRtl ? "الكورسات (قوالب الدورات)" : "Courses (Templates)");
    m_tabs->setTabText(1, isRtl ? "المجموعات الدراسية (الفصول الفعلية)" : "Class Groups (Scheduled)");

    m_addCourseBtn->setText(isRtl ? "+ كورس جديد" : "+ Add Course");
    m_editCourseBtn->setText(isRtl ? "تعديل الكورس" : "Edit Course");
    m_refreshCoursesBtn->setText(isRtl ? "تحديث" : "Refresh");

    m_coursesTable->setHorizontalHeaderLabels({
        isRtl ? "اسم الكورس" : "Course Name",
        isRtl ? "المدة المقترحة" : "Duration",
        isRtl ? "السعر الافتراضي" : "Default Fee",
        isRtl ? "الوصف" : "Description"
    });

    m_addGroupBtn->setText(isRtl ? "+ إنشاء مجموعة جديدة" : "+ Create New Group");
    m_editGroupBtn->setText(isRtl ? "تعديل المجموعة" : "Edit Group");
    m_enrollInGroupBtn->setText(isRtl ? "تسجيل طالب في هذه المجموعة" : "Enroll Student in Group");
    m_refreshGroupsBtn->setText(isRtl ? "تحديث" : "Refresh");

    m_groupsTable->setHorizontalHeaderLabels({
        isRtl ? "اسم المجموعة" : "Group Name",
        isRtl ? "الكورس" : "Course",
        isRtl ? "المدرب" : "Instructor",
        isRtl ? "المواعيد" : "Schedule",
        isRtl ? "المسجلين / السعة" : "Enrolled / Cap",
        isRtl ? "رسوم المجموعة" : "Group Fee",
        isRtl ? "الحالة" : "Status"
    });
}

void CoursesView::refresh() {
    loadCourses();
    loadGroups();
}

void CoursesView::loadCourses() {
    application::CourseFilter filter;
    filter.pageSize = 200;
    auto res = m_appContext->courseUseCases()->listCourses(filter);
    if (!res.isOk()) return;

    const auto& courses = res.value();
    m_coursesTable->setRowCount(static_cast<int>(courses.size()));

    for (int i = 0; i < static_cast<int>(courses.size()); ++i) {
        const auto& c = courses[i];

        auto* itemName = new QTableWidgetItem(QString::fromStdString(c.name));
        itemName->setData(Qt::UserRole, QVariant::fromValue(static_cast<qint64>(c.id)));
        itemName->setFont(QFont("Segoe UI", 9, QFont::Bold));

        auto* itemDur = new QTableWidgetItem(QString::number(c.durationWeeks) + " weeks");
        domain::Money fee(c.defaultFeePiastres);
        auto* itemFee = new QTableWidgetItem(QString::fromStdString(fee.format()));
        auto* itemDesc = new QTableWidgetItem(QString::fromStdString(c.description));

        m_coursesTable->setItem(i, 0, itemName);
        m_coursesTable->setItem(i, 1, itemDur);
        m_coursesTable->setItem(i, 2, itemFee);
        m_coursesTable->setItem(i, 3, itemDesc);
    }
}

void CoursesView::loadGroups() {
    bool isRtl = LocalizationManager::instance().isRtl();
    application::GroupFilter filter;
    filter.pageSize = 200;
    auto res = m_appContext->groupUseCases()->listGroups(filter);
    if (!res.isOk()) return;

    const auto& groups = res.value();
    m_groupsTable->setRowCount(static_cast<int>(groups.size()));

    for (int i = 0; i < static_cast<int>(groups.size()); ++i) {
        const auto& g = groups[i];

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

} // namespace pixcode::desktop

