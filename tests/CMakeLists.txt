# Domain split of tst_winui3style.cpp (plan step 8): test-function names are
# kept identical so the DPI reruns below keep addressing tests by function name.
set(_winui3style_split_tests
    winui3style_buttons_tests tst_winui3buttons.cpp
    winui3style_buttonpress_tests tst_winui3buttonpress.cpp
    winui3style_toggles_tests tst_winui3toggles.cpp
    winui3style_views_tests tst_winui3views.cpp
    winui3style_menus_tests tst_winui3menus.cpp
    winui3style_editors_tests tst_winui3editors.cpp
    winui3style_combobox_tests tst_winui3combobox.cpp
    winui3style_surfaces_tests tst_winui3surfaces.cpp
    winui3style_dialogs_tests tst_winui3dialogs.cpp
    winui3style_interaction_tests tst_winui3interaction.cpp
    winui3style_settingscard_tests tst_winui3settingscard.cpp
    winui3style_animatedstack_tests tst_winui3animatedstack.cpp
    winui3style_settings_tests tst_winui3settings.cpp
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

# Standalone like the appearancewatcher binary: the system-appearance cache
# is DLL-internal, so the thread-safety mechanism tests compile the private
# theme source directly. WINUI3STYLE_STATIC keeps the export macros empty
# for the in-binary copy; advapi32/dwmapi back the registry/DWM probes.
add_executable(winui3style_theme_tests
    tst_winui3theme.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/winui3theme_p.cpp)
target_compile_definitions(winui3style_theme_tests PRIVATE WINUI3STYLE_STATIC)
target_include_directories(winui3style_theme_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(winui3style_theme_tests PRIVATE
    Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WIN32)
    target_link_libraries(winui3style_theme_tests PRIVATE advapi32 dwmapi user32)
endif()

# System-mode adaptation: because the style owns the application palette, the
# platform color scheme (not the palette) must drive ThemeMode::System. The
# test process forces a platform theme on non-Windows because the offscreen
# platform publishes no scheme by itself; Unknown still skips the contract.
add_executable(winui3style_systemtheme_tests tst_winui3systemtheme.cpp)
target_link_libraries(winui3style_systemtheme_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_systemtheme_tests winui3styleplugin)
endif()

# Standalone like the appearancewatcher binary: FrameAnimationDriver is a
# private class, so its implementation (plus the frame registry it drives)
# is compiled into the test binary instead of linking WinUI3::Widgets.
add_executable(winui3style_animations_tests
    tst_winui3animations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/winui3animations_p.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/winui3frameproperties_p.cpp)
# WINUI3STYLE_STATIC empties WINUI3STYLE_EXPORT so the registry symbols
# compiled into this binary are plain statics instead of dllimport stubs.
target_compile_definitions(winui3style_animations_tests PRIVATE WINUI3STYLE_STATIC)
target_include_directories(winui3style_animations_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(winui3style_animations_tests PRIVATE
    Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)

add_executable(winui3style_density_tests tst_winui3density.cpp)
target_include_directories(winui3style_density_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_density_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_density_tests winui3styleplugin)
endif()

# Fluent icon font-cache mechanism tests (thread isolation + value
# semantics of the cached QFont). Same shape as winui3style_density_tests:
# links the public library plus Qt Test/Widgets, resolves the private test
# seams in src/winui3icons_p.h via the src include directory.
add_executable(winui3style_icons_tests tst_winui3icons.cpp)
target_include_directories(winui3style_icons_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_icons_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_icons_tests winui3styleplugin)
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

# Menu corner convergence (60 KB cap: menus/surfaces/native binaries have no
# headroom). Same shape as winui3style_density_tests: links the public
# library plus Qt Test/Widgets, resolves private test seams in the src
# headers via the src include directory.
add_executable(winui3style_popupcorners_tests tst_winui3popupcorners.cpp)
target_include_directories(winui3style_popupcorners_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_popupcorners_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WIN32)
    target_link_libraries(winui3style_popupcorners_tests PRIVATE dwmapi user32)
endif()
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_popupcorners_tests winui3styleplugin)
endif()

# Calendar header ordering (60 KB cap: density_widgets has no headroom).
# Same shape as winui3style_popupcorners_tests: links the public library
# plus Qt Test/Widgets, resolves private test seams in the src headers via
# the src include directory.
add_executable(winui3style_calendarheader_tests tst_winui3calendarheader.cpp)
target_include_directories(winui3style_calendarheader_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_calendarheader_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_calendarheader_tests winui3styleplugin)
endif()

# Calendar surface split (60 KB cap: density_widgets has no headroom).
# Same shape as winui3style_calendarheader_tests: links the public library
# plus Qt Test/Widgets, resolves private test seams in the src headers via
# the src include directory.
add_executable(winui3style_calendarsurface_tests tst_winui3calendarsurface.cpp)
target_include_directories(winui3style_calendarsurface_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_calendarsurface_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_calendarsurface_tests winui3styleplugin)
endif()

# ScrollBar rest groove inside an opaque group card (60 KB cap: views has no
# headroom). Same shape as winui3style_calendarheader_tests: links the public
# library plus Qt Test/Widgets, resolves private test seams in the src
# headers via the src include directory.
add_executable(winui3style_scrollbarcard_tests tst_winui3scrollbarcard.cpp)
target_include_directories(winui3style_scrollbarcard_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_scrollbarcard_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_scrollbarcard_tests winui3styleplugin)
endif()

# Mica backdrop regression binary (60 KB cap: surfaces/views have no
# headroom). Same shape as winui3style_scrollbarcard_tests: links the public
# library plus Qt Test/Widgets, resolves private test seams via the src
# include directory. Card-nested backdrop samples (nav/menu in-card veils)
# live in the separate micacard binary, and the theme-switch lifecycle in
# the micatheme binary, so this file stays under the ratchet.
add_executable(winui3style_micabackdrop_tests tst_winui3micabackdrop.cpp)
target_include_directories(winui3style_micabackdrop_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_micabackdrop_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_micabackdrop_tests winui3styleplugin)
endif()

# Mica theme-switch lifecycle split (60 KB cap: micabackdrop has no
# headroom). Same shape as winui3style_micabackdrop_tests.
add_executable(winui3style_micatheme_tests tst_winui3micatheme.cpp)
target_include_directories(winui3style_micatheme_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_micatheme_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_micatheme_tests winui3styleplugin)
endif()

# Card-nested Mica samples split (60 KB cap: micabackdrop has no headroom).
# In-card nav/menu veils live here so the backdrop binary stays under the
# ratchet. Same shape as winui3style_micabackdrop_tests.
add_executable(winui3style_micacard_tests tst_winui3micacard.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../demo/gallerywindow.ui)
set_target_properties(winui3style_micacard_tests PROPERTIES
    AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../demo)
target_include_directories(winui3style_micacard_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(winui3style_micacard_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_micacard_tests winui3styleplugin)
endif()

# Exact-construction live repro: the REAL GalleryWindow class (same sources
# the reviewer runs: gallerywindow.cpp/.h/.ui), offscreen-capable. Mirrors
# the autosuggest-native target shape without its native-capture define.
add_executable(winui3style_galleryprobe_tests
    tst_winui3galleryprobe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../demo/gallerywindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../demo/gallerywindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../demo/gallerywindow.ui)
set_target_properties(winui3style_galleryprobe_tests PROPERTIES
    AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../demo)
target_include_directories(winui3style_galleryprobe_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../demo)
target_link_libraries(winui3style_galleryprobe_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_galleryprobe_tests winui3styleplugin)
endif()

# Native live repro of the reported theme-while-granted regression
# (WIN32 native suite, skipped offscreen): REAL GalleryWindow toggles, REAL
# DWM grant, verdict on presented pixels. Same shape as
# winui3style_galleryprobe_tests.
add_executable(winui3style_micanative_tests
    tst_winui3micanative.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../demo/gallerywindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../demo/gallerywindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../demo/gallerywindow.ui)
set_target_properties(winui3style_micanative_tests PROPERTIES
    AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../demo)
target_include_directories(winui3style_micanative_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../demo)
target_link_libraries(winui3style_micanative_tests PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
if(WINUI3STYLE_BUILD_PLUGIN)
    add_dependencies(winui3style_micanative_tests winui3styleplugin)
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

    add_executable(winui3style_menus_native_tests tst_winui3menus_native.cpp)
    target_include_directories(winui3style_menus_native_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src)
    target_link_libraries(winui3style_menus_native_tests PRIVATE
        WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
    if(WINUI3STYLE_BUILD_PLUGIN)
        add_dependencies(winui3style_menus_native_tests winui3styleplugin)
    endif()

    add_executable(winui3style_autosuggest_native_tests
        tst_winui3autosuggest_native.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../demo/gallerywindow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../demo/gallerywindow.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../demo/gallerywindow.ui)
    target_compile_definitions(winui3style_autosuggest_native_tests PRIVATE
        WINUI3STYLE_NATIVE_CAPTURE)
    target_include_directories(winui3style_autosuggest_native_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../demo)
    target_link_libraries(winui3style_autosuggest_native_tests PRIVATE
        WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
    if(WINUI3STYLE_BUILD_PLUGIN)
        add_dependencies(winui3style_autosuggest_native_tests winui3styleplugin)
    endif()

    add_test(NAME winui3style_micanative
        COMMAND winui3style_micanative_tests
            -o ${CMAKE_CURRENT_BINARY_DIR}/winui3style_micanative.txt,txt -o -,txt)
endif()

# On Windows the executable directory is searched before PATH. Keep the
# runtime DLL beside every linked test executable so an old manual copy can
# never shadow the library that was just rebuilt.
if(WIN32 AND NOT WINUI3STYLE_STATIC)
    set(_winui3style_linked_test_targets
        winui3style_buttons_tests
        winui3style_buttonpress_tests
        winui3style_toggles_tests
        winui3style_views_tests
        winui3style_menus_tests
        winui3style_editors_tests
        winui3style_combobox_tests
        winui3style_surfaces_tests
        winui3style_dialogs_tests
        winui3style_interaction_tests
        winui3style_settingscard_tests
        winui3style_animatedstack_tests
        winui3style_settings_tests
        winui3style_navigation_tests
        winui3style_frameproperties_tests
        winui3style_density_tests
        winui3style_density_api_tests
        winui3style_density_widgets_tests
        winui3style_icons_tests
        winui3style_popupcorners_tests
        winui3style_calendarheader_tests
        winui3style_calendarsurface_tests
        winui3style_scrollbarcard_tests
        winui3style_micabackdrop_tests
        winui3style_micatheme_tests
        winui3style_micacard_tests
        winui3style_galleryprobe_tests
        winui3style_micanative_tests)
    if(WINUI3STYLE_BUILD_NATIVE_TESTS)
        list(APPEND _winui3style_linked_test_targets winui3style_native_tests)
        list(APPEND _winui3style_linked_test_targets
            winui3style_menus_native_tests)
        list(APPEND _winui3style_linked_test_targets
            winui3style_autosuggest_native_tests)
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
        winui3style_buttonpress_tests
        winui3style_toggles_tests
        winui3style_views_tests
        winui3style_menus_tests
        winui3style_editors_tests
        winui3style_combobox_tests
        winui3style_surfaces_tests
        winui3style_dialogs_tests
        winui3style_interaction_tests
        winui3style_settingscard_tests
        winui3style_animatedstack_tests
        winui3style_settings_tests
        winui3style_navigation_tests)
    string(REPLACE "winui3style_" "" _suffix "${_split_target}")
    string(REPLACE "_tests" "" _suffix "${_suffix}")
    add_test(NAME winui3style_${_suffix}_unit
        COMMAND ${CMAKE_COMMAND} -E env
            --unset=WINUI3STYLE_DISABLE_ANIMATIONS
            $<TARGET_FILE:${_split_target}>
            -o ${CMAKE_CURRENT_BINARY_DIR}/winui3style_${_suffix}_unit.txt,txt
            -o -,txt)
endforeach()
add_test(NAME winui3style_frameproperties_unit
         COMMAND winui3style_frameproperties_tests)
add_test(NAME winui3style_animations_unit
         COMMAND winui3style_animations_tests)
add_test(NAME winui3style_appearancewatcher_unit
         COMMAND winui3style_appearancewatcher_tests)
add_test(NAME winui3style_theme_unit
         COMMAND winui3style_theme_tests)
add_test(NAME winui3style_systemtheme_unit
         COMMAND winui3style_systemtheme_tests)
add_test(NAME winui3style_density_unit
         COMMAND winui3style_density_tests)
add_test(NAME winui3style_density_api_unit
         COMMAND winui3style_density_api_tests)
add_test(NAME winui3style_density_widgets_unit
         COMMAND winui3style_density_widgets_tests)
add_test(NAME winui3style_icons_unit
         COMMAND winui3style_icons_tests)
add_test(NAME winui3style_popupcorners_unit
         COMMAND winui3style_popupcorners_tests)
add_test(NAME winui3style_calendarheader_unit
         COMMAND winui3style_calendarheader_tests)
add_test(NAME winui3style_calendarsurface_unit
         COMMAND winui3style_calendarsurface_tests)
add_test(NAME winui3style_scrollbarcard_unit
         COMMAND winui3style_scrollbarcard_tests)
add_test(NAME winui3style_micabackdrop_unit
         COMMAND winui3style_micabackdrop_tests)
add_test(NAME winui3style_micatheme_unit
         COMMAND winui3style_micatheme_tests)
add_test(NAME winui3style_micacard_unit
         COMMAND winui3style_micacard_tests)
add_test(NAME winui3style_galleryprobe_unit
         COMMAND ${CMAKE_COMMAND} -E env
             --unset=WINUI3STYLE_DISABLE_ANIMATIONS
             $<TARGET_FILE:winui3style_galleryprobe_tests>
             -o ${CMAKE_CURRENT_BINARY_DIR}/winui3style_galleryprobe_unit.txt,txt
             -o -,txt)
set_tests_properties(winui3style_galleryprobe_unit PROPERTIES TIMEOUT 180)
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
foreach(_suffix IN ITEMS buttons buttonpress toggles views menus editors combobox surfaces dialogs interaction settingscard animatedstack settings navigation micabackdrop micatheme micacard galleryprobe)
    set_tests_properties(winui3style_${_suffix}_unit PROPERTIES
        ENVIRONMENT "${_winui3style_test_environment}"
    )
endforeach()

# Phase 4 gate: an element a renderer claims must be listed in
# coveredPrimitive / coveredControl / coveredComplex, otherwise the
# Q_ASSERT_X in Style::draw* cannot fire and a covered control could silently
# reach QCommonStyle (METHODOLOGY section 1). Registered after the shared test
# environment so the binary finds the Qt runtime DLL.
add_executable(winui3style_covered_elements_check
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools/covered_elements_check.cpp)
target_link_libraries(winui3style_covered_elements_check PRIVATE
    Qt${QT_VERSION_MAJOR}::Core)
add_test(NAME winui3style_covered_elements
    COMMAND winui3style_covered_elements_check
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/winui3style.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/winui3buttons_p.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/winui3menus_p.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/winui3viewrenderers_p.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/winui3complex_p.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/winui3surfaces_p.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/winui3popupsurface_p.cpp)
set_tests_properties(winui3style_covered_elements PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
    LABELS "contract")

set_tests_properties(winui3style_frameproperties_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_animations_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_appearancewatcher_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_theme_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
# Force a platform theme that publishes a color scheme on non-Windows so the
# offscreen process can exercise the System-follows-desktop contract.
set(_winui3style_systemtheme_environment ${_winui3style_test_environment})
if(NOT WIN32)
    list(APPEND _winui3style_systemtheme_environment "QT_QPA_PLATFORMTHEME=kde")
endif()
set_tests_properties(winui3style_systemtheme_unit PROPERTIES
    ENVIRONMENT "${_winui3style_systemtheme_environment}"
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
set_tests_properties(winui3style_icons_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_popupcorners_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_calendarheader_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_calendarsurface_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_scrollbarcard_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_micabackdrop_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_micatheme_unit PROPERTIES
    ENVIRONMENT "${_winui3style_test_environment}"
)
set_tests_properties(winui3style_micacard_unit PROPERTIES
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
    add_test(NAME winui3style_native COMMAND winui3style_native_tests
        -o ${CMAKE_CURRENT_BINARY_DIR}/winui3style_native.txt,txt -o -,txt)
    add_test(NAME winui3style_menus_native COMMAND winui3style_menus_native_tests
        -o ${CMAKE_CURRENT_BINARY_DIR}/winui3style_menus_native.txt,txt -o -,txt)
    add_test(NAME winui3style_autosuggest_native
        COMMAND winui3style_autosuggest_native_tests)
    if(WINUI3STYLE_BUILD_DEMO)
        add_test(NAME winui3style_galleryprobe_native
            COMMAND winui3style_galleryprobe_tests
                galleryNavigationPaletteFollowsThemeAfterMicaDisable)
    endif()
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
    set_tests_properties(winui3style_menus_native PROPERTIES
        ENVIRONMENT "${_winui3style_native_environment}"
        RUN_SERIAL TRUE
        RESOURCE_LOCK winui3style_desktop
        LABELS "native;interactive"
        TIMEOUT 180
    )
    set_tests_properties(winui3style_autosuggest_native PROPERTIES
        ENVIRONMENT "${_winui3style_native_environment}"
        RUN_SERIAL TRUE
        RESOURCE_LOCK winui3style_desktop
        LABELS "native;interactive;opt-in"
        TIMEOUT 180)
    set_tests_properties(winui3style_micanative PROPERTIES
        ENVIRONMENT "${_winui3style_native_environment}"
        RUN_SERIAL TRUE
        RESOURCE_LOCK winui3style_desktop
        LABELS "native;interactive"
        TIMEOUT 180)
    if(WINUI3STYLE_BUILD_DEMO)
        set_tests_properties(winui3style_galleryprobe_native PROPERTIES
            ENVIRONMENT "${_winui3style_native_environment}"
            RUN_SERIAL TRUE
            RESOURCE_LOCK winui3style_desktop
            LABELS "native;interactive"
            TIMEOUT 180)
    endif()
endif()
