cmake_minimum_required(VERSION 3.21)

if(NOT BUILD_CONFIG)
    set(BUILD_CONFIG Release)
endif()

set(configure_args -G "${GENERATOR}"
    "-D${QT_PACKAGE}_DIR=${QT_DIR}"
    "-DCMAKE_BUILD_TYPE=${BUILD_CONFIG}")
if(QT_PACKAGE STREQUAL "Qt5")
    list(APPEND configure_args -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON)
endif()
foreach(setting GENERATOR_PLATFORM GENERATOR_TOOLSET CXX_COMPILER MAKE_PROGRAM)
    if(NOT "${${setting}}" STREQUAL "")
        list(APPEND configure_args "-DCMAKE_${setting}=${${setting}}")
    endif()
endforeach()

function(run_cmake)
    execute_process(COMMAND "${CMAKE_COMMAND}" ${ARGN}
        COMMAND_ERROR_IS_FATAL ANY)
endfunction()

file(MAKE_DIRECTORY "${TEST_DIR}/producer" "${TEST_DIR}/consumer")
file(WRITE "${TEST_DIR}/producer/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.21)
project(StaticPackageProducer LANGUAGES CXX)
add_subdirectory("${WINUI_SOURCE_DIR}" winui3style)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/$<CONFIG>-$<COMPILE_LANGUAGE>-library-options.txt"
    CONTENT "$<TARGET_PROPERTY:winui3style,COMPILE_OPTIONS>")
]=])

run_cmake(-S "${TEST_DIR}/producer" -B "${TEST_DIR}/build"
    ${configure_args}
    "-DWINUI_SOURCE_DIR=${SOURCE_DIR}"
    "-DCMAKE_INSTALL_PREFIX=${TEST_DIR}/install"
    -DCMAKE_INSTALL_LIBDIR=lib
    -DWINUI3STYLE_STATIC=ON
    -DWINUI3STYLE_WARNINGS_AS_ERRORS=ON
    -DWINUI_ENABLE_TESTS=OFF
    -DWINUI3STYLE_BUILD_DEMO=OFF
    -DWINUI3STYLE_BUILD_PLUGIN=OFF)

file(READ "${TEST_DIR}/build/${BUILD_CONFIG}-CXX-library-options.txt" library_options)
if(NOT library_options MATCHES "(/W4|-Wall)" OR
   NOT library_options MATCHES "(/WX|-Werror)")
    message(FATAL_ERROR "The library lost its warning policy: ${library_options}")
endif()

run_cmake(--build "${TEST_DIR}/build" --config "${BUILD_CONFIG}" --parallel 2)
run_cmake(--install "${TEST_DIR}/build" --config "${BUILD_CONFIG}")

file(READ "${TEST_DIR}/install/lib/cmake/WinUI3Style/WinUI3StyleTargets.cmake"
    installed_targets)
if(installed_targets MATCHES "winui3style_warnings")
    message(FATAL_ERROR "The installed package references the build-only warnings helper")
endif()

file(WRITE "${TEST_DIR}/consumer/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.21)
project(StaticPackageConsumer LANGUAGES CXX)
find_package(WinUI3Style CONFIG REQUIRED
    PATHS "${WINUI_PACKAGE_DIR}" NO_DEFAULT_PATH)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE WinUI3::Widgets)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/$<CONFIG>-$<COMPILE_LANGUAGE>-consumer-options.txt"
    CONTENT "$<TARGET_PROPERTY:consumer,COMPILE_OPTIONS>")
]=])
file(WRITE "${TEST_DIR}/consumer/main.cpp" [=[
#include <winui3style/winui3style.h>
#include <QApplication>

#ifndef WINUI3STYLE_STATIC
#error The installed target must propagate WINUI3STYLE_STATIC
#endif

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    WinUI3::Style style(WinUI3::ThemeMode::Light);
    return style.themeMode() == WinUI3::ThemeMode::Light ? 0 : 1;
}
]=])

run_cmake(-S "${TEST_DIR}/consumer" -B "${TEST_DIR}/consumer-build"
    ${configure_args}
    "-DWINUI_PACKAGE_DIR=${TEST_DIR}/install/lib/cmake/WinUI3Style")
file(READ "${TEST_DIR}/consumer-build/${BUILD_CONFIG}-CXX-consumer-options.txt"
    consumer_options)
if(consumer_options MATCHES "(^|[; ])(/W4|/WX|/external:W0|-Wall|-Wextra|-Wpedantic|-Werror)([; ]|$)")
    message(FATAL_ERROR "The consumer inherited the library warning policy: ${consumer_options}")
endif()
run_cmake(--build "${TEST_DIR}/consumer-build" --config "${BUILD_CONFIG}" --parallel 2)
