project(libowncloudclient)
set(CMAKE_AUTOMOC TRUE)
include(GenerateExportHeader)

configure_file(version.h.in "${CMAKE_CURRENT_BINARY_DIR}/version.h" )

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
# # csync is required.
include_directories(
                    ${CMAKE_SOURCE_DIR}/csync/src
                    #${CMAKE_SOURCE_DIR}/csync/src/httpbf/src
                    ${CMAKE_SOURCE_DIR}/src/gui
                    ${CMAKE_BINARY_DIR}/src/gui # for ui_sslerrordialog.h
                    ${CMAKE_SOURCE_DIR}/src/libsync
                    ${CMAKE_BINARY_DIR}/src/libsync # for owncloudlib.h
                    ${CMAKE_BINARY_DIR}/csync
#                    ${CMAKE_BINARY_DIR}/csync/src
                   )
include_directories(${CMAKE_SOURCE_DIR}/src/3rdparty/qjson)
# include_directories(${OPENSSL_INCLUDE_DIR})
#
if ( APPLE )
    list(APPEND OS_SPECIFIC_LINK_LIBRARIES
         /System/Library/Frameworks/CoreServices.framework
         /System/Library/Frameworks/Foundation.framework
         /System/Library/Frameworks/AppKit.framework
    )
endif()

set(owncloudclientlib_NAME "owncloudclient")
# IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD|NetBSD|OpenBSD")
#     list(APPEND OS_SPECIFIC_LINK_LIBRARIES
#         inotify
#     )
# ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD|NetBSD|OpenBSD")
#
# if(SPARKLE_FOUND AND NOT BUILD_LIBRARIES_ONLY)
#     list (APPEND OS_SPECIFIC_LINK_LIBRARIES ${SPARKLE_LIBRARY})
# endif()

set(libowncloudclient_SRCS
    #account.cpp
    ../gui/folder.cpp
    ../gui/folderman.cpp
    ../gui/folderwatcher.cpp
    ../gui/folderwatcher_linux.cpp
    ../gui/accountstate.cpp
    ../gui/accountmanager.cpp
    ../gui/sslerrordialog.cpp
    ../gui/creds/httpcredentialsgui.cpp
    ../gui/creds/credentialsfactory.cpp
    ../gui/creds/shibbolethcredentials.cpp
    ../gui/creds/shibboleth/shibbolethwebview.cpp
#    ../gui/creds/shibboleth/shibbolethrefresher.cpp
    ../gui/creds/shibboleth/shibbolethuserjob.cpp
    ../gui/socketapi.cpp
    ../gui/syncrunfilelog.cpp
)

# if(USE_NEON)
#     list(APPEND libowncloudclient_SRCS
#         propagator_legacy.cpp
#     )
#     add_definitions(-DUSE_NEON)
# endif(USE_NEON)

# These headers are installed for libowncloudsync to be used by 3rd party apps
set(owncloudsync_HEADERS
    ../gui/accountmanager.h
    ../gui/accountstate.h
    ../gui/folder.h
    ../gui/folderman.h
    ../gui/folderwatcher.h
)

IF (NOT APPLE)
    INSTALL(
        FILES ${owncloudsync_HEADERS}
	DESTINATION ${INCLUDE_INSTALL_DIR}/${owncloudclientlib_NAME}
    )
    INSTALL(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/owncloudclientlib.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/${owncloudclientlib_NAME}
    )
ENDIF(NOT APPLE)

list(APPEND libowncloudclient_LINK_TARGETS
    ${QT_LIBRARIES}
    ocsync
    owncloudsync
    ${OS_SPECIFIC_LINK_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

if(QTKEYCHAIN_FOUND OR QT5KEYCHAIN_FOUND)
    list(APPEND libowncloudclient_LINK_TARGETS ${QTKEYCHAIN_LIBRARY})
    include_directories(${QTKEYCHAIN_INCLUDE_DIR})
endif()
#
# if(INOTIFY_FOUND)
#     include_directories(${INOTIFY_INCLUDE_DIR})
#     link_directories(${INOTIFY_LIBRARY_DIR})
# endif()
#
# if(NEON_FOUND)
#     list(APPEND libowncloudclient_LINK_TARGETS ${NEON_LIBRARIES} httpbf)
#     include_directories(${NEON_INCLUDE_DIRS})
#
#     if(NEON_WITH_LFS)
#         add_definitions(-DNE_LFS)
#     endif()
# endif()
#
# if(ZLIB_FOUND)
#     list(APPEND libowncloudclient_LINK_TARGETS ${ZLIB_LIBRARIES})
#     include_directories(${ZLIB_INCLUDE_DIRS})
# endif(ZLIB_FOUND)
#
add_library(${owncloudclientlib_NAME} SHARED ${libowncloudclient_SRCS} ${syncMoc})
GENERATE_EXPORT_HEADER( ${owncloudclientlib_NAME}
	BASE_NAME ${owncloudclientlib_NAME}
	EXPORT_MACRO_NAME OWNCLOUDSYNC_EXPORT
        EXPORT_FILE_NAME owncloudclientlib.h
	STATIC_DEFINE OWNCLOUD_BUILT_AS_STATIC
)

# if(TOKEN_AUTH_ONLY)
#     qt5_use_modules(${owncloudclientlib_NAME} Network Concurrent)
# else()
    qt5_use_modules(${owncloudclientlib_NAME} Widgets Network WebKitWidgets Concurrent)
# endif()

set_target_properties( ${owncloudclientlib_NAME}  PROPERTIES
	VERSION ${MIRALL_VERSION}
	SOVERSION ${MIRALL_SOVERSION}
	RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIRECTORY}
)
set_target_properties( ${owncloudclientlib_NAME} PROPERTIES
	INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/${APPLICATION_EXECUTABLE}" )

target_link_libraries(${owncloudclientlib_NAME} ${libowncloudclient_LINK_TARGETS} )

if(INOTIFY_FOUND)
    target_link_libraries(${owncloudclientlib_NAME} ${INOTIFY_LIBRARY} )
endif()

if(BUILD_LIBRARIES_ONLY)
    #add_library(${owncloudclientlib_NAME}_static STATIC ${libowncloudclient_SRCS} ${syncMoc})
    #qt5_use_modules(${owncloudclientlib_NAME}_static Widgets Network Xml Sql)

    #set_target_properties( ${owncloudclientlib_NAME}_static  PROPERTIES
    #    VERSION ${MIRALL_VERSION}
    #    SOVERSION ${MIRALL_SOVERSION}
    #)

    #target_link_libraries(${owncloudclientlib_NAME}_static ${libowncloudclient_LINK_TARGETS} )
endif()

if(NOT BUILD_OWNCLOUD_OSX_BUNDLE)
    install(TARGETS ${owncloudclientlib_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else()
    install(TARGETS ${owncloudclientlib_NAME} DESTINATION ${OWNCLOUD_OSX_BUNDLE}/Contents/MacOS)
    if (SPARKLE_FOUND)
        install(DIRECTORY "${SPARKLE_LIBRARY}"
	        DESTINATION "${OWNCLOUD_OSX_BUNDLE}/Contents/Frameworks" USE_SOURCE_PERMISSIONS)
    endif (SPARKLE_FOUND)
endif()


