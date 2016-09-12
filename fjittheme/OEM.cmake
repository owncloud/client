set( APPLICATION_NAME       "Skylagring" )
set( APPLICATION_EXECUTABLE "Skylagring" )
set( APPLICATION_DOMAIN     "fil.fjit.no" )
set( APPLICATION_VENDOR     "Serit Fjordane IT AS" )
set( APPLICATION_UPDATE_URL "https://deploy.fjit.no/skylagring/client/" CACHE string "URL for updater" )
set( APPLICATION_CLSID      "{2F8BD8E0-03B3-47C5-A1E1-5C182FA6FFD0}" )

set( THEME_CLASS            "MyTheme" )
set( APPLICATION_REV_DOMAIN "no.fjit.fil.desktopclient" )
set( WIN_SETUP_BITMAP_PATH  "${OEM_THEME_DIR}/resources" )

set( CPACK_PACKAGE_ICON  "${OEM_THEME_DIR}/resources/skylagring.ico" )

# set( MAC_INSTALLER_BACKGROUND_FILE "${CMAKE_SOURCE_DIR}/admin/osx/installer-background.png" CACHE STRING "The MacOSX installer background image")

set( THEME_INCLUDE          "${OEM_THEME_DIR}/mytheme.h" )
# set( APPLICATION_LICENSE    "${OEM_THEME_DIR}/license.txt )

set( MIRALL_VERSION_SUFFIX "")
