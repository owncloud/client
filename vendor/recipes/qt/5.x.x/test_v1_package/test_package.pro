SOURCES += ../test_package/test_package.cpp

HEADERS += ../test_package/greeter.h

RESOURCES = ../test_package/example.qrc

QT -= gui
QT += network sql concurrent xml

CONFIG += console

CONFIG += conan_basic_setup
include($$OUT_PWD/../conanbuildinfo.pri)
LIBS -= $$CONAN_LIBS_QT
