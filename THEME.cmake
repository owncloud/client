if(WITH_EXTERNAL_BRANDING)
    include(FetchContent)

    if(NOT WITH_EXTERNAL_BRANDING_TAG)
        set(WITH_EXTERNAL_BRANDING_TAG "master")
    endif()

    FetchContent_Declare(branding
            GIT_REPOSITORY ${WITH_EXTERNAL_BRANDING}
            GIT_TAG ${WITH_EXTERNAL_BRANDING_TAG}
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

# Name used for packaging artifacts (AppImage, packages, installers). This
# deliberately does not use APPLICATION_SHORTNAME directly: themes take it from
# theme.json (desktop_application_short_name_text), which can differ in case,
# e.g. "kiteworks" vs "Kiteworks". APPLICATION_SHORTNAME is used at runtime
# (settings dir, keychain), so it must not be changed to match.
set(ARTIFACT_PREFIX "${APPLICATION_SHORTNAME}")
if (EXISTS "${OEM_THEME_DIR}/theme.json")
    if (CMAKE_VERSION VERSION_LESS 3.19)
        message(WARNING "Branding: CMake ${CMAKE_VERSION} cannot read theme.json, using APPLICATION_SHORTNAME for artifact names")
    else()
        file(READ "${OEM_THEME_DIR}/theme.json" _theme_json)
        string(JSON _theme_short_name ERROR_VARIABLE _theme_json_error GET "${_theme_json}" desktop_application_short_name_text)
        if (NOT _theme_json_error AND NOT _theme_short_name STREQUAL "")
            set(ARTIFACT_PREFIX "${_theme_short_name}")
        endif()
    endif()
endif()
string(TOLOWER "${ARTIFACT_PREFIX}" _linux_package_base)
string(REPLACE "_" "-" _linux_package_base "${_linux_package_base}")
set(LINUX_PACKAGE_NAME "${_linux_package_base}-client")
message(STATUS "Branding: artifact prefix ${ARTIFACT_PREFIX}, Linux package ${LINUX_PACKAGE_NAME}")

# Read by the packaging scripts in admin/ so artifact names match the build.
file(WRITE "${CMAKE_BINARY_DIR}/packaging.env"
    "ARTIFACT_PREFIX='${ARTIFACT_PREFIX}'\n"
    "VERSION_STRING='${MIRALL_VERSION_STRING}'\n"
    "APPLICATION_EXECUTABLE='${APPLICATION_EXECUTABLE}'\n"
    "LINUX_PACKAGE_NAME='${LINUX_PACKAGE_NAME}'\n"
)
