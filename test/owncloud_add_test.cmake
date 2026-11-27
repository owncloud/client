include(OCApplyCommonSettings)
find_package(Qt6 COMPONENTS Test REQUIRED)

include(ECMAddTests)

function(owncloud_add_test test_class)
    set(OWNCLOUD_TEST_CLASS ${test_class})
    string(TOLOWER "${OWNCLOUD_TEST_CLASS}" OWNCLOUD_TEST_CLASS_LOWERCASE)
    set(SRC_PATH test${OWNCLOUD_TEST_CLASS_LOWERCASE}.cpp)
    if (IS_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/test${OWNCLOUD_TEST_CLASS_LOWERCASE}/)
        set(SRC_PATH test${OWNCLOUD_TEST_CLASS_LOWERCASE}/${SRC_PATH})
    endif()

    ecm_add_test(${SRC_PATH}
        ${ARGN}
        TEST_NAME "${OWNCLOUD_TEST_CLASS}Test"
        LINK_LIBRARIES
        owncloudGui syncenginetestutils testutilsloader Qt::Test
    )
    apply_common_target_settings(${OWNCLOUD_TEST_CLASS}Test)
    target_compile_definitions(${OWNCLOUD_TEST_CLASS}Test PRIVATE SOURCEDIR="${PROJECT_SOURCE_DIR}" QT_FORCE_ASSERTS)

    target_include_directories(${OWNCLOUD_TEST_CLASS}Test PRIVATE "${CMAKE_SOURCE_DIR}/test/")
    if (UNIX AND NOT APPLE)
        set_property(TEST ${OWNCLOUD_TEST_CLASS}Test PROPERTY ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
    if (WIN32)
        # Without a console window, as on CI, QtTest on Windows sends its results to
        # OutputDebugString and ctest captures none of them. Despite its name, the variable keeps
        # them on stdout, as on the other platforms.
        set_property(TEST ${OWNCLOUD_TEST_CLASS}Test APPEND PROPERTY ENVIRONMENT "QT_FORCE_STDERR_LOGGING=1")
    endif()

    foreach(arg IN LISTS ARGN)
        target_sources(${OWNCLOUD_TEST_CLASS}Test PRIVATE ${arg})
    endforeach()
endfunction()
