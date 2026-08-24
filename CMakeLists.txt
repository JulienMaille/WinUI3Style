cmake_minimum_required(VERSION 3.21)

project(WinUI3Style VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(WINUI3STYLE_BUILD_PLUGIN "Build the Qt style plugin" ON)
option(WINUI3STYLE_BUILD_DEMO "Build the widget gallery" ON)
option(WINUI3STYLE_BUILD_TESTS "Build tests" ON)
option(WINUI3STYLE_BUILD_NATIVE_TESTS
       "Build interactive native Windows tests" ON)
option(WINUI3STYLE_BUILD_BENCHMARKS
       "Build the opt-in render benchmark (not registered with CTest)" OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

add_subdirectory(src)

if(WINUI3STYLE_BUILD_PLUGIN)
    add_subdirectory(plugin)
endif()

if(WINUI3STYLE_BUILD_DEMO)
    add_subdirectory(demo)
endif()

if(WINUI3STYLE_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

if(WINUI3STYLE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(WINUI3STYLE_BUILD_DEMO AND WINUI3STYLE_BUILD_TESTS)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import PIL"
        RESULT_VARIABLE _winui3style_pillow_result
        OUTPUT_QUIET ERROR_QUIET)
    if(NOT _winui3style_pillow_result EQUAL 0)
        message(FATAL_ERROR
            "Pillow is required for the mandatory WinUI3Style snapshot oracle")
    endif()
    add_test(
            NAME winui3style_snapshot_matrix
            COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_snapshot_matrix.py
                --gallery $<TARGET_FILE:winui3style_gallery>
                --baseline ${CMAKE_CURRENT_SOURCE_DIR}/spec/baselines/gallery
                --report ${CMAKE_CURRENT_BINARY_DIR}/snapshot-report.json
    )
    set_tests_properties(winui3style_snapshot_matrix PROPERTIES
            RUN_SERIAL TRUE
            LABELS "offscreen;snapshot"
            ENVIRONMENT
                "QT_QPA_PLATFORM=offscreen;QT_ACCESSIBILITY=0;WINUI3STYLE_DISABLE_ANIMATIONS=1;QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/plugins"
    )
    if(WIN32)
            get_filename_component(_winui3style_snapshot_qt_bin
                "${Qt6_DIR}/../../../bin" ABSOLUTE)
            set(_winui3style_snapshot_path
                "${_winui3style_snapshot_qt_bin};${CMAKE_BINARY_DIR}/src/$<CONFIG>;${CMAKE_BINARY_DIR}/demo/$<CONFIG>;${CMAKE_BINARY_DIR}/plugins/styles;$ENV{PATH}")
            string(REPLACE ";" "\\;" _winui3style_snapshot_path
                   "${_winui3style_snapshot_path}")
            set_tests_properties(winui3style_snapshot_matrix PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_FONTDIR=$ENV{SystemRoot}/Fonts;QT_ACCESSIBILITY=0;WINUI3STYLE_DISABLE_ANIMATIONS=1;QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/plugins;PATH=${_winui3style_snapshot_path}")
    endif()
endif()

add_test(
    NAME winui3style_no_stylesheets
    COMMAND ${CMAKE_COMMAND}
        -DPROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_no_stylesheets.cmake
)

configure_package_config_file(
    cmake/WinUI3StyleConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/WinUI3StyleConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinUI3Style
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/WinUI3StyleConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(EXPORT WinUI3StyleTargets
    FILE WinUI3StyleTargets.cmake
    NAMESPACE WinUI3::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinUI3Style
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/WinUI3StyleConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/WinUI3StyleConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinUI3Style
)
