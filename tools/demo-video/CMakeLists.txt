cmake_minimum_required(VERSION 3.21)
project(winui3showcase LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(winui3showcase main.cpp)
target_link_libraries(winui3showcase PRIVATE Qt6::Widgets dwmapi)
set_target_properties(winui3showcase PROPERTIES WIN32_EXECUTABLE TRUE)
