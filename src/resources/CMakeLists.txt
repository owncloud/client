find_package(Qt5LinguistTools)

add_library(owncloudResources SHARED resourceloader.cpp client.qrc)
target_link_libraries(owncloudResources PUBLIC Qt5::Core)
set_target_properties(owncloudResources PROPERTIES
    OUTPUT_NAME "${APPLICATION_EXECUTABLE}_resources"
    AUTORCC ON
)
GENERATE_EXPORT_HEADER(owncloudResources
        EXPORT_MACRO_NAME OWNCLOUDRESOURCE_EXPORT
        EXPORT_FILE_NAME owncloudresource.h
        STATIC_DEFINE OWNCLOUD_BUILT_AS_STATIC
)

if(Qt5LinguistTools_FOUND)
    # Handle Translations, pick all client_* files from trans directory.
    file(GLOB client_translations ${CMAKE_SOURCE_DIR}/translations/client_*.ts)
    qt5_add_translation(client_compiled_translations ${client_translations})
    add_resources_to_target(
        TARGET owncloudResources
        PREFIX translations
        FILES "${client_compiled_translations}"
    )
endif()

generate_theme(owncloudResources)

install(TARGETS owncloudResources ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

