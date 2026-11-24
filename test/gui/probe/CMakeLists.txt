# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT

# Builds on its own (cmake -S test/gui/probe) and as a subdirectory of an application.
cmake_minimum_required(VERSION 3.18)
project(QtGuiProbe LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(QTGUIPROBE_STANDALONE ON)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    include(CTest)
else()
    set(QTGUIPROBE_STANDALONE OFF)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Network Test Quick QuickWidgets)

# The probe throws ProbeError internally; applications may build with -fno-exceptions.
# The definitions keep the sources clean under the strict Qt settings applications commonly use.
function(qtguiprobe_apply_settings target)
    set_target_properties(${target} PROPERTIES AUTOMOC ON)
    target_compile_features(${target} PUBLIC cxx_std_17)
    target_compile_options(${target} PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fexceptions>)
    target_compile_definitions(${target}
        PRIVATE
            QT_NO_CAST_TO_ASCII
            QT_NO_CAST_FROM_ASCII
            QT_NO_URL_CAST_FROM_STRING
            QT_NO_CAST_FROM_BYTEARRAY
            QT_USE_QSTRINGBUILDER
            QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
            QT_NO_KEYWORDS
            QT_NO_FOREACH
    )
endfunction()

# An OBJECT library, so the linker cannot drop the Q_COREAPP_STARTUP_FUNCTION that starts the probe.
add_library(qtguiprobe OBJECT
    probe.cpp
    probe.h
    registry.cpp
    registry.h
    jsonvalue.cpp
    jsonvalue.h
    selector.cpp
    selector.h
    items.cpp
    items.h
    quick.cpp
    quick.h
    actions.cpp
    actions.h
    keys.cpp
    keys.h
)
add_library(QtGuiProbe::qtguiprobe ALIAS qtguiprobe)
qtguiprobe_apply_settings(qtguiprobe)
target_include_directories(qtguiprobe
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/QtGuiProbe>
)
target_link_libraries(qtguiprobe
    PUBLIC
        Qt6::Widgets
        Qt6::Network
        Qt6::Test
        Qt6::Quick
        Qt6::QuickWidgets
)

# Only a standalone build installs the probe; an application that adds it as a subdirectory must not ship it.
if(QTGUIPROBE_STANDALONE)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    set(QTGUIPROBE_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/QtGuiProbe)
    install(TARGETS qtguiprobe EXPORT QtGuiProbeTargets OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR}/QtGuiProbe)
    install(FILES probe.h registry.h jsonvalue.h selector.h items.h quick.h actions.h keys.h DESTINATION include/QtGuiProbe)
    install(EXPORT QtGuiProbeTargets NAMESPACE QtGuiProbe:: DESTINATION ${QTGUIPROBE_CMAKE_DIR})
    configure_package_config_file(QtGuiProbeConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/QtGuiProbeConfig.cmake
        INSTALL_DESTINATION ${QTGUIPROBE_CMAKE_DIR}
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/QtGuiProbeConfig.cmake DESTINATION ${QTGUIPROBE_CMAKE_DIR})
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
