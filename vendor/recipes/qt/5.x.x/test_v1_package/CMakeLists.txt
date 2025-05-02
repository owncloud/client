cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_CXX_STANDARD 11)

project(test_package)

cmake_policy(SET CMP0020 NEW)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_set_vs_runtime()
conan_set_libcxx()
conan_output_dirs_setup()

find_package(Qt5 COMPONENTS Core Network Sql Concurrent Xml REQUIRED CONFIG)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(SOURCES ../test_package/test_package.cpp ../test_package/greeter.h ../test_package/example.qrc)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Network Qt5::Sql Qt5::Concurrent Qt5::Xml)
