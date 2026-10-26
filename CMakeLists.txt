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
    src/core/theme_manager.h
    src/core/theme_manager.cpp
    src/core/attendance_duplicate_recovery.h
    src/core/attendance_duplicate_recovery.cpp
    src/core/attendance_uniqueness_migration.h
    src/data/subject_dao.h
    src/data/subject_dao.cpp
    src/data/assignment_dao.h
    src/data/assignment_dao.cpp
    src/data/timetable_dao.h
    src/data/timetable_dao.cpp
    src/data/attendance_dao.h
    src/data/attendance_dao.cpp
    src/data/exam_dao.h
    src/data/exam_dao.cpp
    src/data/notes_dao.h
    src/data/notes_dao.cpp
    src/models/subject.h
    src/models/subject.cpp
    src/models/assignment.h
    src/models/assignment.cpp
    src/models/attendance.h
    src/models/attendance.cpp
    src/models/exam.h
    src/models/exam.cpp
    src/models/note.h
    src/models/note.cpp
    src/models/timetable.h
    src/models/timetable.cpp
    src/services/subject_service.h
    src/services/subject_service.cpp
    src/services/assignment_service.h
    src/services/assignment_service.cpp
    src/services/attendance_service.h
    src/services/attendance_service.cpp
    src/services/exam_service.h
    src/services/exam_service.cpp
    src/services/notes_service.h
    src/services/notes_service.cpp
    src/services/timetable_service.h
    src/services/timetable_service.cpp
    src/ui/dialogs/subject_dialog.h
    src/ui/dialogs/subject_dialog.cpp
    src/ui/dialogs/timetable_entry_dialog.h
    src/ui/dialogs/timetable_entry_dialog.cpp
    src/ui/dialogs/assignment_dialog.h
    src/ui/dialogs/assignment_dialog.cpp
    src/ui/dialogs/exam_dialog.h
    src/ui/dialogs/exam_dialog.cpp
    src/ui/dialogs/note_dialog.h
    src/ui/dialogs/note_dialog.cpp
    src/ui/dialogs/attendance_duplicate_recovery_dialog.h
    src/ui/dialogs/attendance_duplicate_recovery_dialog.cpp
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

    add_executable(nexus_attendance_model_tests
        tests/attendance_model_tests.cpp
        src/models/attendance.cpp
        src/models/attendance.h
    )
    target_link_libraries(nexus_attendance_model_tests
        PRIVATE
            Qt6::Core
            Qt6::Test
    )
    set_target_properties(nexus_attendance_model_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_attendance_model_tests)
    endif()
    add_test(NAME AttendanceModelTests COMMAND nexus_attendance_model_tests)

    add_executable(nexus_attendance_dao_tests
        tests/attendance_dao_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/attendance_dao.cpp
        src/data/attendance_dao.h
        src/models/attendance.cpp
        src/models/attendance.h
    )
    target_link_libraries(nexus_attendance_dao_tests
        PRIVATE
            Qt6::Core
            Qt6::Sql
            Qt6::Test
    )
    set_target_properties(nexus_attendance_dao_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_attendance_dao_tests)
    endif()
    add_test(NAME AttendanceDAOTests COMMAND nexus_attendance_dao_tests)

    add_executable(nexus_attendance_migration_tests
        tests/attendance_migration_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
    )
    target_link_libraries(nexus_attendance_migration_tests
        PRIVATE
            Qt6::Core
            Qt6::Sql
            Qt6::Test
    )
    set_target_properties(nexus_attendance_migration_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_attendance_migration_tests)
    endif()
    add_test(NAME AttendanceMigrationTests COMMAND nexus_attendance_migration_tests)

    add_executable(nexus_attendance_duplicate_recovery_tests
        tests/attendance_duplicate_recovery_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/core/attendance_duplicate_recovery.cpp
        src/core/attendance_duplicate_recovery.h
        src/core/attendance_uniqueness_migration.h
        src/models/attendance.cpp
        src/models/attendance.h
    )
    target_link_libraries(nexus_attendance_duplicate_recovery_tests
        PRIVATE
            Qt6::Core
            Qt6::Sql
            Qt6::Test
    )
    set_target_properties(nexus_attendance_duplicate_recovery_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_attendance_duplicate_recovery_tests)
    endif()
    add_test(NAME AttendanceDuplicateRecoveryTests
        COMMAND nexus_attendance_duplicate_recovery_tests)

    add_executable(nexus_attendance_service_tests
        tests/attendance_service_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/attendance_dao.cpp
        src/data/attendance_dao.h
        src/models/attendance.cpp
        src/models/attendance.h
        src/services/attendance_service.cpp
        src/services/attendance_service.h
    )
    target_link_libraries(nexus_attendance_service_tests
        PRIVATE
            Qt6::Core
            Qt6::Sql
            Qt6::Test
    )
    set_target_properties(nexus_attendance_service_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_attendance_service_tests)
    endif()
    add_test(NAME AttendanceServiceTests COMMAND nexus_attendance_service_tests)

    add_executable(nexus_attendance_ui_tests
        tests/attendance_ui_tests.cpp
        src/core/application.cpp
        src/core/application.h
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/attendance_dao.cpp
        src/data/attendance_dao.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/models/attendance.cpp
        src/models/attendance.h
        src/models/subject.cpp
        src/models/subject.h
        src/services/attendance_service.cpp
        src/services/attendance_service.h
        src/services/subject_service.cpp
        src/services/subject_service.h
        src/ui/pages/attendance_page.cpp
        src/ui/pages/attendance_page.h
    )
    target_link_libraries(nexus_attendance_ui_tests
        PRIVATE
            Qt6::Widgets
            Qt6::Core
            Qt6::Gui
            Qt6::Sql
            Qt6::Test
    )
    set_target_properties(nexus_attendance_ui_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_attendance_ui_tests)
    endif()
    add_test(NAME AttendanceUiTests COMMAND nexus_attendance_ui_tests)

    add_executable(nexus_attendance_duplicate_recovery_ui_tests
        tests/attendance_duplicate_recovery_ui_tests.cpp
        src/core/attendance_duplicate_recovery.cpp
        src/core/attendance_duplicate_recovery.h
        src/core/attendance_uniqueness_migration.h
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/models/attendance.cpp
        src/models/attendance.h
        src/ui/dialogs/attendance_duplicate_recovery_dialog.cpp
        src/ui/dialogs/attendance_duplicate_recovery_dialog.h
    )
    target_link_libraries(nexus_attendance_duplicate_recovery_ui_tests
        PRIVATE
            Qt6::Widgets
            Qt6::Core
            Qt6::Gui
            Qt6::Sql
            Qt6::Test
    )
    set_target_properties(nexus_attendance_duplicate_recovery_ui_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_attendance_duplicate_recovery_ui_tests)
    endif()
    add_test(NAME AttendanceDuplicateRecoveryUiTests
        COMMAND nexus_attendance_duplicate_recovery_ui_tests)

    add_executable(nexus_subject_ui_tests
        tests/subjects_ui_tests.cpp
        src/core/application.cpp
        src/core/application.h
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/data/assignment_dao.cpp
        src/data/assignment_dao.h
        src/data/timetable_dao.cpp
        src/data/timetable_dao.h
        src/data/attendance_dao.cpp
        src/data/attendance_dao.h
        src/data/exam_dao.cpp
        src/data/exam_dao.h
        src/data/notes_dao.cpp
        src/data/notes_dao.h
        src/models/assignment.cpp
        src/models/assignment.h
        src/models/attendance.cpp
        src/models/attendance.h
        src/models/exam.cpp
        src/models/exam.h
        src/models/note.cpp
        src/models/note.h
        src/models/subject.cpp
        src/models/subject.h
        src/models/timetable.cpp
        src/models/timetable.h
        src/services/subject_service.cpp
        src/services/subject_service.h
        src/services/assignment_service.cpp
        src/services/assignment_service.h
        src/services/timetable_service.cpp
        src/services/timetable_service.h
        src/services/attendance_service.cpp
        src/services/attendance_service.h
        src/services/exam_service.cpp
        src/services/exam_service.h
        src/services/notes_service.cpp
        src/services/notes_service.h
        src/ui/main_window.cpp
        src/ui/main_window.h
        src/ui/dialogs/subject_dialog.cpp
        src/ui/dialogs/subject_dialog.h
        src/ui/dialogs/timetable_entry_dialog.cpp
        src/ui/dialogs/timetable_entry_dialog.h
        src/ui/dialogs/assignment_dialog.cpp
        src/ui/dialogs/assignment_dialog.h
        src/ui/dialogs/exam_dialog.cpp
        src/ui/dialogs/exam_dialog.h
        src/ui/dialogs/note_dialog.cpp
        src/ui/dialogs/note_dialog.h
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
        src/core/theme_manager.h
        src/core/theme_manager.cpp
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
    set_tests_properties(AttendanceUiTests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLATFORM_PLUGIN_DIR}"
    )
    set_tests_properties(AttendanceDuplicateRecoveryUiTests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLATFORM_PLUGIN_DIR}"
    )

    add_executable(nexus_timetable_ui_tests
        tests/timetable_ui_tests.cpp
        src/core/application.cpp
        src/core/application.h
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/data/timetable_dao.cpp
        src/data/timetable_dao.h
        src/models/subject.cpp
        src/models/subject.h
        src/models/timetable.cpp
        src/models/timetable.h
        src/services/subject_service.cpp
        src/services/subject_service.h
        src/services/timetable_service.cpp
        src/services/timetable_service.h
        src/ui/dialogs/timetable_entry_dialog.cpp
        src/ui/dialogs/timetable_entry_dialog.h
        src/ui/pages/timetable_page.cpp
        src/ui/pages/timetable_page.h
    )
    target_link_libraries(nexus_timetable_ui_tests
        PRIVATE
            Qt6::Widgets
            Qt6::Core
            Qt6::Gui
            Qt6::Sql
            Qt6::Test
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_timetable_ui_tests)
    endif()
    add_test(NAME TimetableUiTests COMMAND nexus_timetable_ui_tests)
    set_tests_properties(TimetableUiTests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLATFORM_PLUGIN_DIR}"
    )

    add_executable(nexus_timetable_tests
        tests/timetable_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/timetable_dao.cpp
        src/data/timetable_dao.h
        src/models/timetable.cpp
        src/models/timetable.h
    )
    target_link_libraries(nexus_timetable_tests
        PRIVATE
            Qt6::Core
            Qt6::Sql
            Qt6::Test
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_timetable_tests)
    endif()
    add_test(NAME TimetableTests COMMAND nexus_timetable_tests)

    add_executable(nexus_timetable_service_tests
        tests/timetable_service_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/timetable_dao.cpp
        src/data/timetable_dao.h
        src/models/timetable.cpp
        src/models/timetable.h
        src/services/timetable_service.cpp
        src/services/timetable_service.h
    )
    target_link_libraries(nexus_timetable_service_tests
        PRIVATE
            Qt6::Core
            Qt6::Sql
            Qt6::Test
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_timetable_service_tests)
    endif()
    add_test(NAME TimetableServiceTests COMMAND nexus_timetable_service_tests)

    add_executable(nexus_assignment_tests
        tests/assignment_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/assignment_dao.cpp
        src/data/assignment_dao.h
        src/models/assignment.cpp
        src/models/assignment.h
    )
    target_link_libraries(nexus_assignment_tests
        PRIVATE
            Qt6::Core
            Qt6::Sql
            Qt6::Test
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_assignment_tests)
    endif()
    add_test(NAME AssignmentTests COMMAND nexus_assignment_tests)

    add_executable(nexus_assignment_service_tests
        tests/assignment_service_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/assignment_dao.cpp
        src/data/assignment_dao.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/models/assignment.cpp
        src/models/assignment.h
        src/models/subject.cpp
        src/models/subject.h
        src/services/assignment_service.cpp
        src/services/assignment_service.h
    )
    target_link_libraries(nexus_assignment_service_tests
        PRIVATE
            Qt6::Core
            Qt6::Sql
            Qt6::Test
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_assignment_service_tests)
    endif()
    add_test(NAME AssignmentServiceTests COMMAND nexus_assignment_service_tests)

    add_executable(nexus_assignment_ui_tests
        tests/assignment_ui_tests.cpp
        src/core/application.cpp
        src/core/application.h
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/assignment_dao.cpp
        src/data/assignment_dao.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/models/assignment.cpp
        src/models/assignment.h
        src/models/subject.cpp
        src/models/subject.h
        src/services/assignment_service.cpp
        src/services/assignment_service.h
        src/services/subject_service.cpp
        src/services/subject_service.h
        src/ui/dialogs/assignment_dialog.cpp
        src/ui/dialogs/assignment_dialog.h
        src/ui/pages/assignments_page.cpp
        src/ui/pages/assignments_page.h
    )
    target_link_libraries(nexus_assignment_ui_tests
        PRIVATE
            Qt6::Widgets
            Qt6::Core
            Qt6::Gui
            Qt6::Sql
            Qt6::Test
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_assignment_ui_tests)
    endif()
    add_test(NAME AssignmentUiTests COMMAND nexus_assignment_ui_tests)
    set_tests_properties(AssignmentUiTests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLATFORM_PLUGIN_DIR}"
    )

    add_executable(nexus_dashboard_ui_tests
        tests/dashboard_ui_tests.cpp
        src/core/application.cpp
        src/core/application.h
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/assignment_dao.cpp
        src/data/assignment_dao.h
        src/data/attendance_dao.cpp
        src/data/attendance_dao.h
        src/data/exam_dao.cpp
        src/data/exam_dao.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/data/timetable_dao.cpp
        src/data/timetable_dao.h
        src/models/assignment.cpp
        src/models/assignment.h
        src/models/attendance.cpp
        src/models/attendance.h
        src/models/exam.cpp
        src/models/exam.h
        src/models/subject.cpp
        src/models/subject.h
        src/models/timetable.cpp
        src/models/timetable.h
        src/services/assignment_service.cpp
        src/services/assignment_service.h
        src/services/attendance_service.cpp
        src/services/attendance_service.h
        src/services/exam_service.cpp
        src/services/exam_service.h
        src/services/subject_service.cpp
        src/services/subject_service.h
        src/services/timetable_service.cpp
        src/services/timetable_service.h
        src/ui/pages/dashboard_page.cpp
        src/ui/pages/dashboard_page.h
    )
    target_link_libraries(nexus_dashboard_ui_tests
        PRIVATE
            Qt6::Widgets
            Qt6::Core
            Qt6::Gui
            Qt6::Sql
            Qt6::Test
    )
    set_target_properties(nexus_dashboard_ui_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_dashboard_ui_tests)
    endif()
    add_test(NAME DashboardUiTests COMMAND nexus_dashboard_ui_tests)
    set_tests_properties(DashboardUiTests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLATFORM_PLUGIN_DIR}"
    )

    add_executable(nexus_exam_model_tests
        tests/exam_model_tests.cpp
        src/models/exam.cpp
        src/models/exam.h
    )
    target_link_libraries(nexus_exam_model_tests
        PRIVATE Qt6::Core Qt6::Test
    )
    set_target_properties(nexus_exam_model_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_exam_model_tests)
    endif()
    add_test(NAME ExamModelTests COMMAND nexus_exam_model_tests)

    add_executable(nexus_exam_service_tests
        tests/exam_service_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/exam_dao.cpp
        src/data/exam_dao.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/models/exam.cpp
        src/models/exam.h
        src/models/subject.cpp
        src/models/subject.h
        src/services/exam_service.cpp
        src/services/exam_service.h
    )
    target_link_libraries(nexus_exam_service_tests
        PRIVATE Qt6::Core Qt6::Sql Qt6::Test
    )
    set_target_properties(nexus_exam_service_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_exam_service_tests)
    endif()
    add_test(NAME ExamServiceTests COMMAND nexus_exam_service_tests)

    add_executable(nexus_exam_ui_tests
        tests/exam_ui_tests.cpp
        src/core/application.cpp
        src/core/application.h
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/exam_dao.cpp
        src/data/exam_dao.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/models/exam.cpp
        src/models/exam.h
        src/models/subject.cpp
        src/models/subject.h
        src/services/exam_service.cpp
        src/services/exam_service.h
        src/services/subject_service.cpp
        src/services/subject_service.h
        src/ui/dialogs/exam_dialog.cpp
        src/ui/dialogs/exam_dialog.h
        src/ui/pages/exams_page.cpp
        src/ui/pages/exams_page.h
    )
    target_link_libraries(nexus_exam_ui_tests
        PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Sql Qt6::Test
    )
    set_target_properties(nexus_exam_ui_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_exam_ui_tests)
    endif()
    add_test(NAME ExamUiTests COMMAND nexus_exam_ui_tests)
    set_tests_properties(ExamUiTests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLATFORM_PLUGIN_DIR}"
    )

    add_executable(nexus_note_model_tests
        tests/note_model_tests.cpp
        src/models/note.cpp
        src/models/note.h
    )
    target_link_libraries(nexus_note_model_tests
        PRIVATE Qt6::Core Qt6::Test
    )
    set_target_properties(nexus_note_model_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_note_model_tests)
    endif()
    add_test(NAME NoteModelTests COMMAND nexus_note_model_tests)

    add_executable(nexus_notes_service_tests
        tests/notes_service_tests.cpp
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/notes_dao.cpp
        src/data/notes_dao.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/models/note.cpp
        src/models/note.h
        src/models/subject.cpp
        src/models/subject.h
        src/services/notes_service.cpp
        src/services/notes_service.h
        src/services/subject_service.cpp
        src/services/subject_service.h
    )
    target_link_libraries(nexus_notes_service_tests
        PRIVATE Qt6::Core Qt6::Sql Qt6::Test
    )
    set_target_properties(nexus_notes_service_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_notes_service_tests)
    endif()
    add_test(NAME NotesServiceTests COMMAND nexus_notes_service_tests)

    add_executable(nexus_notes_ui_tests
        tests/notes_ui_tests.cpp
        src/core/application.cpp
        src/core/application.h
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/notes_dao.cpp
        src/data/notes_dao.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/models/note.cpp
        src/models/note.h
        src/models/subject.cpp
        src/models/subject.h
        src/services/notes_service.cpp
        src/services/notes_service.h
        src/services/subject_service.cpp
        src/services/subject_service.h
        src/ui/dialogs/note_dialog.cpp
        src/ui/dialogs/note_dialog.h
        src/ui/pages/notes_page.cpp
        src/ui/pages/notes_page.h
    )
    target_link_libraries(nexus_notes_ui_tests
        PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Sql Qt6::Test
    )
    set_target_properties(nexus_notes_ui_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_notes_ui_tests)
    endif()
    add_test(NAME NotesUiTests COMMAND nexus_notes_ui_tests)
    set_tests_properties(NotesUiTests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLATFORM_PLUGIN_DIR}"
    )

    add_executable(nexus_analytics_ui_tests
        tests/analytics_ui_tests.cpp
        src/core/application.cpp
        src/core/application.h
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/data/subject_dao.cpp
        src/data/subject_dao.h
        src/data/assignment_dao.cpp
        src/data/assignment_dao.h
        src/data/attendance_dao.cpp
        src/data/attendance_dao.h
        src/data/exam_dao.cpp
        src/data/exam_dao.h
        src/models/subject.cpp
        src/models/subject.h
        src/models/assignment.cpp
        src/models/assignment.h
        src/models/attendance.cpp
        src/models/attendance.h
        src/models/exam.cpp
        src/models/exam.h
        src/services/subject_service.cpp
        src/services/subject_service.h
        src/services/assignment_service.cpp
        src/services/assignment_service.h
        src/services/attendance_service.cpp
        src/services/attendance_service.h
        src/services/exam_service.cpp
        src/services/exam_service.h
        src/ui/pages/analytics_page.cpp
        src/ui/pages/analytics_page.h
    )
    target_link_libraries(nexus_analytics_ui_tests
        PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Sql Qt6::Test
    )
    set_target_properties(nexus_analytics_ui_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_analytics_ui_tests)
    endif()
    add_test(NAME AnalyticsUiTests COMMAND nexus_analytics_ui_tests)
    set_tests_properties(AnalyticsUiTests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLATFORM_PLUGIN_DIR}"
    )

    add_executable(nexus_settings_ui_tests
        tests/settings_ui_tests.cpp
        src/core/application.cpp
        src/core/application.h
        src/core/database_manager.cpp
        src/core/database_manager.h
        src/core/theme_manager.h
        src/core/theme_manager.cpp
        src/ui/pages/settings_page.cpp
        src/ui/pages/settings_page.h
    )
    target_link_libraries(nexus_settings_ui_tests
        PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Sql Qt6::Test
    )
    set_target_properties(nexus_settings_ui_tests PROPERTIES
        AUTOMOC_COMPILER_PREDEFINES OFF
    )
    if(WIN32)
        nexus_deploy_qt_runtime(nexus_settings_ui_tests)
    endif()
    add_test(NAME SettingsUiTests COMMAND nexus_settings_ui_tests)
    set_tests_properties(SettingsUiTests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLATFORM_PLUGIN_DIR}"
    )
endif()
