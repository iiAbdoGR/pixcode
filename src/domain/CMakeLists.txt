add_library(academy_domain STATIC
    entities/User.cpp
    value_objects/Money.cpp
)

target_include_directories(academy_domain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
