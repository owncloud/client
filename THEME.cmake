if(WITH_EXTERNAL_BRANDING)
    include(FetchContent)

    FetchContent_Declare(branding
            GIT_REPOSITORY ${WITH_EXTERNAL_BRANDING}
            GIT_TAG main
    )
    FetchContent_MakeAvailable(branding)

    set(OEM_THEME_DIR ${branding_SOURCE_DIR} CACHE STRING "The directory containing a custom theme")
else()
    if (EXISTS "${PROJECT_SOURCE_DIR}/branding")
        set(OEM_THEME_DIR "${PROJECT_SOURCE_DIR}/branding" CACHE STRING "The directory containing a custom theme")
    else()
        set(OEM_THEME_DIR "${PROJECT_SOURCE_DIR}/src/resources/" CACHE STRING "Define directory containing a custom theme")
    endif()
endif()

if (EXISTS "${OEM_THEME_DIR}/OEM.cmake")
    include("${OEM_THEME_DIR}/OEM.cmake")
    if (DEFINED WITHOUT_AUTO_UPDATER)
        set(WITH_AUTO_UPDATER OFF)
        MESSAGE(STATUS "Branding: WITHOUT_AUTO_UPDATER was defined in the theme and therefore the auto updater has been disabled")
    endif ()

else()
    include ("${CMAKE_CURRENT_LIST_DIR}/OWNCLOUD.cmake")
endif()

message(STATUS "Branding: ${APPLICATION_NAME}")

set(APPLICATION_REV_DOMAIN_INSTALLER ${APPLICATION_REV_DOMAIN})

# need this logic to not mess with re/uninstallations via macosx.pkgproj
if(${APPLICATION_REV_DOMAIN} STREQUAL "com.owncloud.desktopclient")
    set(APPLICATION_REV_DOMAIN_INSTALLER "com.ownCloud.client")
else()
    set(APPLICATION_REV_DOMAIN_INSTALLER ${APPLICATION_REV_DOMAIN})
endif()

# For usage in XML files we preprocess
string(REPLACE "&" "&amp;" APPLICATION_NAME_XML_ESCAPED "${APPLICATION_NAME}")
string(REPLACE "<" "&lt;" APPLICATION_NAME_XML_ESCAPED "${APPLICATION_NAME_XML_ESCAPED}")
string(REPLACE ">" "&gt;" APPLICATION_NAME_XML_ESCAPED "${APPLICATION_NAME_XML_ESCAPED}")

if (NOT DEFINED LINUX_PACKAGE_SHORTNAME)
    set(LINUX_PACKAGE_SHORTNAME "${APPLICATION_SHORTNAME}")
endif()

if (NOT DEFINED PACKAGE)
    set(PACKAGE "${LINUX_PACKAGE_SHORTNAME}-client")
endif()

if(NOT CRASHREPORTER_EXECUTABLE)
    set(CRASHREPORTER_EXECUTABLE "${APPLICATION_EXECUTABLE}_crash_reporter")
endif()


include("${CMAKE_CURRENT_LIST_DIR}/VERSION.cmake")
