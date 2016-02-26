# (c) 2014 Copyright ownCloud, Inc.
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING* file.

# - Try to find QtKeychain
# Once done this will define
#  OWNCLOUDSYNC_FOUND - System has QtKeychain
#  OWNCLOUDSYNC_INCLUDE_DIRS - The QtKeychain include directories
#  OWNCLOUDSYNC_LIBRARIES - The libraries needed to use QtKeychain
#  OWNCLOUDSYNC_DEFINITIONS - Compiler switches required for using LibXml2

find_path(OWNCLOUDSYNC_INCLUDE_DIR
            NAMES
              owncloudlib.h
            PATH_SUFFIXES
              owncloudsync
            )

find_library(OWNCLOUDSYNC_LIBRARY
            NAMES
              owncloudsync
            PATHS
               /usr/lib
               /usr/lib/${CMAKE_ARCH_TRIPLET}
               /usr/local/lib
               /opt/local/lib
               ${CMAKE_LIBRARY_PATH}
               ${CMAKE_INSTALL_PREFIX}/lib
            )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set OWNCLOUDSYNC_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(OwncloudSync  DEFAULT_MSG
    OWNCLOUDSYNC_LIBRARY OWNCLOUDSYNC_INCLUDE_DIR)

mark_as_advanced(OWNCLOUDSYNC_INCLUDE_DIR OWNCLOUDSYNC_LIBRARY)
