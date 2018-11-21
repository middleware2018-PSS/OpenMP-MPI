cmake_minimum_required(VERSION 3.9)
project(openmpmpi)

set(CMAKE_CXX_STANDARD 17)
set(dir ${CMAKE_CURRENT_SOURCE_DIR})
message(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(openmpmpi main.cpp game.h parsers.cpp)