# global needed variables
set(APPLICATION_NAME "ocsync")
set(APPLICATION_VERSION_MAJOR ${MIRALL_VERSION_MAJOR})
set(APPLICATION_VERSION_MINOR ${MIRALL_VERSION_MINOR})
set(APPLICATION_VERSION_PATCH ${MIRALL_VERSION_PATCH})
set(APPLICATION_VERSION ${MIRALL_VERSION})

set(LIBRARY_VERSION ${MIRALL_VERSION})
set(LIBRARY_SOVERSION ${MIRALL_SOVERSION})

# add definitions
include(DefineCMakeDefaults)
include(DefinePlatformDefaults)
include(DefineCompilerFlags)
include(DefineOptions.cmake)

include(DefineInstallationPaths)

# add macros
include(MacroAddPlugin)
include(MacroCopyFile)

if (NOT WIN32)
    find_package(Iconv)
endif (NOT WIN32)
find_package(CMocka)
if (CMOCKA_FOUND AND UNIT_TESTING)
    include(AddCMockaTest)
endif (CMOCKA_FOUND AND UNIT_TESTING)

include(ConfigureChecks.cmake)

# Statically include sqlite
if (CSYNC_STATIC_COMPILE_DIR)
    set(SQLITE3_INCLUDE_DIRS "")
    set(SQLITE3_LIBRARIES "")
    include_directories(${CSYNC_STATIC_COMPILE_DIR})
else (CSYNC_STATIC_COMPILE_DIR)
    find_package(SQLite3 3.3.9 REQUIRED)
endif()

set(SOURCE_DIR ${CMAKE_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

macro_copy_file(${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.cmake ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake)

if (MEM_NULL_TESTS)
  add_definitions(-DCSYNC_MEM_NULL_TESTS)
endif (MEM_NULL_TESTS)

add_subdirectory(src)

set(CSYNC_PUBLIC_INCLUDE_DIRS
   ${CMAKE_CURRENT_BINARY_DIR}
   ${CSYNC_SRC_PUBLIC_INCLUDE_DIRS}
   ${HTTPBF_PUBLIC_INCLUDE_DIRS}
   CACHE INTERNAL "csync public include directories"
)

if (CMOCKA_FOUND AND UNIT_TESTING)
    add_subdirectory(tests)
endif (CMOCKA_FOUND AND UNIT_TESTING)

configure_file(config_csync.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config_csync.h)
configure_file(config_test.h.cmake  ${CMAKE_CURRENT_BINARY_DIR}/config_test.h)

