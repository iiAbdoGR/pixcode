set(CMAKE_AUTOMOC ON)

add_executable(academy_tests
    TestRunner.cpp
    domain/PasswordHasherTests.cpp
    domain/UserEntityTests.cpp
    domain/MoneyTests.cpp
    domain/AcademicDomainTests.cpp
    domain/AttendanceAndPayrollTests.cpp
    application/AuthorizationTests.cpp
    infrastructure/DatabaseMigrationTests.cpp
    infrastructure/AuditLogTests.cpp
)

target_link_libraries(academy_tests PRIVATE
    academy_domain
    academy_application
    academy_infrastructure
    Qt6::Core
    Qt6::Sql
    Qt6::Test
    ${SODIUM_LIBRARY}
    ${SQLITE3_LIBRARY}
)

target_include_directories(academy_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${SODIUM_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}
)

add_test(NAME AcademyTests COMMAND academy_tests)
