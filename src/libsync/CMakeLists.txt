find_package(LibreGraphAPI 1.0.4 REQUIRED)
set_package_properties(LibreGraphAPI PROPERTIES
        URL https://github.com/owncloud/libre-graph-api-cpp-qt-client.git
        DESCRIPTION "Libre Graph is a free API for cloud collaboration inspired by the MS Graph API."
        TYPE OPTIONAL
)

set(libsync_SRCS
    account.cpp
    capabilities.cpp
    cookiejar.cpp
    discovery.cpp
    discoveryphase.cpp
    filesystem.cpp
    httplogger.cpp
    jobqueue.cpp
    logger.cpp
    accessmanager.cpp
    configfile.cpp
    abstractnetworkjob.cpp
    networkjobs.cpp
    owncloudpropagator.cpp
    owncloudtheme.cpp
    platform.cpp
    progressdispatcher.cpp
    propagatorjobs.cpp
    propagatedownload.cpp
    propagateuploadcommon.cpp
    propagateuploadfile.cpp
    propagateuploadtus.cpp
    propagateremotedelete.cpp
    propagateremotemove.cpp
    propagateremotemkdir.cpp
    putfilejob.cpp
    syncengine.cpp
    syncfileitem.cpp
    syncfilestatustracker.cpp
    localdiscoverytracker.cpp
    syncresult.cpp
    syncoptions.cpp
    theme.cpp
    uploaddevice.cpp

    creds/credentialmanager.cpp
    creds/abstractcredentials.cpp

    networkjobs/fetchuserinfojobfactory.cpp
    networkjobs/checkserverjobfactory.cpp
    networkjobs/jsonjob.cpp
    networkjobs/resources.cpp

    abstractcorejob.cpp

    appprovider.cpp
)

if(WIN32)
    list(APPEND libsync_SRCS platform_win.cpp)
elseif(UNIX)
    if (APPLE)
        list(APPEND libsync_SRCS platform_mac.mm platform_mac_deprecated.mm)
    else()
        list(APPEND libsync_SRCS platform_unix.cpp)
    endif()
endif()

set(libsync_SRCS ${libsync_SRCS})

# These headers are installed for libowncloudsync to be used by 3rd party apps
INSTALL(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/owncloudlib.h
        logger.h
        accessmanager.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/${APPLICATION_SHORTNAME}/libsync
)

add_library(libsync SHARED ${libsync_SRCS}
    creds/credentialssupport.h)
set_target_properties(libsync PROPERTIES EXPORT_NAME SyncCore)

target_link_libraries(libsync
    PUBLIC
        csync
        owncloudResources
        Qt::Core
        Qt::Network
        Qt::Widgets
        Qt::QuickControls2

    PRIVATE
        Qt::Concurrent
        ZLIB::ZLIB
        Qt6Keychain::Qt6Keychain
)

apply_common_target_settings(libsync)
ecm_add_qml_module(libsync
        URI org.ownCloud.libsync
        VERSION 1.0
        NAMESPACE OCC
)

ecm_finalize_qml_module(libsync DESTINATION ${KDE_INSTALL_QMLDIR})

set_source_files_properties(configfile.cpp
    PROPERTIES
        COMPILE_DEFINITIONS EXCLUDE_FILE_NAME="${EXCLUDE_FILE_NAME}"
)

target_link_libraries(libsync PUBLIC $<BUILD_INTERFACE:OpenAPI::LibreGraphAPI>)
add_subdirectory(graphapi)

if ( APPLE )
    target_link_libraries(libsync PUBLIC
         /System/Library/Frameworks/CoreServices.framework
         /System/Library/Frameworks/Foundation.framework
         /System/Library/Frameworks/AppKit.framework
         /System/Library/Frameworks/IOKit.framework
    )
endif()

if(Inotify_FOUND)
    target_include_directories(libsync PRIVATE ${Inotify_INCLUDE_DIRS})
    target_link_libraries(libsync PUBLIC ${Inotify_LIBRARIES})
endif()

if(NO_MSG_HANDLER)
    target_compile_definitions(libsync PRIVATE -DNO_MSG_HANDLER=1)
endif()

GENERATE_EXPORT_HEADER(libsync
        EXPORT_MACRO_NAME OWNCLOUDSYNC_EXPORT
        EXPORT_FILE_NAME owncloudlib.h
        STATIC_DEFINE OWNCLOUD_BUILT_AS_STATIC
)

set_target_properties(libsync PROPERTIES
        OUTPUT_NAME "ownCloudLibSync"
        VERSION ${MIRALL_VERSION}
        SOVERSION ${MIRALL_SOVERSION}
)

if(WITH_CRASHREPORTER)
    set_source_files_properties(platform.cpp PROPERTIES COMPILE_DEFINITIONS CRASHREPORTER_EXECUTABLE="${CRASHREPORTER_EXECUTABLE}")
    target_link_libraries(libsync PRIVATE CrashReporterQt::Handler)

    if(UNIX AND NOT MAC)
        find_package(Threads REQUIRED)
        target_link_libraries(libsync PUBLIC Threads::Threads)
    endif()
endif()


install(TARGETS libsync EXPORT ${APPLICATION_SHORTNAME}Config ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
