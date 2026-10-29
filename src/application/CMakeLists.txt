add_library(academy_application STATIC
    authorization/AuthorizationContext.cpp
    use_cases/AuthenticateUserUseCase.cpp
    use_cases/FirstRunSetupUseCase.cpp
    use_cases/ChangePasswordUseCase.cpp
    use_cases/AuditService.cpp
    use_cases/StudentUseCases.cpp
    use_cases/CourseUseCases.cpp
    use_cases/GroupUseCases.cpp
    use_cases/EnrollmentUseCases.cpp
    use_cases/StaffManagementUseCases.cpp
    use_cases/AttendanceUseCases.cpp
    use_cases/InstructorPayrollUseCases.cpp
    use_cases/DashboardUseCases.cpp
)

target_link_libraries(academy_application PUBLIC
    academy_domain
)

target_include_directories(academy_application PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
