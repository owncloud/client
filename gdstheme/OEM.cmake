set( APPLICATION_NAME       "GreenDataStorage" )
set( APPLICATION_EXECUTABLE "GreenDataStorage" )
set( APPLICATION_DOMAIN     "cloud.green-data-storage.com" )
set( APPLICATION_VENDOR     "Serit Fjordane IT AS" )
set( APPLICATION_UPDATE_URL "http://dev-deploy.fjit.no/GreenDataStorage/client/" CACHE string "URL for updater" )
set( APPLICATION_CLSID      "{A25C5915-BCDB-4601-AEE7-748FC94A434A}" )

set( THEME_CLASS            "MyTheme" )
set( APPLICATION_REV_DOMAIN "no.fjit.GreenDataStorage.desktopclient" )
set( WIN_SETUP_BITMAP_PATH  "${OEM_THEME_DIR}/resources" )

set( CPACK_PACKAGE_ICON  "${OEM_THEME_DIR}/resources/gds.ico" )

# set( MAC_INSTALLER_BACKGROUND_FILE "${CMAKE_SOURCE_DIR}/admin/osx/installer-background.png" CACHE STRING "The MacOSX installer background image")

set( THEME_INCLUDE          "${OEM_THEME_DIR}/mytheme.h" )
# set( APPLICATION_LICENSE    "${OEM_THEME_DIR}/license.txt )

set( MIRALL_VERSION_SUFFIX "")
