cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

find_package(libregraphapi)
find_package(Qt6)

add_executable(example src/example.cpp)
target_link_libraries(example LibreGraphAPI)
#target_link_libraries(... libregraphapi::libregraphapi qt::qt)
