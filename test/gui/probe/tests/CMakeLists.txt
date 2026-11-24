# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT

function(qtguiprobe_add_test name)
    add_executable(${name} ${name}.cpp)
    qtguiprobe_apply_settings(${name})
    target_link_libraries(${name} PRIVATE qtguiprobe Qt6::Test)
    add_test(NAME qtguiprobe-${name} COMMAND ${name})
    set_tests_properties(qtguiprobe-${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

qtguiprobe_add_test(tst_protocol)
qtguiprobe_add_test(tst_startup)
qtguiprobe_add_test(tst_selector)
qtguiprobe_add_test(tst_items)
qtguiprobe_add_test(tst_quick)
qt_add_resources(tst_quick tst_quick_qml PREFIX "/" FILES tst_quick.qml)
# Qt Quick renders without a GPU under the offscreen platform; the Basic style looks the same everywhere
set_tests_properties(qtguiprobe-tst_quick PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Basic")
