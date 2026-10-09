cmake_minimum_required(VERSION 3.16)

project(NEXUS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Sql)
find_package(Qt6 COMPONENTS Test QUIET)

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
    src/data/subject_dao.h
    src/data/subject_dao.cpp
    src/models/subject.h
    src/models/subject.cpp
    src/services/subject_service.h
    src/services/subject_service.cpp
    src/ui/dialogs/subject_dialog.h
    src/ui/dialogs/subject_dialog.cpp
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

    function(nexus_deploy_qt_runtime target_name)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND "${QT_WINDEPLOYQT_EXECUTABLE}"
                --no-translations
                --compiler-runtime
                "$<TARGET_FILE:${target_name}>"
            COMMENT "Deploying Qt runtime dependencies for ${target_name}"
            VERBATIM
        )
    endfunction()

    nexus_deploy_qt_runtime(NEXUS)
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
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_database_tests)
    endif()

    enable_testing()
    add_test(NAME DatabaseTests COMMAND nexus_database_tests)

    add_executable(nexus_subject_tests
        tests/subject_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/models/subject.cpp
        src/models/subject.h
        src/services/subject_service.cpp
        src/services/subject_service.h
    )
    target_link_libraries(nexus_subject_tests
        PRIVATE
            Qt6::Core
            Qt6::Sql
            Qt6::Test
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_subject_tests)
    endif()
    add_test(NAME SubjectTests COMMAND nexus_subject_tests)

    add_executable(nexus_subject_ui_tests
        tests/subjects_ui_tests.cpp
        src/core/application.cpp
        src/core/application.h
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/models/subject.cpp
        src/models/subject.h
        src/services/subject_service.cpp
        src/services/subject_service.h
        src/ui/main_window.cpp
        src/ui/main_window.h
        src/ui/dialogs/subject_dialog.cpp
        src/ui/dialogs/subject_dialog.h
        src/ui/pages/dashboard_page.cpp
        src/ui/pages/dashboard_page.h
        src/ui/pages/subjects_page.cpp
        src/ui/pages/subjects_page.h
        src/ui/pages/timetable_page.cpp
        src/ui/pages/timetable_page.h
        src/ui/pages/assignments_page.cpp
        src/ui/pages/assignments_page.h
        src/ui/pages/attendance_page.cpp
        src/ui/pages/attendance_page.h
        src/ui/pages/exams_page.cpp
        src/ui/pages/exams_page.h
        src/ui/pages/notes_page.cpp
        src/ui/pages/notes_page.h
        src/ui/pages/ai_tutor_page.cpp
        src/ui/pages/ai_tutor_page.h
        src/ui/pages/analytics_page.cpp
        src/ui/pages/analytics_page.h
        src/ui/pages/settings_page.cpp
        src/ui/pages/settings_page.h
        resources/resources.qrc
    )
    target_link_libraries(nexus_subject_ui_tests
        PRIVATE
            Qt6::Widgets
            Qt6::Core
            Qt6::Gui
            Qt6::Sql
            Qt6::Test
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_subject_ui_tests)
    endif()
    add_test(NAME SubjectUiTests COMMAND nexus_subject_ui_tests)
    get_filename_component(QT_PLATFORM_PLUGIN_DIR
        "${Qt6_DIR}/../../../plugins/platforms" ABSOLUTE)
    set_tests_properties(SubjectUiTests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLATFORM_PLUGIN_DIR}"
    )
endif()
