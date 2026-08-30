add_executable(winui3style_tests tst_winui3style.cpp)
target_link_libraries(winui3style_tests PRIVATE WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    target_compile_definitions(winui3style_tests PRIVATE WINUI3STYLE_TEST_PLUGIN)
    add_dependencies(winui3style_tests winui3styleplugin)
endif()

add_executable(winui3style_frameproperties_tests
    tst_winui3frameproperties.cpp)
target_include_directories(winui3style_frameproperties_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_frameproperties_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Core)

add_executable(winui3style_appearancewatcher_tests
    tst_winui3appearancewatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/winui3appearancewatcher_p.cpp)
target_include_directories(winui3style_appearancewatcher_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_appearancewatcher_tests PRIVATE
    Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Core)

add_executable(winui3style_density_tests tst_winui3density.cpp)
target_include_directories(winui3style_density_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_density_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_density_tests winui3styleplugin)
endif()

add_executable(winui3style_density_api_tests tst_winui3density_api.cpp)
target_link_libraries(winui3style_density_api_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_density_api_tests winui3styleplugin)
endif()

add_executable(winui3style_density_widgets_tests
    tst_winui3density_widgets.cpp)
target_link_libraries(winui3style_density_widgets_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_density_widgets_tests winui3styleplugin)
endif()

if(WINUI3STYLE_BUILD_NATIVE_TESTS AND WIN32)
    add_executable(winui3style_native_tests tst_winui3style_native.cpp)
    target_include_directories(winui3style_native_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src)
    target_link_libraries(winui3style_native_tests PRIVATE
        WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
    if(WINUI3STYLE_BUILD_PLUGIN)
        add_dependencies(winui3style_native_tests winui3styleplugin)
    endif()
endif()

# On Windows the executable directory is searched before PATH. Keep the
# runtime DLL beside every linked test executable so an old manual copy can
# never shadow the library that was just rebuilt.
if(WIN32 AND NOT WINUI3STYLE_STATIC)
    set(_winui3style_linked_test_targets
        winui3style_tests
        winui3style_frameproperties_tests
        winui3style_density_tests
        winui3style_density_api_tests
        winui3style_density_widgets_tests)
    if(WINUI3STYLE_BUILD_NATIVE_TESTS)
        list(APPEND _winui3style_linked_test_targets winui3style_native_tests)
    endif()
    foreach(_target IN LISTS _winui3style_linked_test_targets)
        add_custom_command(TARGET ${_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:winui3style>
                $<TARGET_FILE_DIR:${_target}>)
    endforeach()
endif()

# Interactive-state tests must never inherit the deterministic snapshot flag
# from an outer test runner or developer shell.
add_test(NAME winui3style_unit
    COMMAND ${CMAKE_COMMAND} -E env
        --unset=WINUI3STYLE_DISABLE_ANIMATIONS
        $<TARGET_FILE:winui3style_tests>)
add_test(NAME winui3style_frameproperties_unit
         COMMAND winui3style_frameproperties_tests)
add_test(NAME winui3style_appearancewatcher_unit
         COMMAND winui3style_appearancewatcher_tests)
add_test(NAME winui3style_density_unit
         COMMAND winui3style_density_tests)
add_test(NAME winui3style_density_api_unit
         COMMAND winui3style_density_api_tests)
add_test(NAME winui3style_density_widgets_unit
         COMMAND winui3style_density_widgets_tests)
add_test(NAME winui3style_designer_gallery_contract
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_designer_gallery.cmake)
set(_winui3style_test_environment
    "QT_QPA_PLATFORM=offscreen"
    "QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/plugins"
)
if(WIN32)
    list(APPEND _winui3style_test_environment
        "QT_QPA_FONTDIR=$ENV{SystemRoot}/Fonts")
    get_filename_component(_winui3style_qt_bin "${${WINUI3STYLE_QT}_DIR}/../../../bin" ABSOLUTE)
    set(_winui3style_test_path
        "${_winui3style_qt_bin};$<TARGET_FILE_DIR:winui3style>;$ENV{PATH}")
    string(REPLACE ";" "\\;" _winui3style_test_path "${_winui3style_test_path}")
    list(APPEND _winui3style_test_environment "PATH=${_winui3style_test_path}")
endif()
set_tests_properties(winui3style_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_frameproperties_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_appearancewatcher_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_density_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_density_api_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_density_widgets_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)

add_test(NAME winui3style_dpi_100
         COMMAND winui3style_tests dpiGeometry dpiHitTestContracts
                 radioDotDpiGeometry
                 splitterGripPixelAlignment settingsCardInteractiveFrames
                 navigationInteractiveFrames)
add_test(NAME winui3style_dpi_125
         COMMAND winui3style_tests dpiGeometry dpiHitTestContracts
                 radioDotDpiGeometry
                 splitterGripPixelAlignment settingsCardInteractiveFrames
                 navigationInteractiveFrames)
add_test(NAME winui3style_dpi_150
         COMMAND winui3style_tests dpiGeometry dpiHitTestContracts
                 radioDotDpiGeometry
                 splitterGripPixelAlignment settingsCardInteractiveFrames
                 navigationInteractiveFrames)
add_test(NAME winui3style_dpi_200
         COMMAND winui3style_tests dpiGeometry dpiHitTestContracts
                 radioDotDpiGeometry
                 splitterGripPixelAlignment settingsCardInteractiveFrames
                 navigationInteractiveFrames)
set_tests_properties(
    winui3style_dpi_100 winui3style_dpi_125
    winui3style_dpi_150 winui3style_dpi_200
    PROPERTIES ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_dpi_100 PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment};QT_ENABLE_HIGHDPI_SCALING=1;QT_SCALE_FACTOR=1")
set_tests_properties(winui3style_dpi_125 PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment};QT_ENABLE_HIGHDPI_SCALING=1;QT_SCALE_FACTOR=1.25")
set_tests_properties(winui3style_dpi_150 PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment};QT_ENABLE_HIGHDPI_SCALING=1;QT_SCALE_FACTOR=1.5")
set_tests_properties(winui3style_dpi_200 PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment};QT_ENABLE_HIGHDPI_SCALING=1;QT_SCALE_FACTOR=2")

if(WINUI3STYLE_BUILD_NATIVE_TESTS AND WIN32)
    add_test(NAME winui3style_native COMMAND winui3style_native_tests)
    set(_winui3style_native_environment
        "QT_QPA_PLATFORM=windows"
        "QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/plugins"
        "WINUI3STYLE_DISABLE_ANIMATIONS=1"
    )
    if(WIN32)
        set(_winui3style_native_path
            "${_winui3style_qt_bin};$<TARGET_FILE_DIR:winui3style>;$ENV{PATH}")
        string(REPLACE ";" "\\;" _winui3style_native_path
               "${_winui3style_native_path}")
        list(APPEND _winui3style_native_environment
            "PATH=${_winui3style_native_path}")
    endif()
    set_tests_properties(winui3style_native PROPERTIES
        ENVIRONMENT "${_winui3style_native_environment}"
        RUN_SERIAL TRUE
        RESOURCE_LOCK winui3style_desktop
        LABELS "native;interactive"
        TIMEOUT 180
    )
endif()
