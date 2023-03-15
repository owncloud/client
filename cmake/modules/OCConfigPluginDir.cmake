function(IS_SUBDIR ROOT CHILD OUT)
  file(RELATIVE_PATH REL_PATH "${ROOT}" "${CHILD}")
  string(REGEX MATCH "^\.\./" NOT_SUBDIR "${REL_PATH}")
  if(NOT_SUBDIR)
    set(${OUT}
        FALSE
        PARENT_SCOPE)
  else()
    set(${OUT}
        TRUE
        PARENT_SCOPE)
  endif()
endfunction()

if(UNIX AND NOT APPLE)
  set(OC_PLUGIN_DIR ${KDE_INSTALL_FULL_PLUGINDIR})
  is_subdir("${CMAKE_INSTALL_PREFIX}" "${OC_PLUGIN_DIR}" _is_subdir)
  if(NOT _is_subdir)
    if(KDE_INSTALL_USE_QT_SYS_PATHS)
      message(FATAL_ERROR "Using KDE_INSTALL_USE_QT_SYS_PATHS with a non bundled Qt is not supported.")
    else()
      message(FATAL_ERROR "KDE_INSTALL_PLUGINDIR must be located in CMAKE_INSTALL_PREFIX")
    endif()
  endif()

  include(ECMQueryQmake)
  query_qmake(_qt_plugin_dir QT_INSTALL_PLUGINS TRY)
  # any sub dir of _qt_plugin_dir is sufficient
  is_subdir("${_qt_plugin_dir}" "${OC_PLUGIN_DIR}" _is_subdir)
  if(_is_subdir)
    # no need to add a additional plugin dir
    unset(OC_PLUGIN_DIR)
  else()
    # set plugin dir to a path relative to the binary
    file(RELATIVE_PATH OC_PLUGIN_DIR "${CMAKE_INSTALL_FULL_BINDIR}" "${OC_PLUGIN_DIR}")
    message(STATUS "Adding additional plugin path: ${OC_PLUGIN_DIR}")
  endif()
endif()
