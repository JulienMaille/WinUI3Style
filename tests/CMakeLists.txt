# Domain split of tst_winui3style.cpp (plan step 8): test-function names are
# kept identical so the DPI reruns below keep addressing tests by function name.
set(_winui3style_split_tests
    winui3style_buttons_tests tst_winui3buttons.cpp
    winui3style_toggles_tests tst_winui3toggles.cpp
    winui3style_views_tests tst_winui3views.cpp
    winui3style_menus_tests tst_winui3menus.cpp
    winui3style_editors_tests tst_winui3editors.cpp
    winui3style_combobox_tests tst_winui3combobox.cpp
    winui3style_surfaces_tests tst_winui3surfaces.cpp
    winui3style_interaction_tests tst_winui3interaction.cpp
    winui3style_navigation_tests tst_winui3navigation.cpp)
list(LENGTH _winui3style_split_tests _winui3style_split_len)
math(EXPR _winui3style_split_last "${_winui3style_split_len} - 1")
foreach(_i RANGE 0 ${_winui3style_split_last} 2)
    list(GET _winui3style_split_tests ${_i} _target)
    math(EXPR _src_i "${_i} + 1")
    list(GET _winui3style_split_tests ${_src_i} _src)
    add_executable(${_target} ${_src})
    target_include_directories(${_target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src)
    target_link_libraries(${_target} PRIVATE WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
    if(WINUI3STYLE_BUILD_PLUGIN)
        target_compile_definitions(${_target} PRIVATE WINUI3STYLE_TEST_PLUGIN)
        add_dependencies(${_target} winui3styleplugin)
    endif()
endforeach()

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
        winui3style_buttons_tests
        winui3style_toggles_tests
        winui3style_views_tests
        winui3style_menus_tests
        winui3style_editors_tests
        winui3style_combobox_tests
        winui3style_surfaces_tests
        winui3style_interaction_tests
        winui3style_navigation_tests
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
foreach(_split_target IN ITEMS
        winui3style_buttons_tests
        winui3style_toggles_tests
        winui3style_views_tests
        winui3style_menus_tests
        winui3style_editors_tests
        winui3style_combobox_tests
        winui3style_surfaces_tests
        winui3style_interaction_tests
        winui3style_navigation_tests)
    string(REPLACE "winui3style_" "" _suffix "${_split_target}")
    string(REPLACE "_tests" "" _suffix "${_suffix}")
    add_test(NAME winui3style_${_suffix}_unit
        COMMAND ${CMAKE_COMMAND} -E env
            --unset=WINUI3STYLE_DISABLE_ANIMATIONS
            $<TARGET_FILE:${_split_target}>)
endforeach()
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
add_test(NAME winui3style_source_contracts
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/check_source_contracts.cmake)
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
foreach(_suffix IN ITEMS buttons toggles views menus editors combobox surfaces interaction navigation)
    set_tests_properties(winui3style_${_suffix}_unit PROPERTIES
        ENVIRONMENT "${_winui3style_test_environment}"
    )
endforeach()
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

# DPI reruns address tests by function name (names identical after the
# split); each function runs in its own domain binary.
set(_winui3style_dpi_cases
    "winui3style_views_tests:dpiGeometry,dpiHitTestContracts"
    "winui3style_toggles_tests:radioDotDpiGeometry"
    "winui3style_buttons_tests:toolbarButtonCornerSymmetry"
    "winui3style_menus_tests:splitterGripPixelAlignment"
    "winui3style_interaction_tests:settingsCardInteractiveFrames"
    "winui3style_navigation_tests:navigationInteractiveFrames")
foreach(_level IN ITEMS 100 125 150 200)
    foreach(_case IN LISTS _winui3style_dpi_cases)
        string(REPLACE ":" ";" _pair "${_case}")
        list(GET _pair 0 _dpi_target)
        list(GET _pair 1 _dpi_funcs_csv)
        string(REPLACE "," ";" _dpi_funcs "${_dpi_funcs_csv}")
        string(REPLACE "," "_" _dpi_suffix "${_dpi_funcs_csv}")
        add_test(NAME winui3style_dpi_${_level}_${_dpi_target}
            COMMAND ${_dpi_target} ${_dpi_funcs})
    endforeach()
endforeach()
foreach(_level IN ITEMS 100 125 150 200)
    foreach(_dpi_target IN ITEMS
            winui3style_views_tests
            winui3style_toggles_tests
            winui3style_buttons_tests
            winui3style_menus_tests
            winui3style_interaction_tests
            winui3style_navigation_tests)
        set_tests_properties(winui3style_dpi_${_level}_${_dpi_target} PROPERTIES
            ENVIRONMENT "${_winui3style_test_environment}")
    endforeach()
endforeach()
foreach(_pair IN ITEMS "100:1" "125:1.25" "150:1.5" "200:2")
    string(REPLACE ":" ";" _parts "${_pair}")
    list(GET _parts 0 _level)
    list(GET _parts 1 _factor)
    foreach(_dpi_target IN ITEMS
            winui3style_views_tests
            winui3style_toggles_tests
            winui3style_buttons_tests
            winui3style_menus_tests
            winui3style_interaction_tests
            winui3style_navigation_tests)
        set_tests_properties(winui3style_dpi_${_level}_${_dpi_target} PROPERTIES
            ENVIRONMENT "${_winui3style_test_environment};QT_ENABLE_HIGHDPI_SCALING=1;QT_SCALE_FACTOR=${_factor}")
    endforeach()
endforeach()

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
