add_library(academy_infrastructure STATIC
    database/DatabaseManager.cpp
    migrations/MigrationRunner.cpp
    security/Argon2idPasswordHasher.cpp
    system/SystemClock.cpp
    repositories/SqliteUserRepository.cpp
    repositories/SqliteRoleRepository.cpp
    repositories/SqliteAuditLogRepository.cpp
    repositories/SqliteStudentRepository.cpp
    repositories/SqliteCourseRepository.cpp
    repositories/SqliteInstructorRepository.cpp
    repositories/SqliteGroupRepository.cpp
    repositories/SqliteEnrollmentRepository.cpp
    repositories/SqlitePaymentRepository.cpp
    logging/AppLogger.cpp
)

target_link_libraries(academy_infrastructure PUBLIC
    academy_application
    Qt6::Core
    Qt6::Sql
    ${SODIUM_LIBRARY}
    ${SQLITE3_LIBRARY}
)

target_include_directories(academy_infrastructure PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${SODIUM_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}
)
