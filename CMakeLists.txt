cmake_minimum_required(VERSION 3.20)
project(PixCodeAcademy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Disable AUTOUIC as all UI is constructed in pure C++
set(CMAKE_AUTOUIC OFF)

# Enable compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find Qt6 Components
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
    Test
    PrintSupport
)

# External Libraries (libsodium, sqlite3)
find_package(PkgConfig QUIET)
find_path(SODIUM_INCLUDE_DIR sodium.h PATHS "C:/msys64/ucrt64/include")
find_library(SODIUM_LIBRARY NAMES sodium libsodium PATHS "C:/msys64/ucrt64/lib")

find_path(SQLITE3_INCLUDE_DIR sqlite3.h PATHS "C:/msys64/ucrt64/include")
find_library(SQLITE3_LIBRARY NAMES sqlite3 libsqlite3 PATHS "C:/msys64/ucrt64/lib")

if(NOT SODIUM_LIBRARY)
    message(FATAL_ERROR "libsodium library not found!")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SODIUM_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}
)

# Subtargets
add_subdirectory(src/domain)
add_subdirectory(src/application)
add_subdirectory(src/infrastructure)
add_subdirectory(src/desktop)

# Unit & Integration Tests
enable_testing()
add_subdirectory(tests)
