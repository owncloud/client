target_sources(libsync PRIVATE
        jobs/drives.cpp
        openapi/OAICollection_of_drives.cpp
        openapi/OAIDeleted.cpp
        openapi/OAIDrive.cpp
        openapi/OAIDriveItem.cpp
        openapi/OAIHelpers.cpp
        openapi/OAIQuota.cpp
        openapi/OAISpecialFolder.cpp
        space.cpp
        spacesmanager.cpp)

target_include_directories(libsync PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
