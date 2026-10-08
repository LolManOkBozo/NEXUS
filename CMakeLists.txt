cmake_minimum_required(VERSION 3.16)

project(NEXUS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Sql)
find_package(Qt6 COMPONENTS Test)  # Optional, but we will use it if found

add_executable(NEXUS
    src/main.cpp
    src/core/application.h
    src/core/application.cpp
    src/ui/main_window.h
    src/ui/main_window.cpp
    src/ui/pages/dashboard_page.h
    src/ui/pages/dashboard_page.cpp
    src/ui/pages/subjects_page.h
    src/ui/pages/subjects_page.cpp
    src/ui/pages/timetable_page.h
    src/ui/pages/timetable_page.cpp
    src/ui/pages/assignments_page.h
    src/ui/pages/assignments_page.cpp
    src/ui/pages/attendance_page.h
    src/ui/pages/attendance_page.cpp
    src/ui/pages/exams_page.h
    src/ui/pages/exams_page.cpp
    src/ui/pages/notes_page.h
    src/ui/pages/notes_page.cpp
    src/ui/pages/ai_tutor_page.h
    src/ui/pages/ai_tutor_page.cpp
    src/ui/pages/analytics_page.h
    src/ui/pages/analytics_page.cpp
    src/ui/pages/settings_page.h
    src/ui/pages/settings_page.cpp
    src/core/database_manager.h
    src/core/database_manager.cpp
    resources/resources.qrc
)

target_link_libraries(NEXUS
    PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
        Qt6::Sql
)

if(WIN32)
    find_program(QT_WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${Qt6_DIR}/../../../bin"
    )
    if(NOT QT_WINDEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR "Could not find windeployqt in the selected Qt installation.")
    endif()

    add_custom_command(TARGET NEXUS POST_BUILD
        COMMAND "${QT_WINDEPLOYQT_EXECUTABLE}"
            --no-translations
            --compiler-runtime
            "$<TARGET_FILE:NEXUS>"
        COMMENT "Deploying Qt runtime dependencies"
        VERBATIM
    )
endif()

# Database tests
if(TARGET Qt6::Test)
    add_executable(nexus_database_tests
        tests/database_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
    )

    target_link_libraries(nexus_database_tests
        PRIVATE
            Qt6::Core
            Qt6::Sql
            Qt6::Test
    )

    enable_testing()
    add_test(NAME DatabaseTest COMMAND nexus_database_tests)
endif()
