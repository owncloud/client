project(libsyncclient)
set(CMAKE_AUTOMOC TRUE)
include(GenerateExportHeader)

configure_file(version.h.in "${CMAKE_CURRENT_BINARY_DIR}/version.h" )

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
# # csync is required.
include_directories(
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${CMAKE_SOURCE_DIR}/csync/src
                    ${CMAKE_SOURCE_DIR}/src/gui
                    ${CMAKE_BINARY_DIR}/src/gui # for ui_sslerrordialog.h
                    ${CMAKE_SOURCE_DIR}/src/libsync
                    ${CMAKE_BINARY_DIR}/src/libsync # for owncloudlib.h
                    ${CMAKE_BINARY_DIR}/csync
                   )
include_directories(${CMAKE_SOURCE_DIR}/src/3rdparty/qjson)

if ( APPLE )
    list(APPEND OS_SPECIFIC_LINK_LIBRARIES
         /System/Library/Frameworks/CoreServices.framework
         /System/Library/Frameworks/Foundation.framework
         /System/Library/Frameworks/AppKit.framework
    )
endif()

set(syncclientlib_NAME "syncclient")

set(syncclient_UI
    ../gui/sslerrordialog.ui
    ../gui/proxyauthdialog.ui
)

qt_wrap_ui(clientlib_UI_SRCS ${syncclient_UI})

set(libsyncclient_SRCS
    ../gui/folder.cpp
    ../gui/folderman.cpp
    ../gui/folderwatcher.cpp
    ../gui/accountstate.cpp
    ../gui/accountmanager.cpp
    ../gui/proxyauthhandler.cpp
    ../gui/proxyauthdialog.cpp
    ../gui/sslerrordialog.cpp
    ../gui/creds/httpcredentialsgui.cpp
    ../gui/creds/credentialsfactory.cpp
    ../gui/creds/shibbolethcredentials.cpp
    ../gui/creds/shibboleth/shibbolethwebview.cpp
    ../gui/creds/shibboleth/shibbolethuserjob.cpp
    ../gui/windowraiser.cpp
    ../gui/socketapi.cpp
    ../gui/syncrunfilelog.cpp
    ${clientlib_UI_SRCS}
)

IF( NOT WIN32 AND NOT APPLE )
set(libsyncclient_SRCS ${libsyncclient_SRCS} ../gui/folderwatcher_linux.cpp)
ENDIF()
IF( WIN32 )
set(libsyncclient_SRCS ${libsyncclient_SRCS} ../gui/folderwatcher_win.cpp)
ENDIF()
IF( APPLE )
list(APPEND libsyncclient_SRCS ../gui/folderwatcher_mac.cpp)
ENDIF()


# These headers are installed for libsyncclient to be used by 3rd party apps
set(syncclient_HEADERS
    ../gui/accountmanager.h
    ../gui/accountstate.h
    ../gui/folder.h
    ../gui/folderman.h
    ../gui/folderwatcher.h
)

IF (NOT APPLE)
    INSTALL(
        FILES ${syncclient_HEADERS}
        DESTINATION ${INCLUDE_INSTALL_DIR}/${syncclientlib_NAME}
    )
    INSTALL(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/syncclientlib.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/${syncclientlib_NAME}
    )
ENDIF(NOT APPLE)

list(APPEND libsyncclient_LINK_TARGETS
#    ${QT_LIBRARIES}
#    ocsync
    owncloudsync
)

if(QTKEYCHAIN_FOUND OR QT5KEYCHAIN_FOUND)
    list(APPEND libsyncclient_LINK_TARGETS ${QTKEYCHAIN_LIBRARY})
    include_directories(${QTKEYCHAIN_INCLUDE_DIR})
endif()

add_library(${syncclientlib_NAME} SHARED ${libsyncclient_SRCS} ${syncMoc})
GENERATE_EXPORT_HEADER( ${syncclientlib_NAME}
    BASE_NAME ${syncclientlib_NAME}
    EXPORT_MACRO_NAME OWNCLOUDSYNC_EXPORT
        EXPORT_FILE_NAME syncclientlib.h
    STATIC_DEFINE OWNCLOUD_BUILT_AS_STATIC
)

qt5_use_modules(${syncclientlib_NAME} Network)

set_target_properties( ${syncclientlib_NAME}  PROPERTIES
    VERSION ${MIRALL_VERSION}
    SOVERSION ${MIRALL_SOVERSION}
    RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIRECTORY}
)
set_target_properties( ${syncclientlib_NAME} PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/${APPLICATION_EXECUTABLE}" )

target_link_libraries(${syncclientlib_NAME} ${libsyncclient_LINK_TARGETS} )

if(INOTIFY_FOUND)
    target_link_libraries(${syncclientlib_NAME} ${INOTIFY_LIBRARY} )
endif()

if(NOT BUILD_OWNCLOUD_OSX_BUNDLE)
    install(TARGETS ${syncclientlib_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else()
    install(TARGETS ${syncclientlib_NAME} DESTINATION ${OWNCLOUD_OSX_BUNDLE}/Contents/MacOS)
    if (SPARKLE_FOUND)
        install(DIRECTORY "${SPARKLE_LIBRARY}"
            DESTINATION "${OWNCLOUD_OSX_BUNDLE}/Contents/Frameworks" USE_SOURCE_PERMISSIONS)
    endif (SPARKLE_FOUND)
endif()


