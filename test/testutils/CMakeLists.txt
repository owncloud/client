add_executable(test_helper test_helper.cpp)
apply_common_target_settings(test_helper)
target_link_libraries(test_helper PUBLIC Qt::Core libsync)

add_library(syncenginetestutils STATIC syncenginetestutils.cpp testutils.cpp)
target_link_libraries(syncenginetestutils PUBLIC owncloudGui Qt::Test)
target_compile_definitions(syncenginetestutils PRIVATE TEST_HELPER_EXE="$<TARGET_FILE:test_helper>")
apply_common_target_settings(syncenginetestutils)

# testutilsloader.cpp uses Q_COREAPP_STARTUP_FUNCTION which can't used reliably in a static lib
# therefore we compile it in the tests
add_library(testutilsloader OBJECT testutilsloader.cpp)
apply_common_target_settings(testutilsloader)
# target_compile_options(testutilsloader PRIVATE /fsanitize=address)
target_link_libraries(testutilsloader PUBLIC owncloudGui owncloudResources)
