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
