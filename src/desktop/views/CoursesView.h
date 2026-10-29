#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QLabel>
#include <memory>
#include "../app/ApplicationContext.h"

namespace pixcode::desktop {

class CoursesView : public QWidget {
    Q_OBJECT

public:
    explicit CoursesView(std::shared_ptr<ApplicationContext> appContext, QWidget* parent = nullptr);

    void refresh();
    void selectCoursesTab();
    void selectGroupsTab();
    void selectInstructorsTab();

private slots:
    void onAddCourseClicked();
    void onEditCourseClicked();
    void onAddGroupClicked();
    void onEditGroupClicked();
    void onEnrollInGroupClicked();
    void onAddInstructorClicked();
    void onEditInstructorClicked();

private:
    void setupUi();
    void updateTranslations();
    void loadCourses();
    void loadGroups();
    void loadInstructors();

    std::shared_ptr<ApplicationContext> m_appContext;

    QLabel* m_titleLabel;
    QTabWidget* m_tabs;

    // Courses tab
    QWidget* m_coursesTab;
    QTableWidget* m_coursesTable;
    QPushButton* m_addCourseBtn;
    QPushButton* m_editCourseBtn;
    QPushButton* m_refreshCoursesBtn;

    // Groups tab
    QWidget* m_groupsTab;
    QTableWidget* m_groupsTable;
    QPushButton* m_addGroupBtn;
    QPushButton* m_editGroupBtn;
    QPushButton* m_enrollInGroupBtn;
    QPushButton* m_refreshGroupsBtn;

    // Instructors tab
    QWidget* m_instructorsTab;
    QTableWidget* m_instructorsTable;
    QPushButton* m_addInstructorBtn;
    QPushButton* m_editInstructorBtn;
    QPushButton* m_refreshInstructorsBtn;
};

} // namespace pixcode::desktop
