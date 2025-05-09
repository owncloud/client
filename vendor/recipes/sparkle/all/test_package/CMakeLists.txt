cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

find_package(sparkle CONFIG REQUIRED)

add_executable(example example.mm)
target_link_libraries(example sparkle::sparkle)
