cmake_minimum_required(VERSION 3.0)

project(watertank)

set(CMAKE_BUILD_TYPE Debug)
add_executable(watertank termination.c watertank.c exos_watertank.c)
target_include_directories(watertank PUBLIC ..)
target_link_libraries(watertank zmq exos-api)
