#
# This file was originally licensed under ownCloud Commercial License.
# see <https://owncloud.com/licenses/owncloud-commercial/>
# As of 2025, it is relicensed under MIT.
#
# SPDX-License-Identifier: GPL-2.0-or-later
if(WIN32)
    add_library(vfs_plugin_win_static OBJECT
        vfs_win.cpp
        hydrationdevice.cpp
        validationdevice.cpp
        callbacks.cpp
        utility.cpp
        )

    apply_common_target_settings(vfs_plugin_win_static)

    target_link_libraries(vfs_plugin_win_static PUBLIC
        libsync
        cldapi
        Propsys
        WindowsApp
    )
    add_vfs_plugin(NAME wincfapi SRC vfs_win_plugin_factory.cpp LIBS vfs_plugin_win_static)
endif()
