cmake_minimum_required(VERSION 3.20)
project(megaman_x_dpi_demo)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${megaman_x_dpi_demo_SOURCE_DIR}/cmake/Modules")
message(${CMAKE_MODULE_PATH})
find_package(SDL2 REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})
include_directories(../dpiengine/include)
link_directories(libs)

set(CMAKE_CXX_STANDARD 14)

add_executable(megaman_x_dpi_demo src/main.cpp)
target_link_libraries(megaman_x_dpi_demo nlohmann_json::nlohmann_json dpiEngine ${CMAKE_DL_LIBS})
