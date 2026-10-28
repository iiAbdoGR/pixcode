set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)

add_executable(academy_desktop
    main.cpp
    app/ApplicationContext.cpp
    localization/LocalizationManager.cpp
    styles/Theme.cpp
    dialogs/FirstRunWizardDialog.cpp
    dialogs/LoginDialog.cpp
    dialogs/ChangePasswordDialog.cpp
    dialogs/StudentFormDialog.cpp
    dialogs/StudentDetailDialog.cpp
    dialogs/CourseFormDialog.cpp
    dialogs/GroupFormDialog.cpp
    dialogs/EnrollmentDialog.cpp
    dialogs/PaymentDialog.cpp
    dialogs/StaffFormDialog.cpp
    dialogs/ResetPasswordDialog.cpp
    dialogs/ScheduleSessionDialog.cpp
    dialogs/InstructorPayoutDialog.cpp
    navigation/NavigationSidebar.cpp
    views/MainWindow.cpp
    views/DashboardView.cpp
    views/UsersView.cpp
    views/AuditView.cpp
    views/StudentsView.cpp
    views/CoursesView.cpp
    views/RegistrationView.cpp
    views/FinanceView.cpp
    views/AttendanceView.cpp
    views/PayrollView.cpp
)

target_link_libraries(academy_desktop PRIVATE
    academy_application
    academy_infrastructure
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::PrintSupport
)

target_include_directories(academy_desktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
