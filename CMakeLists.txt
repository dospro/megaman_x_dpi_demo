cmake_minimum_required(VERSION 3.20)
project(megaman_x_dpi_demo)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${megaman_x_dpi_demo_SOURCE_DIR}/cmake/Modules")
message(${CMAKE_MODULE_PATH})
find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})
include_directories(../dpiengine/include)

set(CMAKE_CXX_STANDARD 14)

add_executable(megaman_x_dpi_demo
        src/main.cpp)
