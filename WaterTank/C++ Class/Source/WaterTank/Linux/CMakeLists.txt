cmake_minimum_required(VERSION 3.0)

project(watertank)

set(CMAKE_BUILD_TYPE Debug)
add_library(libwatertank STATIC WaterTankDataset.hpp WaterTankDatamodel.hpp WaterTankDatamodel.cpp WaterTankLogger.hpp WaterTankLogger.cpp)
target_include_directories(libwatertank PUBLIC ..)
add_executable(watertank termination.c watertank.cpp exos_watertank.c)
target_include_directories(watertank PUBLIC ..)
target_link_libraries(watertank libwatertank zmq exos-api)
