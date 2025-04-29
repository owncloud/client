cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

find_package(Qt6Keychain REQUIRED)

add_executable(example src/example.cpp)
target_link_libraries(example Qt6Keychain::Qt6Keychain)
