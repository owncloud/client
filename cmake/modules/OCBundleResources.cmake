function(__add_file_to_qrc_file)
    set(options "")
    set(oneValueArgs QRC_PATH FILE_PATH ALIAS)
    set(multiValueArgs)
    cmake_parse_arguments(__ADD_FILE_TO_QRC_FILE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(param QRC_PATH FILE_PATH)
        if(NOT __ADD_FILE_TO_QRC_FILE_${param})
            message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION}: Argument missing: ${param}")
        endif()
    endforeach()

    set(line "<file ")
    if(__ADD_FILE_TO_QRC_FILE_ALIAS)
        set(line "${line} alias=\"${__ADD_FILE_TO_QRC_FILE_ALIAS}\"")
    endif()
    set(line "${line}>${__ADD_FILE_TO_QRC_FILE_FILE_PATH}</file>")

    file(APPEND ${__ADD_FILE_TO_QRC_FILE_QRC_PATH} "        ${line}\n")
endfunction()

function(__addIconFromJson JSON QRC_PATH THEME ICON_KEY)
    set(options)
    set(oneValueArgs ICON_NAME)
    set(multiValueArgs)
    cmake_parse_arguments(_ICON "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    string(JSON ICON_NAME GET "${JSON}" ${ICON_KEY})
    get_filename_component(ICON_NAME_EXT ${ICON_NAME} LAST_EXT)

    if (NOT _ICON_ICON_NAME)
        get_filename_component(_ICON_ICON_NAME ${ICON_NAME} NAME)
    endif()

    set(iconAlias "theme/${THEME}/${_ICON_ICON_NAME}${ICON_NAME_EXT}")
    if (NOT EXISTS "${OEM_THEME_DIR}/${ICON_NAME}")
        message(FATAL_ERROR "Icon ${OEM_THEME_DIR}/${ICON_NAME} does not exist")
    endif()
    file(APPEND "${QRC_PATH}" "<file alias=\"${iconAlias}\">${OEM_THEME_DIR}/${ICON_NAME}</file>\n")
endfunction()

function(__write_qrc_file_header QRC_PATH FILES_PREFIX)
    file(WRITE ${QRC_PATH} "<RCC>\n")
    file(APPEND ${QRC_PATH} "    <qresource prefix=\"/client/\">\n")
endfunction()

function(__write_qrc_file_footer QRC_PATH)
    file(APPEND ${QRC_PATH} "    </qresource>\n")
    file(APPEND ${QRC_PATH} "</RCC>\n")
endfunction()

function(generate_theme TARGET)
    set(STATES "ok;error;information;offline;pause;sync")
    set(THEMES "colored;dark;black;white")

    set(QRC_PATH ${CMAKE_CURRENT_BINARY_DIR}/theme.qrc)
    __write_qrc_file_header(${QRC_PATH} theme)

    __add_file_to_qrc_file(
        QRC_PATH ${QRC_PATH}
        FILE_PATH "${OEM_THEME_DIR}/config_with_defaults.json"
        ALIAS "theme/config_with_defaults.json"
    )
    file(READ "${OEM_THEME_DIR}/config_with_defaults.json" JSON)
    __addIconFromJson(${JSON} ${QRC_PATH} "universal" desktop_appshortname_icon_image ICON_NAME "${APPLICATION_ICON_NAME}-icon")
    __addIconFromJson(${JSON} ${QRC_PATH} "universal" desktop_wizard_logo_image ICON_NAME "wizard_logo")

    foreach(theme ${THEMES})
        foreach(state ${STATES})
            set(key ${state})
            if (${key} STREQUAL "information")
                set(key "info")
            endif()
            __addIconFromJson(${JSON} ${QRC_PATH} ${theme} "desktop_state_${key}_image" ICON_NAME "state-${state}")
        endforeach()
    endforeach()
    __write_qrc_file_footer(${QRC_PATH})
    target_sources(${TARGET} PRIVATE ${QRC_PATH})
endfunction()

function(generate_qrc_file)
    set(options "")
    set(oneValueArgs QRC_PATH PREFIX)
    set(multiValueArgs FILES)
    cmake_parse_arguments(GENERATE_QRC_FILE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(param ${oneValueArgs} ${multiValueArgs})
        if(NOT GENERATE_QRC_FILE_${param})
            message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION}: Argument missing: ${param}")
        endif()
    endforeach()

    __write_qrc_file_header(${GENERATE_QRC_FILE_QRC_PATH} ${GENERATE_QRC_FILE_PREFIX})

    foreach(file ${GENERATE_QRC_FILE_FILES})
        get_filename_component(file_name ${file} NAME)
        set(file_alias ${GENERATE_QRC_FILE_PREFIX}/${file_name})
        __add_file_to_qrc_file(
            QRC_PATH ${GENERATE_QRC_FILE_QRC_PATH}
            FILE_PATH ${file}
            ALIAS ${file_alias}
        )
    endforeach()

    __write_qrc_file_footer(${GENERATE_QRC_FILE_QRC_PATH})
endfunction()

# add resources to a target using the Qt resources system
# parameters:
#   - TARGET: the target to bundle the resources with
#   - PREFIX: virtual "subdirectory" the files will be available from
#   - FILES: the files to bundle
function(add_resources_to_target)
    set(options "")
    set(oneValueArgs TARGET PREFIX)
    set(multiValueArgs FILES)
    cmake_parse_arguments(ADD_RESOURCES_TO_TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(param ${oneValueArgs} ${multiValueArgs})
        if(NOT ADD_RESOURCES_TO_TARGET_${param})
            message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION}: Argument missing: ${param}")
        endif()
    endforeach()

    set(qrc_path ${CMAKE_CURRENT_BINARY_DIR}/${ADD_RESOURCES_TO_TARGET_TARGET}_${ADD_RESOURCES_TO_TARGET_PREFIX}.qrc)
    generate_qrc_file(
        QRC_PATH ${qrc_path}
        PREFIX ${ADD_RESOURCES_TO_TARGET_PREFIX}
        FILES "${ADD_RESOURCES_TO_TARGET_FILES}"
    )
    target_sources(${ADD_RESOURCES_TO_TARGET_TARGET} PRIVATE ${qrc_path})
endfunction()
