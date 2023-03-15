if(UNIX AND NOT APPLE)

  set(ICON_DIR ${DATA_INSTALL_DIR}/icons/hicolor)

  foreach(
    size
    128x128
    16x16
    256x256
    32x32
    48x48
    64x64
    72x72)
    file(GLOB files "${size}/*.png")
    foreach(file ${files})
      # the GLOB returns a absolute path. Make it relative by replacing the current src dir by nothing
      string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/${size}/" "" shortFile ${file})
      string(REPLACE "oC" ${APPLICATION_SHORTNAME} brandedName ${shortFile})
      install(
        FILES ${file}
        DESTINATION ${ICON_DIR}/${size}/apps
        RENAME ${brandedName})
    endforeach(file)
  endforeach(size)

endif()
